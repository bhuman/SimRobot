/**
 * @file Graphics/Metal/GraphicsContext.cpp
 *
 * This file implements a class that handles graphics using Metal 3.
 *
 * @author Arne Hasselbring
 */

#ifdef SIMROBOTCORE3_USE_METAL

#include "GraphicsContext.h"
#include "Graphics/Light.h"
#include "Platform/Assert.h"
#include "Simulation/Simulation.h"
#include <rhi/qrhi.h>
#include <bit>
#include <cstddef>

// The following shader source code is a port of the GLSL shaders of the OpenGL graphics context.

static const char* colorShaderSourceCode = R"metal(
struct VertexIn
{
  float3 positionInModel [[attribute(0)]];
  float3 normalInModel [[attribute(1)]];
  float2 textureCoordinates [[attribute(2)]];
};

struct VertexOut
{
  float4 positionInClip [[position]];
  float3 positionInWorld;
  float3 normalInWorld NORMAL_QUALIFIER;
  float2 textureCoordinates;
};

struct DirLight
{
  float3 color;
  float3 direction;
};

struct PointLight
{
  float3 color;
  float3 position;
  float constantAttenuation;
  float linearAttenuation;
  float quadraticAttenuation;
};

struct SpotLight
{
  float3 color;
  float3 position;
  float constantAttenuation;
  float linearAttenuation;
  float quadraticAttenuation;
  float3 direction;
  float cutoff;
};

// Must match GraphicsContext::surfaceMemorySize and surfaceTextureOffset.
struct Surface
{
  float4 albedoAlpha;
  float metallic;
  float roughness;
  float ambient;
  metal::texture2d<float> texture;
};

vertex VertexOut vertexMain(VertexIn in [[stage_in]], constant metal::float4x4& cameraPV [[buffer(1)]], constant metal::float4x4& modelMatrix [[buffer(2)]])
{
  VertexOut out;
  out.positionInWorld = (modelMatrix * float4(in.positionInModel, 1.0)).xyz;
  out.normalInWorld = (modelMatrix * float4(in.normalInModel, 0.0)).xyz;
  out.textureCoordinates = in.textureCoordinates;
  out.positionInClip = cameraPV * float4(out.positionInWorld, 1.0);
  return out;
}

#ifdef WITH_LIGHTING
constant float PI = 3.14159265359;

float distributionTrowbridgeReitz(float3 normal, float3 halfDir, float roughness)
{
  float alpha = roughness * roughness;
  float alphaSqr = alpha * alpha;
  float cosTheta = metal::max(metal::dot(normal, halfDir), 0.0);
  float denominator = cosTheta * cosTheta * (alphaSqr - 1.0) + 1.0;
  return alphaSqr / (PI * denominator * denominator);
}

float geometrySchlickGGX(float cosTheta, float roughness)
{
  float r = roughness + 1.0;
  float k = r * r / 8.0;
  return cosTheta / (cosTheta * (1.0 - k) + k);
}

float geometrySmith(float3 normal, float cosView, float cosLight, float roughness)
{
  return geometrySchlickGGX(cosView, roughness) * geometrySchlickGGX(cosLight, roughness);
}

float3 fresnelSchlick(float cosTheta, float3 F0)
{
  return F0 + (1.0 - F0) * metal::pow(metal::clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float3 calcBrdf(float3 lightDir, float3 normal, float3 viewDir, float3 albedo, float3 F0, float roughness, float metallic)
{
  float3 halfDir = metal::normalize(viewDir + lightDir);
  float cosView = metal::max(metal::dot(normal, viewDir), 0.0);
  float cosLight = metal::max(metal::dot(normal, lightDir), 0.0);

  float NDF = distributionTrowbridgeReitz(normal, halfDir, roughness);
  float G = geometrySmith(normal, cosView, cosLight, roughness);
  float3 F = fresnelSchlick(metal::clamp(metal::dot(halfDir, viewDir), 0.0, 1.0), F0);

  float3 numerator = NDF * G * F;
  float denominator = 4.0 * cosView * cosLight + 0.0001;
  float3 specular = numerator / denominator;

  float3 kS = F;
  float3 kD = (float3(1.0) - kS) * (1.0 - metallic);

  return kD * albedo / PI + specular;
}

float3 calcLight(float3 irradiance, float3 lightDir, float3 normal, float3 viewDir, float3 albedo, float3 F0, float roughness, float metallic)
{
  float cosTheta = metal::max(metal::dot(normal, lightDir), 0.0);
  return calcBrdf(lightDir, normal, viewDir, albedo, F0, roughness, metallic) * irradiance * cosTheta;
}

float3 calcDirLight(constant DirLight& light, float3 normal, float3 viewDir, float3 albedo, float3 F0, float roughness, float metallic)
{
  float3 lightDir = metal::normalize(-light.direction);
  float3 irradiance = light.color;

  return calcLight(irradiance, lightDir, normal, viewDir, albedo, F0, roughness, metallic);
}

float3 calcPointLight(constant PointLight& light, float3 pos, float3 normal, float3 viewDir, float3 albedo, float3 F0, float roughness, float metallic)
{
  float3 lightDir = metal::normalize(light.position - pos);
  float distance = metal::length(light.position - pos);
  float attenuation = 1.0 / (light.constantAttenuation + light.linearAttenuation * distance + light.quadraticAttenuation * distance * distance);
  float3 irradiance = light.color * attenuation;

  return calcLight(irradiance, lightDir, normal, viewDir, albedo, F0, roughness, metallic);
}

float3 calcSpotLight(constant SpotLight& light, float3 pos, float3 normal, float3 viewDir, float3 albedo, float3 F0, float roughness, float metallic)
{
  float3 lightDir = metal::normalize(light.position - pos);
  float distance = metal::length(light.position - pos);
  float attenuation = 1.0 / (light.constantAttenuation + light.linearAttenuation * distance + light.quadraticAttenuation * distance * distance);
  float cosTheta = metal::dot(lightDir, metal::normalize(-light.direction));
  float intensity = metal::clamp((cosTheta - light.cutoff) / (1 - light.cutoff), 0.0, 1.0);
  float3 irradiance = light.color * attenuation * intensity;

  return calcLight(irradiance, lightDir, normal, viewDir, albedo, F0, roughness, metallic);
}

DECLARE_LIGHTS
#endif

fragment float4 fragmentMain(VertexOut in [[stage_in]], device const Surface* surfaces [[buffer(0)]], constant float4& cameraPos [[buffer(1)]], constant uint& surfaceIndex [[buffer(2)]])
{
#ifdef WITH_TEXTURES
  constexpr metal::sampler textureSampler(metal::filter::linear, metal::mip_filter::linear, metal::address::repeat);
#endif
  device const Surface& surface = surfaces[surfaceIndex];
  float4 color;
#ifdef WITH_LIGHTING
  float3 normalizedNormal = metal::normalize(in.normalInWorld);
  float3 viewDir = metal::normalize(cameraPos.xyz - in.positionInWorld);

  float3 albedo = surface.albedoAlpha.rgb;
  color.a = surface.albedoAlpha.a;
#ifdef WITH_TEXTURES
  if(!metal::is_null_texture(surface.texture))
  {
    float4 modulation = surface.texture.sample(textureSampler, in.textureCoordinates);
    albedo = albedo * modulation.rgb;
    color.a = color.a * modulation.a;
  }
#endif
  albedo = metal::pow(albedo, float3(2.2)); // convert from sRGB to linear
  float3 F0 = float3(0.04);
  F0 = metal::mix(F0, albedo, surface.metallic);

  float3 Lo = float3(0.0);
  CALCULATE_LIGHTS
  float3 ambient = GLOBAL_AMBIENT_LIGHT * albedo * surface.ambient;
  color.rgb = ambient + Lo;
  color.rgb = color.rgb / (color.rgb + float3(1.0)); // Reinhard tone mapping
  color.rgb = metal::pow(color.rgb, float3(1.0 / 2.2)); // convert from linear to sRGB
#else
  color = surface.albedoAlpha;
#ifdef WITH_TEXTURES
  if(!metal::is_null_texture(surface.texture))
    color = color * surface.texture.sample(textureSampler, in.textureCoordinates);
#endif
#endif
  if(color.a < 0.01)
    metal::discard_fragment();
  return color;
}
)metal";

static const char* depthOnlyShaderSourceCode = R"metal(
struct VertexIn
{
  float3 positionInModel [[attribute(0)]];
};

vertex float4 vertexMain(VertexIn in [[stage_in]], constant metal::float4x4& cameraPV [[buffer(1)]], constant metal::float4x4& modelMatrix [[buffer(2)]])
{
  return cameraPV * modelMatrix * float4(in.positionInModel, 1.0);
}
)metal";

// These kernels copy render targets into buffers with the bottom row first, as glReadPixels does.
static const char* readbackShaderSourceCode = R"metal(
kernel void readbackColor(metal::texture2d<float, metal::access::read> inTexture [[texture(0)]],
                          device uchar* outBuffer [[buffer(0)]],
                          constant uint& width [[buffer(1)]],
                          uint2 gid [[thread_position_in_grid]])
{
  float4 pixel = metal::saturate(inTexture.read(uint2(gid.x, inTexture.get_height() - 1 - gid.y)));
  uint index = (gid.y * width + gid.x) * 3;
  outBuffer[index] = uchar(metal::rint(pixel.r * 255.0));
  outBuffer[index + 1] = uchar(metal::rint(pixel.g * 255.0));
  outBuffer[index + 2] = uchar(metal::rint(pixel.b * 255.0));
}

kernel void readbackDepth(metal::depth2d<float, metal::access::read> inTexture [[texture(0)]],
                          device float* outBuffer [[buffer(0)]],
                          constant uint& width [[buffer(1)]],
                          uint2 gid [[thread_position_in_grid]])
{
  outBuffer[gid.y * width + gid.x] = inTexture.read(uint2(gid.x, inTexture.get_height() - 1 - gid.y));
}
)metal";

/**
 * Converts OpenGL clip space (depth from -1 to 1) to Metal clip space (depth from 0 to 1).
 * This way, all projection matrices can stay in OpenGL convention and the depth buffer contains
 * the same values as in OpenGL (which e.g. the DepthImageSensor relies on).
 */
static const Matrix4f clipSpaceCorrection = (Matrix4f() << 1.f, 0.f, 0.f, 0.f,
                                                           0.f, 1.f, 0.f, 0.f,
                                                           0.f, 0.f, 0.5f, 0.5f,
                                                           0.f, 0.f, 0.f, 1.f).finished();

static NS::String* toNSString(const char* string)
{
  return NS::String::string(string, NS::UTF8StringEncoding);
}

static std::string toMSL(const float* vector)
{
  return "float3(" + std::to_string(vector[0]) + ", " + std::to_string(vector[1]) + ", " + std::to_string(vector[2]) + ")";
}

GraphicsContext::GraphicsContext()
{
  // The getters of the descriptors might return autoreleased objects, and there might not be a pool on the current thread.
  NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();

  // Both vertex types provide all attributes. VertexPN reuses the position as texture coordinates (they are never used).
  const auto createDescriptor = [](std::size_t stride, std::size_t textureCoordinatesOffset)
  {
    MTL::VertexDescriptor* descriptor = MTL::VertexDescriptor::alloc()->init();
    const std::size_t offsets[] = {0, 3 * sizeof(float), textureCoordinatesOffset};
    const MTL::VertexFormat formats[] = {MTL::VertexFormatFloat3, MTL::VertexFormatFloat3, MTL::VertexFormatFloat2};
    for(NS::UInteger i = 0; i < 3; ++i)
    {
      auto* attribute = descriptor->attributes()->object(i);
      attribute->setOffset(offsets[i]);
      attribute->setBufferIndex(0);
      attribute->setFormat(formats[i]);
    }
    auto* layout = descriptor->layouts()->object(0);
    layout->setStepFunction(MTL::VertexStepFunctionPerVertex);
    layout->setStepRate(1);
    layout->setStride(stride);
    return descriptor;
  };

  vertexDescriptors[VertexPN::category] = createDescriptor(VertexPN::size, 0);
  vertexDescriptors[VertexPNT::category] = createDescriptor(VertexPNT::size, 6 * sizeof(float));

  pool->release();
}

GraphicsContext::~GraphicsContext()
{
  ASSERT(!externalReferenceCounter);
  ASSERT(!commandBuffer);
  ASSERT(!selectedPipelineStates);
  ASSERT(!forcedSurface);

  if(device)
    destroyDeviceResources();

  for(MTL::VertexDescriptor* descriptor : vertexDescriptors)
    descriptor->release();
}

void GraphicsContext::compile()
{
  layoutResources();

  if(!shadowMaps.empty())
    TRACE("Shadow maps are not supported with Metal yet, so the lights are not shadowed.");

  ASSERT(!device);
  device = MTL::CreateSystemDefaultDevice();
  ASSERT(device);

  createDeviceResources();
}

void GraphicsContext::createDeviceResources()
{
  NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();

  commandQueue = device->newCommandQueue();

  // Upload vertex and index data.
  if(vertexBufferTotalSize)
  {
    vertexBuffer = device->newBuffer(vertexBufferTotalSize, MTL::ResourceCPUCacheModeDefaultCache | MTL::ResourceStorageModeShared);
    std::memset(vertexBuffer->contents(), 0, vertexBufferTotalSize);
    for(const auto& category : vertexBuffers)
      for(const auto* buffer : category)
        std::memcpy(static_cast<std::uint8_t*>(vertexBuffer->contents()) + buffer->offset, buffer->data, buffer->size());
  }

  if(indexBufferTotalSize)
  {
    indexBuffer = device->newBuffer(indexBufferTotalSize, MTL::ResourceCPUCacheModeDefaultCache | MTL::ResourceStorageModeShared);
    for(const auto* buffer : indexBuffers)
      std::memcpy(static_cast<std::uint8_t*>(indexBuffer->contents()) + buffer->offset, buffer->indices.data(), buffer->size());
  }

  // Upload textures (all of them are allocated in one heap so that they can be made resident at once).
  // The descriptors are owned by this function (they are released when the texture was created).
  mtlTextures.resize(textures.size());
  std::vector<MTL::TextureDescriptor*> textureDescriptors(textures.size());
  NS::UInteger textureHeapSize = 0;
  for(const auto& pair : textures)
  {
    const Texture* texture = pair.second;
    if(texture->data.empty())
      continue; // loading the texture failed
    MTL::TextureDescriptor* textureDescriptor = MTL::TextureDescriptor::alloc()->init();
    textureDescriptor->setTextureType(MTL::TextureType2D);
    textureDescriptor->setPixelFormat(MTL::PixelFormatRGBA8Unorm);
    textureDescriptor->setWidth(texture->width);
    textureDescriptor->setHeight(texture->height);
    textureDescriptor->setMipmapLevelCount(std::bit_width(std::max(texture->width, texture->height)));
    textureDescriptor->setUsage(MTL::TextureUsageShaderRead);
    textureDescriptor->setStorageMode(MTL::StorageModeShared); // TODO: private storage (requires uploading via a staging buffer)

    const MTL::SizeAndAlign sizeAndAlign = device->heapTextureSizeAndAlign(textureDescriptor);
    textureHeapSize = (textureHeapSize + sizeAndAlign.align - 1) & ~(sizeAndAlign.align - 1);
    textureHeapSize += sizeAndAlign.size;

    textureDescriptors[texture->index] = textureDescriptor;
  }

  if(textureHeapSize)
  {
    MTL::HeapDescriptor* heapDescriptor = MTL::HeapDescriptor::alloc()->init();
    heapDescriptor->setSize(textureHeapSize);
    heapDescriptor->setType(MTL::HeapTypeAutomatic);
    heapDescriptor->setStorageMode(MTL::StorageModeShared);
    textureHeap = device->newHeap(heapDescriptor);
    heapDescriptor->release();
    ASSERT(textureHeap);

    MTL::CommandBuffer* uploadCommandBuffer = commandQueue->commandBuffer();
    MTL::BlitCommandEncoder* blitEncoder = uploadCommandBuffer->blitCommandEncoder();
    std::vector<std::uint8_t> rgba;
    for(const auto& pair : textures)
    {
      const Texture* texture = pair.second;
      if(texture->data.empty())
        continue;
      MTL::Texture*& mtlTexture = mtlTextures[texture->index];
      mtlTexture = textureHeap->newTexture(textureDescriptors[texture->index]);
      textureDescriptors[texture->index]->release();
      ASSERT(mtlTexture);

      // Convert the pixels (blue, green, red, alpha) to RGBA.
      rgba.resize(texture->data.size());
      for(std::size_t i = 0; i < rgba.size(); i += 4)
        rgba[i] = texture->data[i + 2], rgba[i + 1] = texture->data[i + 1], rgba[i + 2] = texture->data[i], rgba[i + 3] = texture->data[i + 3];
      mtlTexture->replaceRegion(MTL::Region(0, 0, texture->width, texture->height), 0, rgba.data(), texture->width * 4);
      blitEncoder->generateMipmaps(mtlTexture);
    }
    blitEncoder->endEncoding();
    uploadCommandBuffer->commit();
    uploadCommandBuffer->waitUntilCompleted();
  }

  // Upload surfaces.
  surfaceBuffer = device->newBuffer(std::max<std::size_t>(surfaces.size(), 1) * surfaceMemorySize, MTL::ResourceCPUCacheModeDefaultCache | MTL::ResourceStorageModeShared);
  std::memset(surfaceBuffer->contents(), 0, surfaceBuffer->length());
  for(const Surface* surface : surfaces)
  {
    static constexpr std::size_t verbatimPart = offsetof(Surface, ambient) - offsetof(Surface, albedo) + sizeof(Surface::ambient);
    std::uint8_t* memory = static_cast<std::uint8_t*>(surfaceBuffer->contents()) + surface->index * surfaceMemorySize;
    std::memcpy(memory, &surface->albedo, verbatimPart);
    if(surface->texture)
    {
      const MTL::ResourceID resourceID = mtlTextures[surface->texture->index]->gpuResourceID();
      std::memcpy(memory + surfaceTextureOffset, &resourceID, sizeof(resourceID));
    }
  }

  // Compile shaders.
  std::string lightDeclarationsCode = "#define DECLARE_LIGHTS";
  std::string lightCalculationsCode = "#define CALCULATE_LIGHTS";
  for(std::size_t i = 0; i < lights.size(); ++i)
  {
    const LightData& light = lights[i];
    const std::string name = "light" + std::to_string(i);
    switch(light.type)
    {
      case LightData::directional:
        lightDeclarationsCode += " constant DirLight " + name + " = {" + toMSL(light.color) + ", " + toMSL(light.direction) + "};";
        lightCalculationsCode += " Lo += calcDirLight(" + name + ", normalizedNormal, viewDir, albedo, F0, surface.roughness, surface.metallic);";
        break;
      case LightData::spot:
        lightDeclarationsCode += " constant SpotLight " + name + " = {" + toMSL(light.color) + ", " + toMSL(light.position) + ", " + std::to_string(light.constantAttenuation) + ", " + std::to_string(light.linearAttenuation) + ", " + std::to_string(light.quadraticAttenuation) + ", " + toMSL(light.direction) + ", " + std::to_string(light.cutoff) + "};";
        lightCalculationsCode += " Lo += calcSpotLight(" + name + ", in.positionInWorld, normalizedNormal, viewDir, albedo, F0, surface.roughness, surface.metallic);";
        break;
      case LightData::point:
        lightDeclarationsCode += " constant PointLight " + name + " = {" + toMSL(light.color) + ", " + toMSL(light.position) + ", " + std::to_string(light.constantAttenuation) + ", " + std::to_string(light.linearAttenuation) + ", " + std::to_string(light.quadraticAttenuation) + "};";
        lightCalculationsCode += " Lo += calcPointLight(" + name + ", in.positionInWorld, normalizedNormal, viewDir, albedo, F0, surface.roughness, surface.metallic);";
        break;
    }
  }
  lightDeclarationsCode += "\n";
  lightCalculationsCode += "\n";

  for(std::size_t i = 0; i < numOfColorShaders; ++i)
  {
    std::string sourceCode = "#define GLOBAL_AMBIENT_LIGHT " + toMSL(globalAmbientLight) + "\n";
    if(i & 4)
      sourceCode += "#define WITH_LIGHTING\n";
    if(i & 2)
      sourceCode += "#define WITH_TEXTURES\n";
    sourceCode += (i & 1) ? "#define NORMAL_QUALIFIER\n" : "#define NORMAL_QUALIFIER [[flat]]\n";
    sourceCode += lightDeclarationsCode + lightCalculationsCode + colorShaderSourceCode;

    if(MTL::Library* library = compileLibrary(sourceCode); library)
    {
      createRenderPipelineStates(library, false, &pipelineStates[i * numOfVertexCategories]);
      library->release();
    }
  }
  if(MTL::Library* library = compileLibrary(depthOnlyShaderSourceCode); library)
  {
    createRenderPipelineStates(library, true, &pipelineStates[depthOnlyShaderIndex * numOfVertexCategories]);
    library->release();
  }
  if(MTL::Library* library = compileLibrary(readbackShaderSourceCode); library)
  {
    colorReadbackPipelineState = createComputePipelineState(library, "readbackColor");
    depthReadbackPipelineState = createComputePipelineState(library, "readbackDepth");
    library->release();
  }

  MTL::DepthStencilDescriptor* depthStencilDescriptor = MTL::DepthStencilDescriptor::alloc()->init();
  depthStencilDescriptor->setDepthWriteEnabled(true);
  depthStencilDescriptor->setDepthCompareFunction(MTL::CompareFunctionLessEqual);
  depthStencilState = device->newDepthStencilState(depthStencilDescriptor);
  depthStencilDescriptor->release();

  pool->release();
}

void GraphicsContext::destroyDeviceResources()
{
  for(const auto& pair : offscreenTargets)
  {
    pair.second.renderPass->release();
    pair.second.readbackBuffer->release();
  }
  offscreenTargets.clear();
  if(depthStencilState)
    depthStencilState->release();
  if(colorReadbackPipelineState)
    colorReadbackPipelineState->release();
  if(depthReadbackPipelineState)
    depthReadbackPipelineState->release();
  for(MTL::RenderPipelineState* pipelineState : pipelineStates)
    if(pipelineState)
      pipelineState->release();
  pipelineStates.fill(nullptr);
  surfaceBuffer->release();
  for(MTL::Texture* texture : mtlTextures)
    if(texture)
      texture->release();
  mtlTextures.clear();
  if(textureHeap)
    textureHeap->release();
  if(indexBuffer)
    indexBuffer->release();
  if(vertexBuffer)
    vertexBuffer->release();
  commandQueue->release();
  device->release();

  depthStencilState = nullptr;
  colorReadbackPipelineState = depthReadbackPipelineState = nullptr;
  surfaceBuffer = indexBuffer = vertexBuffer = nullptr;
  textureHeap = nullptr;
  commandQueue = nullptr;
  device = nullptr;
}

void GraphicsContext::createGraphics(const QRhiNativeHandles* nativeHandles)
{
  ASSERT(device);
  // The QRhi and this graphics context must use the same device, otherwise resources could not be shared.
  ASSERT(!nativeHandles || reinterpret_cast<MTL::Device*>(static_cast<const QRhiMetalNativeHandles*>(nativeHandles)->dev) == device);
  static_cast<void>(nativeHandles);
  ++externalReferenceCounter;
}

void GraphicsContext::destroyGraphics(const QRhiNativeHandles*)
{
  ASSERT(externalReferenceCounter);
  --externalReferenceCounter;
}

void GraphicsContext::updateShadowMaps(const std::function<void()>&, bool, bool)
{
  // TODO: Shadow maps are not supported by this graphics API yet.
}

MTL::PrimitiveType GraphicsContext::getPrimitiveType(PrimitiveTopology topology)
{
  switch(topology)
  {
    case pointList:
      return MTL::PrimitiveTypePoint;
    case lineList:
      return MTL::PrimitiveTypeLine;
    case triangleList:
      return MTL::PrimitiveTypeTriangle;
  }
  ASSERT(false);
  return MTL::PrimitiveTypeTriangle;
}

void GraphicsContext::startRendering(const Matrix4f& projection, const Matrix4f& view, int viewportX, int viewportY, int viewportWidth, int viewportHeight, bool lighting, bool textures, bool smoothShading, bool fillPolygons)
{
  ASSERT(encoder);
  ASSERT(!selectedPipelineStates);
  selectedPipelineStates = &pipelineStates[(renderDepthOnly ? depthOnlyShaderIndex : ((lighting ? 4 : 0) + (textures ? 2 : 0) + (smoothShading ? 1 : 0))) * numOfVertexCategories];
  boundPipelineState = nullptr;

  if(viewportX >= 0)
  {
    // The origin of viewports is at the top left in Metal but at the bottom left in OpenGL.
    ASSERT(offscreenTarget);
    MTL::Viewport viewport;
    viewport.originX = viewportX;
    viewport.originY = offscreenTargetHeight - viewportY - viewportHeight;
    viewport.width = viewportWidth;
    viewport.height = viewportHeight;
    viewport.znear = 0.0;
    viewport.zfar = 1.0;
    encoder->setViewport(viewport);
  }
  encoder->setCullMode(MTL::CullModeBack);
  encoder->setFrontFacingWinding(MTL::WindingCounterClockwise);
  encoder->setTriangleFillMode((fillPolygons || renderDepthOnly) ? MTL::TriangleFillModeFill : MTL::TriangleFillModeLines);
  encoder->setDepthStencilState(depthStencilState);
  encoder->setVertexBuffer(vertexBuffer, 0, 0);
  const Matrix4f pv = clipSpaceCorrection * projection * view;
  encoder->setVertexBytes(pv.data(), sizeof(Matrix4f), 1);
  if(!renderDepthOnly)
  {
    Vector4f pos;
    pos << -view.topLeftCorner<3, 3>().transpose() * view.topRightCorner<3, 1>(), 0.f;
    encoder->setFragmentBytes(pos.data(), sizeof(Vector4f), 1);
    encoder->setFragmentBuffer(surfaceBuffer, 0, 0);
    if(textureHeap)
      encoder->useHeap(textureHeap, MTL::RenderStageFragment);
  }
}

void GraphicsContext::setForcedSurface(const Surface* surface)
{
  ASSERT(encoder);
  forcedSurface = surface;
  if(forcedSurface && !renderDepthOnly)
    encoder->setFragmentBytes(&forcedSurface->index, sizeof(forcedSurface->index), 2);
}

void GraphicsContext::draw(const Mesh* mesh, const ModelMatrix* modelMatrix, const Surface* surface)
{
  ASSERT(encoder);
  ASSERT(selectedPipelineStates);
  MTL::RenderPipelineState* pipelineState = selectedPipelineStates[mesh->vertexBuffer->category];
  if(pipelineState != boundPipelineState)
    encoder->setRenderPipelineState((boundPipelineState = pipelineState));
  encoder->setVertexBytes(modelMatrix->getPointer(), sizeof(Matrix4f), 2);
  // Blending is always enabled, which gives the same result as OpenGL (where it is only enabled for transparent surfaces).
  if(!renderDepthOnly && !forcedSurface)
    encoder->setFragmentBytes(&surface->index, sizeof(surface->index), 2);
  if(mesh->indexBuffer)
    encoder->drawIndexedPrimitives(getPrimitiveType(mesh->topology), mesh->indexBuffer->count, MTL::IndexTypeUInt32, indexBuffer, mesh->indexBuffer->offset, 1, mesh->vertexBuffer->base, 0);
  else
    encoder->drawPrimitives(getPrimitiveType(mesh->topology), mesh->vertexBuffer->base, mesh->vertexBuffer->count);
}

void GraphicsContext::finishRendering()
{
  ASSERT(encoder);
  ASSERT(selectedPipelineStates);
  ASSERT(!forcedSurface);
  selectedPipelineStates = nullptr;
  boundPipelineState = nullptr;
}

bool GraphicsContext::startOffscreenRendering(int width, int height, bool depthOnly)
{
  ASSERT(device);
  ASSERT(width > 0 && height > 0);
  ASSERT(!commandBuffer);
  ASSERT(!encoder);
  ASSERT(!offscreenTarget);

  autoreleasePool = NS::AutoreleasePool::alloc()->init();

  offscreenTarget = &getOffscreenTarget(width, height, depthOnly);
  offscreenTargetHeight = height;
  commandBuffer = commandQueue->commandBuffer();
  encoder = commandBuffer->renderCommandEncoder(offscreenTarget->renderPass);
  renderDepthOnly = depthOnly;

  return true;
}

void GraphicsContext::finishOffscreenRendering(void* image, int w, int h)
{
  ASSERT(commandBuffer);
  ASSERT(encoder);
  ASSERT(offscreenTarget);
  ASSERT(!selectedPipelineStates);
  ASSERT(h <= offscreenTargetHeight);

  encoder->endEncoding();
  encoder = nullptr;

  const MTL::RenderPassDescriptor* renderPass = offscreenTarget->renderPass;
  MTL::Texture* texture = renderDepthOnly ? renderPass->depthAttachment()->texture() : renderPass->colorAttachments()->object(0)->texture();
  ASSERT(static_cast<int>(texture->width()) == w);
  const std::uint32_t lineLength = static_cast<std::uint32_t>(w);

  MTL::ComputeCommandEncoder* computeEncoder = commandBuffer->computeCommandEncoder();
  computeEncoder->setComputePipelineState(renderDepthOnly ? depthReadbackPipelineState : colorReadbackPipelineState);
  computeEncoder->setTexture(texture, 0);
  computeEncoder->setBuffer(offscreenTarget->readbackBuffer, 0, 0);
  computeEncoder->setBytes(&lineLength, sizeof(lineLength), 1);
  computeEncoder->dispatchThreads(MTL::Size(w, h, 1), MTL::Size(16, 16, 1));
  computeEncoder->endEncoding();

  commandBuffer->commit();
  commandBuffer->waitUntilCompleted();
  commandBuffer = nullptr;

  std::memcpy(image, offscreenTarget->readbackBuffer->contents(), static_cast<std::size_t>(w) * h * (renderDepthOnly ? sizeof(float) : 3));
  offscreenTarget = nullptr;

  autoreleasePool->release();
  autoreleasePool = nullptr;
}

bool GraphicsContext::startExternalRendering(const QRhiNativeHandles* nativeHandles)
{
  ASSERT(!commandBuffer);
  ASSERT(!encoder);
  ASSERT(!offscreenTarget);
  const auto* commandBufferHandles = static_cast<const QRhiMetalCommandBufferNativeHandles*>(nativeHandles);
  if(!commandBufferHandles || !commandBufferHandles->encoder)
    return false;
  commandBuffer = reinterpret_cast<MTL::CommandBuffer*>(commandBufferHandles->commandBuffer);
  encoder = reinterpret_cast<MTL::RenderCommandEncoder*>(commandBufferHandles->encoder);
  renderDepthOnly = false;

  return true;
}

void GraphicsContext::finishExternalRendering()
{
  ASSERT(commandBuffer);
  ASSERT(encoder);
  ASSERT(!offscreenTarget);
  ASSERT(!selectedPipelineStates);
  commandBuffer = nullptr;
  encoder = nullptr;
}

MTL::Library* GraphicsContext::compileLibrary(const std::string& sourceCode)
{
  NS::Error* error = nullptr;
  MTL::Library* library = device->newLibrary(toNSString(sourceCode.c_str()), nullptr, &error);
  if(!library)
  {
    TRACE("%s", error ? error->localizedDescription()->utf8String() : "Unknown error while compiling a Metal library.");
    ASSERT(false);
  }
  return library;
}

void GraphicsContext::createRenderPipelineStates(MTL::Library* library, bool depthOnly, MTL::RenderPipelineState** pipelineStates)
{
  MTL::RenderPipelineDescriptor* descriptor = MTL::RenderPipelineDescriptor::alloc()->init();
  MTL::Function* vertexFunction = library->newFunction(toNSString("vertexMain"));
  MTL::Function* fragmentFunction = depthOnly ? nullptr : library->newFunction(toNSString("fragmentMain"));
  descriptor->setVertexFunction(vertexFunction);
  descriptor->setFragmentFunction(fragmentFunction);
  descriptor->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);
  if(!depthOnly)
  {
    auto* attachment = descriptor->colorAttachments()->object(0);
    attachment->setPixelFormat(MTL::PixelFormatRGBA8Unorm);
    attachment->setBlendingEnabled(true);
    attachment->setSourceRGBBlendFactor(MTL::BlendFactorSourceAlpha);
    attachment->setDestinationRGBBlendFactor(MTL::BlendFactorOneMinusSourceAlpha);
    attachment->setRgbBlendOperation(MTL::BlendOperationAdd);
    // Keep the destination opaque (the render target of a QRhiWidget is composited with its alpha).
    attachment->setSourceAlphaBlendFactor(MTL::BlendFactorOne);
    attachment->setDestinationAlphaBlendFactor(MTL::BlendFactorOneMinusSourceAlpha);
    attachment->setAlphaBlendOperation(MTL::BlendOperationAdd);
  }

  for(std::size_t i = 0; i < numOfVertexCategories; ++i)
  {
    descriptor->setVertexDescriptor(vertexDescriptors[i]);
    NS::Error* error = nullptr;
    pipelineStates[i] = device->newRenderPipelineState(descriptor, &error);
    if(!pipelineStates[i])
    {
      TRACE("%s", error ? error->localizedDescription()->utf8String() : "Unknown error while creating a Metal render pipeline state.");
      ASSERT(false);
    }
  }

  if(fragmentFunction)
    fragmentFunction->release();
  vertexFunction->release();
  descriptor->release();
}

MTL::ComputePipelineState* GraphicsContext::createComputePipelineState(MTL::Library* library, const char* name)
{
  MTL::Function* function = library->newFunction(toNSString(name));
  NS::Error* error = nullptr;
  MTL::ComputePipelineState* pipelineState = device->newComputePipelineState(function, &error);
  if(!pipelineState)
  {
    TRACE("%s", error ? error->localizedDescription()->utf8String() : "Unknown error while creating a Metal compute pipeline state.");
    ASSERT(false);
  }
  function->release();
  return pipelineState;
}

GraphicsContext::OffscreenTarget& GraphicsContext::getOffscreenTarget(int width, int height, bool depthOnly)
{
  // Considering weak graphics cards clearing is faster when the render targets are not greater than they have to be.
  // So we create an individual render target for each size in demand.
  OffscreenTarget& target = offscreenTargets[static_cast<std::uint64_t>(width) << 32 | static_cast<std::uint64_t>(height) << 1 | (depthOnly ? 1 : 0)];
  if(target.renderPass)
    return target;

  target.renderPass = MTL::RenderPassDescriptor::alloc()->init();

  const auto createAttachmentTexture = [this, width, height](MTL::PixelFormat pixelFormat)
  {
    MTL::TextureDescriptor* descriptor = MTL::TextureDescriptor::alloc()->init();
    descriptor->setTextureType(MTL::TextureType2D);
    descriptor->setPixelFormat(pixelFormat);
    descriptor->setWidth(width);
    descriptor->setHeight(height);
    descriptor->setUsage(MTL::TextureUsageRenderTarget | MTL::TextureUsageShaderRead);
    descriptor->setStorageMode(MTL::StorageModePrivate);
    MTL::Texture* texture = device->newTexture(descriptor);
    descriptor->release();
    ASSERT(texture);
    return texture;
  };

  // The render pass descriptor retains the attachment textures, so they are released after being set.
  if(!depthOnly)
  {
    MTL::Texture* colorTexture = createAttachmentTexture(MTL::PixelFormatRGBA8Unorm);
    auto* colorAttachment = target.renderPass->colorAttachments()->object(0);
    colorAttachment->setTexture(colorTexture);
    colorAttachment->setLoadAction(MTL::LoadActionClear);
    colorAttachment->setStoreAction(MTL::StoreActionStore);
    colorAttachment->setClearColor(MTL::ClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]));
    colorTexture->release();
  }

  MTL::Texture* depthTexture = createAttachmentTexture(MTL::PixelFormatDepth32Float);
  auto* depthAttachment = target.renderPass->depthAttachment();
  depthAttachment->setTexture(depthTexture);
  depthAttachment->setLoadAction(MTL::LoadActionClear);
  depthAttachment->setStoreAction(depthOnly ? MTL::StoreActionStore : MTL::StoreActionDontCare);
  depthAttachment->setClearDepth(1.0);
  depthTexture->release();

  target.readbackBuffer = device->newBuffer(static_cast<std::size_t>(width) * height * (depthOnly ? sizeof(float) : 3), MTL::ResourceStorageModeShared);
  ASSERT(target.readbackBuffer);

  return target;
}

#endif
