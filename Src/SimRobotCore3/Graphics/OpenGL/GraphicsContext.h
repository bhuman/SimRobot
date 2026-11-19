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
#include <deque>
#include <functional>
#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <qopengl.h>
#undef near
#undef far

class Light;
class QOffscreenSurface;
class QOpenGLContext;
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
   * Forces the instances of the draw lists that are drawn next to use a specific surface.
   * @param surface The surface to use. \c nullptr makes them use their own surfaces again.
   */
  void setForcedSurface(const Surface* surface);

  /**
   * Draws all the instances of a draw list (see \c getDrawList) with their current model matrices.
   * @param drawList The draw list.
   */
  void drawList(const DrawList* drawList);

  /** Must be called as counterpart to \c startRendering. */
  void finishRendering();

  /**
   * Selects the OpenGL context of the off-screen renderer and an off-screen render target.
   * @param width The width of an image that will be rendered using this off-screen renderer.
   * @param height The height of an image that will be rendered using this off-screen renderer.
   * @param depthOnly Whether only depth rendering is required (then only depth images can be read).
   * @return Whether the OpenGL context was successfully selected.
   */
  bool startOffscreenRendering(int width, int height, bool depthOnly = false, bool multisample = false);

  /**
   * Submits the off-screen rendering and the conversions of its images into readback buffers (see \c readOffscreenImage)
   * without waiting for them. Must be called as counterpart to \c startOffscreenRendering.
   * @return The number of the submission (see \c waitForOffscreenRendering).
   */
  std::uint64_t finishOffscreenRendering();

  /**
   * Waits until a submission of an off-screen rendering (and all before it) has been completed, so that the contents of
   * its readback buffers are valid. This changes the rendering context to the rendering context of the off-screen renderer.
   * @param submission The number of the submission.
   */
  void waitForOffscreenRendering(std::uint64_t submission);

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
   * Draws the sky (if the scene has one and the current rendering is lit) behind everything that is drawn afterwards.
   * Must be called between \c startRendering and \c finishRendering, before the scene is drawn.
   */
  void drawSky();

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
    GLint baseInstanceLocation = -1; /**< The location of the baseInstance uniform in the program. */
    GLint forcedSurfaceLocation = -1; /**< The location of the forcedSurface uniform in the program. */
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

  /**
   * A compute shader (OpenGL: program) that converts a render target into a readback buffer, with extracted uniform locations.
   */
  struct ReadbackShader
  {
    GLuint program = 0; /**< The program object. */
    GLint widthLocation = -1; /**< The location of the width uniform in the program. */
    GLint heightLocation = -1; /**< The location of the height uniform in the program. */
    GLint firstRowLocation = -1; /**< The location of the firstRow uniform in the program. */
    GLint depthNumeratorLocation = -1; /**< The location of the depthNumerator uniform in the program. */
    GLint depthOffsetLocation = -1; /**< The location of the depthOffset uniform in the program. */
    GLint seedLocation = -1; /**< The location of the seed uniform in the program (RGB and YUYV only). */
    GLint modeledLocation = -1; /**< The location of the modeled uniform in the program (RGB and YUYV only). */
    GLint blurLocation = -1; /**< The location of the blur uniform in the program (RGB and YUYV only). */
    GLint gainLocation = -1; /**< The location of the gain uniform in the program (RGB and YUYV only). */
    GLint saturationLocation = -1; /**< The location of the saturation uniform in the program (RGB and YUYV only). */
    GLint vignetteLocation = -1; /**< The location of the vignette uniform in the program (RGB and YUYV only). */
    GLint noiseLocation = -1; /**< The location of the noise uniform in the program (RGB and YUYV only). */
  };

  /**
   * An off-screen render target.
   */
  struct OffscreenTarget
  {
    GLuint framebuffer = 0; /**< The framebuffer object. */
    GLuint colorTexture = 0; /**< The color attachment (if multisampled, the texture into which it is resolved). */
    GLuint depthTexture = 0; /**< The depth attachment (a texture, so that compute shaders can read it). */
    GLuint multisampleColorTexture = 0; /**< The multisampled color attachment (only if multisampled). */
    GLuint resolveFramebuffer = 0; /**< The framebuffer object with the resolved color (only if multisampled). */
    int width = 0; /**< The width of the render target. */
    int height = 0; /**< The height of the render target. */
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
    GLuint surfaceSSBO; /**< The shader storage buffer with the surfaces (shared between contexts within a share group). */

    std::vector<GLuint> shadowMapFBOs; /**< The FBOs of the shadow maps. These exist per context. */
    std::vector<GLuint> shadowMapTextures; /**< The depth textures of the shadow maps (shared between contexts within a share group). */

    std::vector<GLuint> textureIDs; /**< IDs for all textures (shared between contexts within a share group). */

    std::array<Shader, 10> shaders; /**< Shaders for different settings (shared between contexts within a share group). */
    std::array<DrawingShader, numOfDrawingShaders> drawingShaders; /**< Shaders for controller drawings (shared between contexts within a share group). */
    GLuint skyProgram = 0; /**< The program that draws the sky (0 if there is none; shared between contexts within a share group). */
    GLuint backdropProgram = 0; /**< The program that draws the backdrop (0 if there is none; shared between contexts within a share group). */
    GLuint environmentTexture = 0; /**< The cube map of the backdrop (0 if there is none; shared between contexts within a share group). */
    GLuint skyVAO = 0; /**< A VAO without attributes for drawing the sky. This exists per context. */
    GLuint skyUniformUBO = 0; /**< The UBO with the values for drawing the sky. This exists per context. */
    GLuint skyLightUBO = 0; /**< The UBO with the values for lighting from the sky. This exists per context. */
    std::uint64_t uploadedSkyGeneration = 0; /**< The value of \c skyGeneration that \c skyLightUBO was filled for. */

    GLuint drawingVAO = 0; /**< The VAO of the vertices of controller drawings. This exists per context. */
    GLuint drawingVBO = 0; /**< The VBO of the vertices of controller drawings. This exists per context. */
    GLuint drawingInstanceVAO = 0; /**< The VAO of the instances of meshes (with vertices of type \c VertexPN) in controller drawings. This exists per context. */
    GLuint drawingInstanceVBO = 0; /**< The VBO of the instances of meshes in controller drawings. This exists per context. */
    std::shared_ptr<std::unordered_map<std::uint64_t, DrawingTexture>> drawingTextureCache; /**< The textures of the images of controller drawings by their ids (shared between contexts within a share group). */
    std::vector<GLuint> drawingTextures; /**< The textures of the images of the controller drawings of the current frame (in the order of \c DrawingList). */

    GLuint modelMatrixSSBO = 0; /**< The buffer with all model matrices, which is filled again when they changed. This exists per context. */
    std::uint64_t uploadedModelMatrixGeneration = 0; /**< The value of \c modelMatrixGeneration that \c modelMatrixSSBO was filled for. */
    GLuint culledInstanceSSBO = 0; /**< The buffer with the instances of a pass that are inside the view (filled for each pass that is drawn). This exists per context. */
    GLuint boundInstanceSSBO = 0; /**< The buffer with instances that is currently bound in this context. */
    std::uint32_t uniformForcedSurface = 0; /**< The current value of the forcedSurface uniform of the shader in this context. */
    std::uint32_t uniformBaseInstance = 0; /**< The current value of the baseInstance uniform of the shader in this context. */
    bool blendEnabled = false; /**< The current blend state in this context. */
    GLuint boundTexture = 0; /**< The texture currently bound to texture unit 0 in this context. */
    GLuint boundNormalMap = 0; /**< The texture currently bound to the texture unit of the normal map in this context. */
    GLuint boundRoughnessMap = 0; /**< The texture currently bound to the texture unit of the roughness map in this context. */
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
   * Binds the model matrices to the shader storage buffer binding that the shaders read them from
   * (the buffer is filled again if they changed).
   */
  void bindModelMatrices();

  /**
   * Binds a buffer with instances to the shader storage buffer binding that the shaders read them from.
   * @param buffer The buffer.
   */
  void bindInstances(GLuint buffer);

  /**
   * Sets the base instance of the following draw calls.
   * @param baseInstance The index of the first instance.
   */
  void setBaseInstance(std::uint32_t baseInstance);

  /**
   * Sets the surface that overrides the surface of the instances (for the following draw calls).
   * @param surfaceIndex The index of the surface or 0xffffffff for none.
   */
  void setForcedSurfaceIndex(std::uint32_t surfaceIndex);

  /**
   * Compile a shader program from the sources of its stages.
   * @param stages The stages, each with its type and a list of source code fragments that are concatenated to form it.
   * @return A shader ID.
   */
  GLuint compileShader(const std::vector<std::pair<GLenum, std::vector<const char*>>>& stages);

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
   * Compiles the shader for depth-only renderings of instances with textures with alpha.
   * @return The shader.
   */
  Shader compileDepthOnlyAlphaTestShader();

  /**
   * Makes a shader of the current rendering the current one (if it is not), with the camera of the rendering.
   * @param newShader The shader (one of those of the current context).
   */
  void useShader(Shader* newShader);

  /**
   * Binds the textures of a surface (the albedo texture, the normal map and the roughness map, if it has them) if the
   * current shader uses textures.
   * @param surface The surface.
   */
  void bindTextures(const Surface& surface);

  /**
   * Compile a compute shader that converts a render target into a readback buffer.
   * @param format The format into which the shader converts.
   * @return A shader object.
   */
  ReadbackShader compileReadbackShader(ImageFormat format);

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

  static constexpr int multisampleCount = 4; /**< The sample count of multisampled off-screen render targets. */
  static constexpr std::size_t surfaceMemorySize = 64; /**< Size of a surface in graphics memory (four vectors of four floats, see the Surface struct in the shaders). */
  static constexpr GLuint normalMapUnit = 9; /**< The texture unit of the normal map (after those of the albedo texture and the shadow maps). */
  static constexpr GLuint roughnessMapUnit = 10; /**< The texture unit of the roughness map. */
  static constexpr GLuint environmentMapUnit = 11; /**< The texture unit of the cube map of the backdrop. */

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
  Matrix4f currentProjection = Matrix4f::Identity(); /**< The projection matrix of the current rendering. */
  Matrix4f currentView = Matrix4f::Identity(); /**< The view matrix of the current rendering. */
  Matrix4f currentPV = Matrix4f::Identity(); /**< The product of the projection and view matrix of the current rendering. */
  bool currentLighting = false; /**< Whether the current rendering is lit. */
  bool currentFillPolygons = true; /**< Whether polygons are filled in the current rendering. */
  const Surface* forcedSurface = nullptr; /**< The surface which overrides the surfaces of the instances of drawn lists. */

  // Offscreen rendering:
  QOpenGLContext* offscreenContext = nullptr; /**< The OpenGL context used for offscreen rendering. */
  QOffscreenSurface* offscreenSurface = nullptr; /**< The surface used for offscreen rendering. */
  std::unordered_map<unsigned int, OffscreenTarget> offscreenTargets; /**< Map from encoded sizes to off-screen render targets. */
  std::array<ReadbackShader, 3> readbackShaders; /**< The shaders that convert render targets into readback buffers per \c ImageFormat. */
  std::vector<GLuint> readbackBufferIDs; /**< The buffer objects of the readback buffers (persistently mapped). */
  std::deque<std::pair<std::uint64_t, GLsync>> submissions; /**< The fences of the off-screen renderings that may not have been completed yet, by their numbers. */
  std::uint64_t numOfSubmissions = 0; /**< The number of off-screen renderings that have been submitted. */
  OffscreenTarget* offscreenTarget = nullptr; /**< The current off-screen render target (only valid between \c startOffscreenRendering and \c finishOffscreenRendering). */
};

#endif
