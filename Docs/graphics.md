# Graphics in SimRobotCore3

`SimRobotCore3` renders the scene for the 3D views, for exporting images and for the camera-like sensors (`Camera`, `DepthImageSensor`, `ObjectSegmentedImageSensor`). Everything that draws goes through the class `GraphicsContext` (and the helpers in `Graphics/Primitives.h`); only the 3D view widget (`SimObjectWidget`) and the renderer (`SimObjectRenderer`) know which graphics API is used.

## Graphics APIs

The graphics API is selected when CMake is configured and fixed for a build:

| API | Platforms | 3D view | Controller drawings | Multisampling | Shadow maps |
|-|-|-|-|-|-|
| OpenGL 3.3 core (default) | all | `QOpenGLWidget` | yes | yes | yes |
| Metal | macOS (Qt >= 6.7) | `QRhiWidget` | no | yes (4x, only in the 3D view) | yes |

Select the API with the environment variable `SIMROBOT_GRAPHICS_API` when the project is generated, e.g. on macOS `SIMROBOT_GRAPHICS_API=Metal Make/macOS/generate` (the selection is cached by CMake and also used when it runs again by itself). CMake defines `SIMROBOTCORE3_USE_OPENGL` or `SIMROBOTCORE3_USE_METAL` for `SimRobotCore3` and for its interface target `SimRobotCore3Interface`, because the signatures of `SimRobotCore3::Renderer` depend on it (with Metal, they take the native handles of a `QRhi` instead of using the current OpenGL context).

OpenGL is deliberately used via `QOpenGLWidget` on all platforms (also where Qt is new enough for `QRhiWidget`), since sharing contexts with the offscreen renderer and drawing external content into a `QRhiWidget` does not work reliably with OpenGL. The long-term plan is to use Metal on macOS and Vulkan (via QRhi, which requires Qt >= 6.7) on the other platforms, but the system Qt on Linux is older on some machines.

## Structure

- `Graphics/GraphicsContextBase.{h,cpp}`: everything that does not depend on the API. It owns the CPU side of vertex/index buffers, meshes, textures (loaded with `QImage`), surfaces, model matrices (and their stacks), the clear color, the global ambient light and the lights, including the shadow maps of lights with a `ShadowMap` element (the matrices come from `Graphics/LightSpace`). `layoutResources` determines the layout of the global vertex and index buffers and the indices of textures and surfaces.
- `Graphics/OpenGL/GraphicsContext.{h,cpp}` and `Graphics/Metal/GraphicsContext.{h,cpp}`: derive from the base class and create the GPU resources, generate the shaders from the light descriptions (GLSL and MSL), and implement the render passes: `startRendering`/`draw`/`finishRendering` inside of `startOffscreenRendering`/`finishOffscreenRendering` (images for sensors, read back with the bottom row first) or `startExternalRendering`/`finishExternalRendering` (the 3D view), plus `updateShadowMaps`.
- Projection matrices always follow the OpenGL conventions (clip space depth from -1 to 1); the Metal context corrects them.

`Graphics/GraphicsContext.h` includes the context of the selected API, so the rest of the code just uses `GraphicsContext`. A new API needs its own directory, an entry there and in `Make/CMake/SimRobotCore3.cmake`.
