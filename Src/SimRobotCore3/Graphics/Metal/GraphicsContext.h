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
#include <functional>
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

  /** Determines buffer offsets of all declared buffers etc. and creates all resources on the GPU. */
  void compile();

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
   * @param depthOnly Whether only depth rendering is required (also affects what is read back in \c finishOffscreenRendering).
   * @return Whether rendering was successfully started.
   */
  bool startOffscreenRendering(int width, int height, bool depthOnly = false);

  /**
   * Reads an image from the off-screen render target. Must be called as counterpart to \c startOffscreenRendering.
   * Like in OpenGL, the bottom row is returned first.
   * @param image The buffer where is image will be saved to (RGB bytes or depth floats).
   * @param width The image width.
   * @param height The image height.
   */
  void finishOffscreenRendering(void* image, int width, int height);

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

private:
  /**
   * The resources of an off-screen render target of a specific size.
   */
  struct OffscreenTarget
  {
    MTL::RenderPassDescriptor* renderPass = nullptr; /**< The render pass descriptor (which retains the attachments). */
    MTL::Buffer* readbackBuffer = nullptr; /**< The buffer to which the result is copied (in the layout that is returned). */
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
  MTL::ComputePipelineState* colorReadbackPipelineState = nullptr; /**< The compute pipeline that converts a color attachment to RGB bytes. */
  MTL::ComputePipelineState* depthReadbackPipelineState = nullptr; /**< The compute pipeline that converts a depth attachment to floats. */
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

  bool renderingShadowMap = false; /**< Whether the current rendering is into a shadow map. */
  int renderTargetHeight = 0; /**< The height of the current off-screen render target or shadow map. */

  // Only valid between \c startRendering and \c finishRendering:
  MTL::RenderPipelineState* const* selectedPipelineStates = nullptr; /**< The pipeline states (per vertex type) of the current shader. */
  const MTL::RenderPipelineState* boundPipelineState = nullptr; /**< The pipeline state that is currently bound to the encoder. */
  const Surface* forcedSurface = nullptr; /**< The surface which overrides \c draw's argument. */
};

#endif
