/**
 * @file Graphics/Metal/GraphicsContext.h
 *
 * This file declares a class that handles graphics using Metal 3.
 *
 * @author Arne Hasselbring
 */

#pragma once

#ifdef SIMROBOTCORE3_USE_METAL

#include "Platform/Assert.h"
#include "Tools/Math/Eigen.h"
#include "Tools/Math/Pose3f.h"
#include "Graphics/GraphicsContextBase.h"
#include <Metal/Metal.hpp>
#include <array>
#include <cstdint>
#include <deque>
#include <functional>
#include <list>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class Light;
class QRhiNativeHandles;

class GraphicsContext : public GraphicsContextBase
{
public:
  static constexpr int multisampleCount = 4; /**< The sample count of render targets if multisampling is enabled (otherwise it is 1). */

  /** Constructor. */
  GraphicsContext();

  /** Destructor. */
  ~GraphicsContext();

  /**
   * Determines buffer offsets of all declared buffers etc. and creates all resources on the GPU.
   * @param errors The errors are added to this list (there are none with Metal).
   * @return Whether the resources could be created.
   */
  bool compile(std::list<std::string>& errors);

  /**
   * Registers an external renderer (i.e. a QRhi) which is going to use this graphics context.
   * Resources are shared with all users, so this only checks that the QRhi uses the same device.
   * @param nativeHandles The native handles of the QRhi.
   */
  void createGraphics(const QRhiNativeHandles* nativeHandles);

  /**
   * Unregisters an external renderer.
   * @param nativeHandles The native handles of the QRhi.
   */
  void destroyGraphics(const QRhiNativeHandles* nativeHandles);

  /**
   * Renders the shadow maps of all lights that have one. This must be called while no rendering is in progress
   * (neither external nor offscreen) and after the model matrices of everything that casts shadows were updated.
   * The shadow maps are rendered with a command buffer of this graphics context, which is completed before the function returns,
   * so this also works while a render pass of a QRhi is being recorded.
   * @param draw A function that draws everything that casts shadows (the appearances of the scene) in the current rendering pass.
   * @param offscreen Whether to render for an off-screen rendering (it makes no difference with this graphics API).
   * @param force Whether the shadow maps are also rendered if they were already updated in the current simulation step.
   */
  void updateShadowMaps(const std::function<void()>& draw, bool offscreen, bool force);

  /**
   * Starts a color render pass.
   * @param projection The projection matrix of the camera (OpenGL convention, i.e. with a clip space depth from -1 to 1).
   * @param view The view matrix (= inverse pose) of the camera.
   * @param viewportX Lower left corner of the viewport. If negative, the viewport is not set.
   * @param viewportY Lower left corner of the viewport.
   * @param viewportWidth Width of the viewport.
   * @param viewportHeight Height of the viewport.
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
   * Starts rendering into an off-screen render target.
   * @param width The width of an image that will be rendered.
   * @param height The height of an image that will be rendered.
   * @param depthOnly Whether only depth rendering is required (then only depth images can be read).
   * @return Whether rendering was successfully started.
   */
  bool startOffscreenRendering(int width, int height, bool depthOnly = false);

  /**
   * Submits the off-screen rendering and the conversions of its images into readback buffers (see \c readOffscreenImage)
   * without waiting for them. Must be called as counterpart to \c startOffscreenRendering.
   * @return The number of the submission (see \c waitForOffscreenRendering).
   */
  std::uint64_t finishOffscreenRendering();

  /**
   * Waits until a submission of an off-screen rendering (and all before it) has been completed, so that the contents of
   * its readback buffers are valid.
   * @param submission The number of the submission.
   */
  void waitForOffscreenRendering(std::uint64_t submission);

  /**
   * Starts rendering into a render pass of a QRhi (between \c beginExternal and \c endExternal).
   * The render pass must have an RGBA8 color attachment and a D32F depth attachment.
   * @param nativeHandles The native handles of the QRhi command buffer.
   * @param multisample Whether the attachments have \c multisampleCount samples (otherwise 1).
   * @return Whether rendering was successfully started.
   */
  bool startExternalRendering(const QRhiNativeHandles* nativeHandles, bool multisample);

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

private:
  /**
   * The resources of an off-screen render target of a specific size.
   */
  struct OffscreenTarget
  {
    MTL::RenderPassDescriptor* renderPass = nullptr; /**< The render pass descriptor (which retains the attachments). */
  };

  /** Creates all resources on the device. */
  void createDeviceResources();

  /** Destroys all resources on the device. */
  void destroyDeviceResources();

  /**
   * Compiles a shader library.
   * @param sourceCode The Metal shading language source code.
   * @return The library (owned by the caller) or \c nullptr if compilation failed.
   */
  MTL::Library* compileLibrary(const std::string& sourceCode);

  /**
   * Creates the render pipeline states for both vertex types.
   * @param library The library which contains the functions \c vertexMain and (unless \c depthOnly) \c fragmentMain.
   * @param depthOnly Whether the pipeline renders only depth.
   * @param multisample Whether the pipeline is used with render targets that have \c multisampleCount samples (otherwise 1).
   * @param pipelineStates The pipeline states per vertex type (output).
   */
  void createRenderPipelineStates(MTL::Library* library, bool depthOnly, bool multisample, MTL::RenderPipelineState** pipelineStates);

  /**
   * Creates a render pipeline state for controller drawings.
   * @param library The library which contains the functions.
   * @param vertexName The name of the vertex function.
   * @param fragmentName The name of the fragment function.
   * @param vertexDescriptor The layout of the vertices (or \c nullptr if the vertex function does not have any).
   * @param writeColor Whether the pipeline blends its colors into the color attachment (otherwise it renders only depth).
   * @param multisample Whether the pipeline is used with render targets that have \c multisampleCount samples (otherwise 1).
   * @return The render pipeline state or \c nullptr if its creation failed.
   */
  MTL::RenderPipelineState* createDrawingPipelineState(MTL::Library* library, const char* vertexName, const char* fragmentName, MTL::VertexDescriptor* vertexDescriptor, bool writeColor, bool multisample);

  /**
   * Creates a compute pipeline state.
   * @param library The library which contains the function.
   * @param name The name of the kernel function.
   * @return The compute pipeline state or \c nullptr if its creation failed.
   */
  MTL::ComputePipelineState* createComputePipelineState(MTL::Library* library, const char* name);

  /**
   * Returns an off-screen render target for the given parameters and creates it if necessary.
   * @param width The width of the render target.
   * @param height The height of the render target.
   * @param depthOnly Whether the render target only has a depth attachment.
   * @return The off-screen render target.
   */
  OffscreenTarget& getOffscreenTarget(int width, int height, bool depthOnly);

  /**
   * The texture of an image of a controller drawing, which exists as long as the drawing.
   */
  struct DrawingTexture
  {
    MTL::Texture* texture = nullptr; /**< The texture. */
    std::weak_ptr<const PreparedDrawing> drawing; /**< The drawing that the image belongs to. */
  };

  /** The render pipeline states for controller drawings. */
  enum DrawingPipeline
  {
    streamPipeline, /**< Points, lines and triangles with a color per vertex. */
    instancePipeline, /**< Instances of a mesh with a color per instance. */
    imagePipeline, /**< A textured mesh. */
    clearDepthPipeline, /**< A triangle that covers everything and clears the depth buffer. */
    numOfDrawingPipelineStates
  };

  static constexpr std::size_t numOfColorShaders = 8; /**< The number of color shader variants (lighting x textures x smooth shading). */
  static constexpr std::size_t depthOnlyShaderIndex = numOfColorShaders; /**< The index of the depth only shader. */
  static constexpr std::size_t numOfShaders = numOfColorShaders + 1; /**< The number of shader variants. */
  static constexpr std::size_t numOfVertexCategories = 2; /**< The number of vertex types. */
  static constexpr std::size_t numOfSampleCounts = 2; /**< The number of sample counts for which pipeline states exist (1 and \c multisampleCount). */

  static constexpr std::size_t shadowMapMemorySize = 80; /**< Size of the data of a shadow map in graphics memory (see the ShadowMapData struct in the shader). */
  static constexpr std::size_t shadowMapTextureOffset = 64; /**< Offset of the texture handle within the data of a shadow map in graphics memory. */
  static constexpr std::size_t surfaceMemorySize = 48; /**< Size of a surface in graphics memory (see the Surface struct in the shader). */
  static constexpr std::size_t surfaceTextureOffset = 32; /**< Offset of the texture handle within a surface in graphics memory. */

  /**
   * Returns the Metal primitive type that corresponds to a primitive topology.
   * @param topology The primitive topology.
   * @return The Metal primitive type.
   */
  static MTL::PrimitiveType getPrimitiveType(PrimitiveTopology topology);

  std::array<MTL::VertexDescriptor*, numOfVertexCategories> vertexDescriptors = {}; /**< The layouts of the vertex categories. */
  MTL::VertexDescriptor* drawingVertexDescriptor = nullptr; /**< The layout of the vertices of controller drawings. */

  // Device resources (created in \c compile):
  MTL::Device* device = nullptr; /**< The device used for rendering (the same as the one of the QRhi). */
  MTL::CommandQueue* commandQueue = nullptr; /**< The command queue for off-screen rendering. */
  MTL::Buffer* vertexBuffer = nullptr; /**< The global vertex buffer. */
  MTL::Buffer* indexBuffer = nullptr; /**< The global index buffer. */
  MTL::Buffer* surfaceBuffer = nullptr; /**< The buffer with the properties of all surfaces. */
  MTL::Heap* textureHeap = nullptr; /**< The heap in which all textures are allocated (\c nullptr if there are no textures). */
  std::vector<MTL::Texture*> mtlTextures; /**< All textures. */
  std::vector<MTL::Texture*> shadowMapTextures; /**< The depth textures of the shadow maps. */
  std::vector<MTL::RenderPassDescriptor*> shadowMapRenderPasses; /**< The render passes that render into the shadow maps. */
  MTL::Buffer* shadowMapBuffer = nullptr; /**< The buffer with the data of all shadow maps (\c nullptr if there are none). */
  unsigned int lastShadowMapUpdate = -1; /**< The simulation step of the last shadow map update (-1 if they were not rendered yet). */
  std::array<MTL::RenderPipelineState*, numOfSampleCounts * numOfShaders * numOfVertexCategories> pipelineStates = {}; /**< Render pipeline states per sample count, shader variant and vertex type. */
  std::array<MTL::RenderPipelineState*, numOfSampleCounts * numOfDrawingPipelineStates> drawingPipelineStates = {}; /**< Render pipeline states for controller drawings per sample count (see \c DrawingPipeline). */
  MTL::DepthStencilState* noDepthWriteState = nullptr; /**< The depth test configuration for drawings that must not write to the depth buffer. */
  MTL::DepthStencilState* clearDepthState = nullptr; /**< The depth configuration for clearing the depth buffer by drawing a triangle. */
  std::unordered_map<std::uint64_t, DrawingTexture> drawingTextureCache; /**< The textures of the images of controller drawings by their ids. */
  std::array<MTL::ComputePipelineState*, 3> readbackPipelineStates = {}; /**< The compute pipelines that convert render targets into readback buffers per \c ImageFormat. */
  std::vector<MTL::Buffer*> mtlReadbackBuffers; /**< The buffers of the readback buffers. */
  std::deque<std::pair<std::uint64_t, MTL::CommandBuffer*>> submissions; /**< The command buffers of the off-screen renderings that may not have been completed yet, by their numbers. */
  std::uint64_t numOfSubmissions = 0; /**< The number of off-screen renderings that have been submitted. */
  MTL::DepthStencilState* depthStencilState = nullptr; /**< The depth test configuration. */
  std::unordered_map<std::uint64_t, OffscreenTarget> offscreenTargets; /**< Off-screen render targets by encoded size. */
  std::size_t externalReferenceCounter = 0; /**< The number of registered external renderers. */

  // Only valid between \c start(External|Offscreen)Rendering and \c finish(External|Offscreen)Rendering:
  MTL::CommandBuffer* commandBuffer = nullptr; /**< The command buffer to which commands are encoded. */
  MTL::RenderCommandEncoder* encoder = nullptr; /**< The encoder of the current render pass. */
  bool renderDepthOnly = false; /**< Whether the current rendering is into a depth-only render target. */
  std::size_t sampleCountIndex = 0; /**< The index of the sample count (0: 1 sample, 1: \c multisampleCount samples) of the current render target in \c pipelineStates. */

  // Only valid between \c startOffscreenRendering and \c finishOffscreenRendering:
  OffscreenTarget* offscreenTarget = nullptr; /**< The current off-screen render target. */
  NS::AutoreleasePool* autoreleasePool = nullptr; /**< The pool for autoreleased objects created during off-screen rendering. */

  // Only valid between \c prepareDrawings and \c finishDrawings:
  MTL::Buffer* drawingBuffer = nullptr; /**< The buffer with the vertices of the drawings (lines, points and triangles). */
  MTL::Buffer* drawingInstanceBuffer = nullptr; /**< The buffer with the instances of meshes in the drawings (spheres and cylinders). */
  std::vector<MTL::Texture*> drawingTextures; /**< The textures of the images of the drawings (in the order of \c DrawingList), which are retained until the command buffer has been completed. */

  bool renderingShadowMap = false; /**< Whether the current rendering is into a shadow map. */
  int renderTargetHeight = 0; /**< The height of the current off-screen render target or shadow map. */

  // Only valid between \c startRendering and \c finishRendering:
  MTL::RenderPipelineState* const* selectedPipelineStates = nullptr; /**< The pipeline states (per vertex type) of the current shader. */
  const MTL::RenderPipelineState* boundPipelineState = nullptr; /**< The pipeline state that is currently bound to the encoder (also used by \c drawDrawings). */
  const Surface* forcedSurface = nullptr; /**< The surface which overrides \c draw's argument. */
};

#endif
