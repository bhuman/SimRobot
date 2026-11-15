/**
 * @file Graphics/OpenGL/GraphicsContext.h
 *
 * This file declares a class that handles graphics using OpenGL 4.5 Core.
 *
 * @author Arne Hasselbring
 */

#pragma once

#ifdef SIMROBOTCORE3_USE_OPENGL

#include "Platform/Assert.h"
#include "Tools/Math/Eigen.h"
#include "Tools/Math/Pose3f.h"
#include "Graphics/GraphicsContextBase.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <qopengl.h>
#undef near
#undef far

class Light;
class QOffscreenSurface;
class QOpenGLContext;
class QOpenGLFramebufferObject;
class QOpenGLFunctions_4_5_Core;

class GraphicsContext : public GraphicsContextBase
{
public:
  /** Constructor. */
  GraphicsContext();

  /** Destructor. */
  ~GraphicsContext();

  /**
   * Determine buffer offsets of all declared buffers etc. and prepares the off-screen renderer to render something.
   * This call changes the rendering context to the rendering context of the off-screen renderer.
   * @param errors The errors are added to this list (if OpenGL 4.5 is not available).
   * @return Whether the off-screen renderer could be created.
   */
  bool compile(std::list<std::string>& errors);

  /** Create per context data for the current context (which may include uploading data to the GPU). */
  void createGraphics();

  /** Destroy per context data for the current context. */
  void destroyGraphics();

  /**
   * Renders the shadow maps of all lights that have one. This must be called while no rendering is in progress
   * (neither external nor offscreen) and after the model matrices of everything that casts shadows were updated.
   * The framebuffer and the viewport that were bound before are restored.
   * @param draw A function that draws everything that casts shadows (the appearances of the scene) in the current rendering pass.
   * @param offscreen Whether to render in the offscreen context (otherwise, the current context is used).
   * @param force Whether the shadow maps are also rendered if they were already updated in the current simulation step.
   */
  void updateShadowMaps(const std::function<void()>& draw, bool offscreen, bool force);

  /**
   * Starts a color render pass.
   * @param projection The projection matrix of the camera.
   * @param view The view matrix (= inverse pose) of the camera.
   * @param viewportX Lower left corner of the viewport. If negative, the viewport is not set.
   * @param viewportY Lower left corner of the viewport.
   * @param viewportWidth Width of the viewport.
   * @param viewportHeight Height of the viewport.
   * @param clear Whether to clear the color and depth buffers.
   * @param lighting Whether lighting should be active.
   * @param textures Whether textures should be active.
   * @param smoothShading Whether vertex normals are interpolated.
   * @param fillPolygons Whether polygons are filled (instead of wireframe rendering).
   */
  void startRendering(const Matrix4f& projection, const Matrix4f& view, int viewportX, int viewportY, int viewportWidth, int viewportHeight, bool lighting = true, bool textures = true, bool smoothShading = true, bool fillPolygons = true);

  /**
   * Forces the following draw calls to use a specific surface.
   * @param surface The surface to use. \c nullptr makes draw calls use the default surface again.
   */
  void setForcedSurface(const Surface* surface);

  /**
   * Draws a mesh with a given transformation and surface.
   * @param mesh The mesh to draw.
   * @param modelMatrix The model matrix representing the transformation of the mesh.
   * @param surface The surface to use (ignored if a forced surface has been set).
   */
  void draw(const Mesh* mesh, const ModelMatrix* modelMatrix, const Surface* surface);

  /** Must be called as counterpart to \c startRendering. */
  void finishRendering();

  /**
   * Selects the OpenGL context of the off-screen renderer.
   * @param width The width of an image that will be rendered using this off-screen renderer.
   * @param height The height of an image that will be rendered using this off-screen renderer.
   * @param depthOnly Whether only depth rendering is required (also affects what is read back in \c finishOffscreenRendering).
   * @return Whether the OpenGL context was successfully selected.
   */
  bool startOffscreenRendering(int width, int height, bool depthOnly = false);

  /**
   * Reads an image from the current rendering context. Must be called as counterpart to \c startOffscreenRendering.
   * @param image The buffer where is image will be saved to.
   * @param width The image width.
   * @param height The image height.
   */
  void finishOffscreenRendering(void* image, int width, int height);

  /**
   * Selects the (already current) OpenGL context for rendering, e.g. into an external framebuffer.
   * @param multisample Whether multisampling is enabled (this only has an effect if the framebuffer has samples).
   * @return Whether the OpenGL context was successfully selected.
   */
  bool startExternalRendering(bool multisample);

  /** Must be called as counterpart to \c startExternalRendering. */
  void finishExternalRendering();

  /**
   * Uploads the vertices of controller drawings and the images that have not been uploaded before, and releases the textures
   * of drawings that do not exist anymore. Must be called between \c startExternalRendering and \c finishExternalRendering
   * (but not between \c startRendering and \c finishRendering) and before \c drawDrawings, also if there are no drawings.
   * \c finishDrawings must be called afterwards.
   * @param drawings The drawings of a frame.
   */
  void prepareDrawings(const DrawingList& drawings);

  /**
   * Draws controller drawings (unlit) that have been uploaded with \c prepareDrawings.
   * @param drawings The drawings of a frame.
   * @param projection The projection matrix of the camera.
   * @param view The view matrix (= inverse pose) of the camera.
   * @param alpha The factor for the opacity of everything that is drawn.
   * @param opaquePass Whether this is a pass that draws the parts of the scene that are not occluded (otherwise, drawings with the option \c disableOpacity are drawn but not the ones with \c disableTransparency).
   * @param fillPolygons Whether polygons are filled (instead of wireframe rendering).
   */
  void drawDrawings(const DrawingList& drawings, const Matrix4f& projection, const Matrix4f& view, float alpha, bool opaquePass, bool fillPolygons);

  /** Must be called as counterpart to \c prepareDrawings. */
  void finishDrawings();

  /** Clears the depth buffer (between \c startExternalRendering and \c finishExternalRendering, but not between \c startRendering and \c finishRendering). */
  void clearDepth();

  /**
   * Accesses the QOpenGLContext used for rendering. It can be used for creating further QOpenGLContexts with shared display lists and textures.
   * @return The QOpenGLContext used for rendering
   */
  QOpenGLContext* getOffscreenContext() const {return offscreenContext;}

  /**
   * Returns the OpenGL functions for the current context.
   * @return The functions or \c nullptr if the context is not registered.
   */
  QOpenGLFunctions_4_5_Core* getOpenGLFunctions() const;

private:
  /**
   * A shader (OpenGL: program) with extracted uniform locations.
   */
  struct Shader
  {
    GLuint program; /**< The program object. */
    GLint cameraPVLocation = -1; /**< The location of the cameraPV uniform in the program. */
    GLint cameraPosLocation = -1; /**< The location of the cameraPos uniform in the program. */
    GLint modelMatrixLocation = -1; /**< The location of the modelMatrix uniform in the program. */
    GLint surfaceIndexLocation = -1; /**< The location of the surfaceIndex uniform in the program. */
    std::vector<GLint> lightPVLocation; /**< The locations of the lightPV uniforms (per shadow map) in the program (the shadow map of a light is used iff its lightPV is). */
  };

  /**
   * A shader (OpenGL: program) for controller drawings with extracted uniform locations.
   */
  struct DrawingShader
  {
    GLuint program = 0; /**< The program object. */
    GLint pvmLocation = -1; /**< The location of the pvm uniform in the program. */
    GLint alphaLocation = -1; /**< The location of the alphaScale uniform in the program. */
  };

  /** The shaders for controller drawings. */
  enum DrawingShaderType
  {
    streamDrawingShader, /**< Points, lines and triangles with a color per vertex. */
    instanceDrawingShader, /**< Instances of a mesh with a color per instance. */
    imageDrawingShader, /**< A textured mesh. */
    numOfDrawingShaders
  };

  /**
   * The texture of an image of a controller drawing, which exists as long as the drawing.
   */
  struct DrawingTexture
  {
    GLuint id = 0; /**< The texture object. */
    std::weak_ptr<const PreparedDrawing> drawing; /**< The drawing that the image belongs to. */
  };

  /**
   * A structure that captures OpenGL object IDs per context (some of them are shared).
   */
  struct PerContextData
  {
    QOpenGLFunctions_4_5_Core* f = nullptr; /**< OpenGL functions for this context (shared between contexts within a share group). */

    std::vector<GLuint> vao; /**< The VAOs per vertex type. These exist per context. */
    GLuint vbo; /**< The VBO (shared between contexts within a share group). */
    GLuint ebo; /**< The EBO (shared between contexts within a share group). */
    GLuint surfaceUBO; /**< The UBO of surfaces (shared between contexts within a share group). */

    std::vector<GLuint> shadowMapFBOs; /**< The FBOs of the shadow maps. These exist per context. */
    std::vector<GLuint> shadowMapTextures; /**< The depth textures of the shadow maps (shared between contexts within a share group). */

    std::vector<GLuint> textureIDs; /**< IDs for all textures (shared between contexts within a share group). */

    std::array<Shader, 9> shaders; /**< Shaders for different settings (shared between contexts within a share group). */
    std::array<DrawingShader, numOfDrawingShaders> drawingShaders; /**< Shaders for controller drawings (shared between contexts within a share group). */

    GLuint drawingVAO = 0; /**< The VAO of the vertices of controller drawings. This exists per context. */
    GLuint drawingVBO = 0; /**< The VBO of the vertices of controller drawings. This exists per context. */
    GLuint drawingInstanceVAO = 0; /**< The VAO of the instances of meshes (with vertices of type \c VertexPN) in controller drawings. This exists per context. */
    GLuint drawingInstanceVBO = 0; /**< The VBO of the instances of meshes in controller drawings. This exists per context. */
    std::shared_ptr<std::unordered_map<std::uint64_t, DrawingTexture>> drawingTextureCache; /**< The textures of the images of controller drawings by their ids (shared between contexts within a share group). */
    std::vector<GLuint> drawingTextures; /**< The textures of the images of the controller drawings of the current frame (in the order of \c DrawingList). */

    bool blendEnabled = false; /**< The current blend state in this context. */
    GLuint boundTexture = 0; /**< The texture currently bound to texture unit 0 in this context. */
    GLuint boundVAO = 0; /**< The currently bound VAO in this context. */

    std::size_t referenceCounter = 1; /**< Reference counter for this context. */
    std::size_t referenceCounterIndex; /**< Index in the vector of reference counters for the share group this context belongs to. */
  };

  /**
   * Sets uniforms for a surface.
   * @param surface The surface to set.
   */
  void setSurface(const Surface* surface);

  /**
   * Compile a shader from a list of vertex shader sources and fragment shader sources
   * @param vertexShaderSources A list of source code fragments that are concatenated to form the vertex shader.
   * @param fragmentShaderSources A list of source code fragments that are concatenated to form the fragment shader.
   * @return A shader ID.
   */
  GLuint compileShader(const std::vector<const char*>& vertexShaderSources, const std::vector<const char*>& fragmentShaderSources);

  /**
   * Compile a shader for color render passes.
   * @param lighting Whether lighting is enabled.
   * @param textures Whether textures are enabled.
   * @param smooth Whether normals are interpolated smoothly between vertices.
   * @return A shader object.
   */
  Shader compileColorShader(bool lighting, bool textures, bool smooth);

  /**
   * Compile a shader for depth only render passes.
   * @return A shader object.
   */
  Shader compileDepthOnlyShader();

  /**
   * Compile a shader for controller drawings.
   * @param vertexShaderSourceCode The source code of the vertex shader.
   * @param fragmentShaderSourceCode The source code of the fragment shader.
   * @return A shader object.
   */
  DrawingShader compileDrawingShader(const char* vertexShaderSourceCode, const char* fragmentShaderSourceCode);

  // Context handling:
  std::vector<unsigned> referenceCounters; /**< Reference counters of shared data per share group. */
  std::unordered_map<const QOpenGLContext*, PerContextData> perContextData; /**< Map of OpenGL context pointers to per context data. */

  static constexpr std::size_t surfaceMemorySize = 32; /**< Size of a surface in graphics memory. The std140 layout pads array elements to multiples of 16 bytes. */

  /**
   * Declares a vertex attribute of a VAO.
   * @param functions The OpenGL functions to use.
   * @param vao The VAO.
   * @param index The index (location) of the attribute.
   * @param binding The buffer binding point of the VAO from which the attribute is read.
   * @param size The number of components of the attribute.
   * @param type The type of the components.
   * @param normalized Whether integer components are normalized to [0, 1].
   * @param offset The offset of the attribute in a vertex (or instance).
   */
  static void setupVertexAttribute(QOpenGLFunctions_4_5_Core& functions, GLuint vao, GLuint index, GLuint binding, GLint size, GLenum type, GLboolean normalized, std::size_t offset);

  /**
   * Declares the vertex attributes of vertices with a position and normal in a VAO (binding point 0).
   * @param functions The OpenGL functions to use.
   * @param vao The VAO.
   * @param vbo The VBO that contains the vertices.
   */
  static void setupVertexAttributesPN(QOpenGLFunctions_4_5_Core& functions, GLuint vao, GLuint vbo);

  /**
   * Declares the vertex attributes of vertices with a position, normal and texture coordinates in a VAO (binding point 0).
   * @param functions The OpenGL functions to use.
   * @param vao The VAO.
   * @param vbo The VBO that contains the vertices.
   */
  static void setupVertexAttributesPNT(QOpenGLFunctions_4_5_Core& functions, GLuint vao, GLuint vbo);

  /**
   * Declares the vertex attributes of the vertices of controller drawings in a VAO (binding point 0).
   * @param functions The OpenGL functions to use.
   * @param vao The VAO.
   * @param vbo The VBO that contains the vertices.
   */
  static void setupVertexAttributesDrawing(QOpenGLFunctions_4_5_Core& functions, GLuint vao, GLuint vbo);

  /**
   * Declares the vertex attributes of the instances of meshes in controller drawings in a VAO (binding point 1).
   * @param functions The OpenGL functions to use.
   * @param vao The VAO.
   * @param vbo The VBO that contains the instances.
   */
  static void setupVertexAttributesDrawingInstances(QOpenGLFunctions_4_5_Core& functions, GLuint vao, GLuint vbo);

  static constexpr std::array<void (*)(QOpenGLFunctions_4_5_Core&, GLuint, GLuint), numOfVertexCategories> vertexAttributeSetups = {setupVertexAttributesPN, setupVertexAttributesPNT}; /**< The functions that declare the vertex attributes per vertex category. */

  /**
   * Returns the OpenGL primitive type that corresponds to a primitive topology.
   * @param topology The primitive topology.
   * @return The OpenGL primitive type.
   */
  static GLenum getPrimitiveType(PrimitiveTopology topology);

  // Objects that are created during initialization but used throughout the runtime.
  unsigned int lastShadowMapUpdate = -1; /**< The simulation step of the last shadow map update (-1 if they were not rendered yet). */

  // Only valid between \c start(External|Offscreen)Rendering and \c finish(External|Offscreen)Rendering:
  PerContextData* data = nullptr; /**< The per context data for the current OpenGL context. */
  QOpenGLFunctions_4_5_Core* f = nullptr; /**< The OpenGL functions for the current OpenGL context. */
  bool renderDepthOnly = false; /**< Whether the current rendering is into a depth-only framebuffer. */

  // Only valid between \c startRendering and \c finishRendering:
  Shader* shader = nullptr; /**< The currently selected shader. */
  const Surface* forcedSurface = nullptr; /**< The surface which overrides \c draw's argument. */

  // Offscreen rendering:
  QOpenGLContext* offscreenContext = nullptr; /**< The OpenGL context used for offscreen rendering. */
  QOffscreenSurface* offscreenSurface = nullptr; /**< The surface used for offscreen rendering. */
  std::unordered_map<unsigned int, QOpenGLFramebufferObject*> offscreenBuffers; /**< Map from encoded sizes to framebuffer objects. */
};

#endif
