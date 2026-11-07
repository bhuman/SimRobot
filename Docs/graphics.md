# Graphics in SimRobotCore3

`SimRobotCore3` renders the scene for the 3D views, for exporting images and for the camera-like sensors (`Camera`, `DepthImageSensor`, `ObjectSegmentedImageSensor`). Everything that draws goes through the class `GraphicsContext` (and the helpers in `Graphics/Primitives.h`); only the 3D view widget (`SimObjectWidget`) and the renderer (`SimObjectRenderer`) know which graphics API is used.

## Graphics APIs

The graphics API is selected when CMake is configured and fixed for a build:

| API | Platforms | 3D view | Controller drawings | Multisampling | Shadow maps |
|-|-|-|-|-|-|
| OpenGL 3.3 core (default) | all | `QOpenGLWidget` | yes | yes | yes |
| Metal | macOS (Qt >= 6.7) | `QRhiWidget` | yes | yes (4x, in the 3D view and in exported images) | yes |

Select the API with the environment variable `SIMROBOT_GRAPHICS_API` when the project is generated, e.g. on macOS `SIMROBOT_GRAPHICS_API=Metal Make/macOS/generate` (the selection is cached by CMake and also used when it runs again by itself). CMake defines `SIMROBOTCORE3_USE_OPENGL` or `SIMROBOTCORE3_USE_METAL` for `SimRobotCore3` and for its interface target `SimRobotCore3Interface`, because the signatures of `SimRobotCore3::Renderer` depend on it (with Metal, they take the native handles of a `QRhi` instead of using the current OpenGL context).

OpenGL is deliberately used via `QOpenGLWidget` on all platforms (also where Qt is new enough for `QRhiWidget`), since sharing contexts with the offscreen renderer and drawing external content into a `QRhiWidget` does not work reliably with OpenGL. The long-term plan is to use Metal on macOS and Vulkan (via QRhi, which requires Qt >= 6.7) on the other platforms, but the system Qt on Linux is older on some machines.

## Structure

- `Graphics/GraphicsContextBase.{h,cpp}`: everything that does not depend on the API. It owns the CPU side of vertex/index buffers, meshes, textures (loaded with `QImage`), surfaces, model matrices (and their stacks), the clear color, the global ambient light and the lights, including the shadow maps of lights with a `ShadowMap` element (the matrices come from `Graphics/LightSpace`). `layoutResources` determines the layout of the global vertex and index buffers and the indices of textures and surfaces.
- `Graphics/OpenGL/GraphicsContext.{h,cpp}` and `Graphics/Metal/GraphicsContext.{h,cpp}`: derive from the base class and create the GPU resources, generate the shaders from the light descriptions (GLSL and MSL), and implement the render passes: `startRendering`/`draw`/`finishRendering` inside of `startOffscreenRendering`/`finishOffscreenRendering` (images for sensors, read back with the bottom row first) or `startExternalRendering`/`finishExternalRendering` (the 3D view), plus `updateShadowMaps`.
- Projection matrices always follow the OpenGL conventions (clip space depth from -1 to 1); the Metal context corrects them.

`Graphics/GraphicsContext.h` includes the context of the selected API, so the rest of the code just uses `GraphicsContext`. A new API needs its own directory, an entry there and in `Make/CMake/SimRobotCore3.cmake`.

## Controller drawings

A controller (like the one of B-Human) can attach flat-colored 3D primitives to physical objects of the scene with `setDrawing(key, SimRobotCore3::Drawing&&)` (see `SimRobotCore3.h`). A `Drawing` consists of lines, dots, quads, spheres, ellipsoids, cylinders/cones and images (RGBA pixels) and a transformation into the coordinate system of the object. Instead of the pixels, an image can have a function that computes them when the image is drawn for the first time, so a controller that sends images faster than they are drawn (e.g. camera images that must be converted) does not convert the ones that are never drawn. The controller does not have to know about the graphics API. A drawing can be set from any thread and is removed with `removeDrawing(key)`.

`setDrawing` only moves the drawing into the `DrawingStore` of the object, whose lock only protects the map. A drawing is prepared when it is drawn for the first time (in the thread that draws the scenes, without the lock of the map), so drawings that are replaced more often than the scene is drawn are mostly never prepared. It becomes a `GraphicsContextBase::PreparedDrawing`, which is not changed afterwards: lines, dots and quads become vertices, spheres, ellipsoids and cylinders become instances of a unit sphere and a unit cylinder (whose ends the vertex shader scales by the radii of the cylinder, which also makes cones), and the pixels of the images are moved into it (or computed by their functions), each image with a new id.

For each frame, `SimObjectRenderer` collects the shared pointers of the drawings of the displayed objects (and their transformations into the world) in a `GraphicsContextBase::DrawingList` and draws it with `prepareDrawings`, `drawDrawings` and `finishDrawings` of the graphics context. `prepareDrawings` copies the vertices and the instances of all drawings into one buffer each, and `drawDrawings` draws the instances of each drawing with one draw call per mesh (with OpenGL 3.3, which has no base instance, the attributes of the instances are declared for each draw call). The textures of images are cached by their ids: an image is uploaded when it is drawn for the first time and its texture is released when its drawing does not exist anymore (the next time `prepareDrawings` is called). With OpenGL, the cache is shared by the contexts of a share group; with Metal, a command buffer retains the textures it uses until it has been completed. The alpha of the primitives is blended with a factor per pass. The passes implement the occlusion modes of the 3D view: with occlusion, the drawings are depth tested against the scene; without it, `clearDepth` is called before so that they are drawn on top; with transparent occlusion, they are drawn depth tested with their full alpha, then the depth buffer is cleared and they are drawn again with half of the alpha. The options of a drawing (`disableDepth`, `disableTransparency`, `disableOpacity`) are evaluated per pass. Drawings are never lit or shadowed.

The old interface (`Controller3DDrawing` and `Controller3DDrawingManager`, which let the controller issue OpenGL calls itself) still exists, but only works with OpenGL.
