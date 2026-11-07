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

// Must match GraphicsContext::shadowMapMemorySize and shadowMapTextureOffset.
struct ShadowMapData
{
  metal::float4x4 lightPV;
  metal::depth2d<float> map;
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

#ifdef WITH_SHADOWS
// Returns the fraction of the fragment that is lit. The shadow map is sampled with hardware percentage closer filtering
// (a bilinear comparison) at four positions that together cover 3x3 texels. This is a port of the GLSL function, but
// the depth range of Metal's clip space (and thereby of the matrix) is already from 0 to 1.
float illumination(metal::depth2d<float> shadowMap, metal::float4x4 lightPV, float3 positionInWorld, float3 lightDir, float3 normal)
{
  constexpr metal::sampler shadowSampler(metal::coord::normalized, metal::filter::linear, metal::address::clamp_to_border, metal::border_color::opaque_white, metal::compare_func::less_equal);
  float cosTheta = metal::dot(normal, lightDir);
  if(cosTheta <= 0.0)
    return 1.0; // The surface faces away from the light, so there is no light to be blocked anyway.
  float4 fragPosInLight = lightPV * float4(positionInWorld, 1.0);
  if(fragPosInLight.w <= 0.0)
    return 1.0;
  // The size of a texel at the fragment in world coordinates (w is the distance for a perspective projection and 1 for an orthographic one).
  float2 size = float2(shadowMap.get_width(), shadowMap.get_height());
  float3 row0 = float3(lightPV[0][0], lightPV[1][0], lightPV[2][0]);
  float3 row1 = float3(lightPV[0][1], lightPV[1][1], lightPV[2][1]);
  float3 depthAxis = -float3(lightPV[0][3], lightPV[1][3], lightPV[2][3]); // zero for an orthographic projection
  float2 scale = float2(metal::sqrt(metal::max(metal::dot(row0, row0) - metal::pow(metal::dot(row0, depthAxis), 2.0), 1e-12)),
                        metal::sqrt(metal::max(metal::dot(row1, row1) - metal::pow(metal::dot(row1, depthAxis), 2.0), 1e-12)));
  float2 texelInWorld = 2.0 * fragPosInLight.w / (size * scale);
  // Moving the lookup position along the normal avoids self-shadowing, the more the surface is inclined towards the light.
  float3 offsetPos = positionInWorld + normal * (1.5 * metal::max(texelInWorld.x, texelInWorld.y) * metal::sqrt(metal::max(1.0 - cosTheta * cosTheta, 0.0)));
  fragPosInLight = lightPV * float4(offsetPos, 1.0);
  float3 coordinates = fragPosInLight.xyz / fragPosInLight.w;
  if(coordinates.z > 1.0)
    return 1.0;
  float2 position = float2(0.5 * coordinates.x + 0.5, 0.5 - 0.5 * coordinates.y);
  float2 texel = 1.0 / size;
  float reference = coordinates.z - 0.0005;
  float visibility = 0.0;
  for(int x = -1; x <= 1; x += 2)
  {
    for(int y = -1; y <= 1; y += 2)
      visibility += shadowMap.sample_compare(shadowSampler, position + 0.5 * float2(x, y) * texel, reference, metal::level(0));
  }
  return 0.25 * visibility;
}
#endif

float3 calcLight(float3 irradiance, float3 lightDir, float3 normal, float3 viewDir, float3 albedo, float3 F0, float roughness, float metallic, float visibility)
{
  float cosTheta = metal::max(metal::dot(normal, lightDir), 0.0);
  return calcBrdf(lightDir, normal, viewDir, albedo, F0, roughness, metallic) * irradiance * cosTheta * visibility;
}

float3 calcDirLight(constant DirLight& light, float3 normal, float3 viewDir, float3 albedo, float3 F0, float roughness, float metallic, float visibility)
{
  float3 lightDir = metal::normalize(-light.direction);
  float3 irradiance = light.color;

  return calcLight(irradiance, lightDir, normal, viewDir, albedo, F0, roughness, metallic, visibility);
}

float3 calcPointLight(constant PointLight& light, float3 pos, float3 normal, float3 viewDir, float3 albedo, float3 F0, float roughness, float metallic, float visibility)
{
  float3 lightDir = metal::normalize(light.position - pos);
  float distance = metal::length(light.position - pos);
  float attenuation = 1.0 / (light.constantAttenuation + light.linearAttenuation * distance + light.quadraticAttenuation * distance * distance);
  float3 irradiance = light.color * attenuation;

  return calcLight(irradiance, lightDir, normal, viewDir, albedo, F0, roughness, metallic, visibility);
}

float3 calcSpotLight(constant SpotLight& light, float3 pos, float3 normal, float3 viewDir, float3 albedo, float3 F0, float roughness, float metallic, float visibility)
{
  float3 lightDir = metal::normalize(light.position - pos);
  float distance = metal::length(light.position - pos);
  float attenuation = 1.0 / (light.constantAttenuation + light.linearAttenuation * distance + light.quadraticAttenuation * distance * distance);
  float cosTheta = metal::dot(lightDir, metal::normalize(-light.direction));
  float intensity = metal::clamp((cosTheta - light.cutoff) / (1 - light.cutoff), 0.0, 1.0);
  float3 irradiance = light.color * attenuation * intensity;

  return calcLight(irradiance, lightDir, normal, viewDir, albedo, F0, roughness, metallic, visibility);
}

DECLARE_LIGHTS
#endif

fragment float4 fragmentMain(VertexOut in [[stage_in]], device const Surface* surfaces [[buffer(0)]], constant float4& cameraPos [[buffer(1)]], constant uint& surfaceIndex [[buffer(2)]]
#ifdef WITH_SHADOWS
                             , device const ShadowMapData* shadowMaps [[buffer(3)]]
#endif
                            )
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

// The shaders of controller drawings (which are not lit).
static const char* drawingShaderSourceCode = R"metal(
struct StreamIn
{
  float3 position [[attribute(0)]];
  float4 color [[attribute(1)]];
  float size [[attribute(2)]];
};

struct StreamOut
{
  float4 position [[position]];
  float4 color;
  float pointSize [[point_size]];
};

struct FlatIn
{
  float3 position [[attribute(0)]];
};

struct ColorIn
{
  float4 position [[position]];
  float4 color;
};

struct ImageIn
{
  float3 position [[attribute(0)]];
  float2 textureCoordinates [[attribute(2)]];
};

struct ImageOut
{
  float4 position [[position]];
  float2 textureCoordinates;
};

vertex StreamOut drawingStreamVertex(StreamIn in [[stage_in]], constant metal::float4x4& pvm [[buffer(1)]], constant float& alphaScale [[buffer(2)]])
{
  StreamOut out;
  out.position = pvm * float4(in.position, 1.0);
  out.color = float4(in.color.rgb, in.color.a * alphaScale);
  out.pointSize = in.size;
  return out;
}

vertex ColorIn drawingFlatVertex(FlatIn in [[stage_in]], constant metal::float4x4& pvm [[buffer(1)]], constant float4& color [[buffer(2)]], constant float& alphaScale [[buffer(3)]])
{
  ColorIn out;
  out.position = pvm * float4(in.position, 1.0);
  out.color = float4(color.rgb, color.a * alphaScale);
  return out;
}

fragment float4 drawingColorFragment(ColorIn in [[stage_in]])
{
  return in.color;
}

vertex ImageOut drawingImageVertex(ImageIn in [[stage_in]], constant metal::float4x4& pvm [[buffer(1)]])
{
  ImageOut out;
  out.position = pvm * float4(in.position, 1.0);
  out.textureCoordinates = in.textureCoordinates;
  return out;
}

fragment float4 drawingImageFragment(ImageOut in [[stage_in]], metal::texture2d<float> image [[texture(0)]], constant float& alphaScale [[buffer(0)]])
{
  constexpr metal::sampler imageSampler(metal::filter::linear, metal::address::clamp_to_edge);
  return float4(image.sample(imageSampler, in.textureCoordinates).rgb, alphaScale);
}

// Covers everything with the farthest depth (the depth test is configured to always pass).
vertex float4 drawingClearDepthVertex(uint vertexID [[vertex_id]])
{
  float2 position = float2((vertexID << 1) & 2, vertexID & 2);
  return float4(position * 2.0 - 1.0, 1.0, 1.0);
}

fragment float4 drawingClearDepthFragment()
{
  return float4(0.0);
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

  // Create the shadow maps. Everything is lit until they have been rendered, so they are cleared with the farthest depth.
  if(!shadowMaps.empty())
  {
    shadowMapBuffer = device->newBuffer(shadowMaps.size() * shadowMapMemorySize, MTL::ResourceCPUCacheModeDefaultCache | MTL::ResourceStorageModeShared);
    std::memset(shadowMapBuffer->contents(), 0, shadowMapBuffer->length());
    MTL::CommandBuffer* clearCommandBuffer = commandQueue->commandBuffer();
    for(std::size_t i = 0; i < shadowMaps.size(); ++i)
    {
      MTL::TextureDescriptor* textureDescriptor = MTL::TextureDescriptor::alloc()->init();
      textureDescriptor->setTextureType(MTL::TextureType2D);
      textureDescriptor->setPixelFormat(MTL::PixelFormatDepth32Float);
      textureDescriptor->setWidth(shadowMaps[i].width);
      textureDescriptor->setHeight(shadowMaps[i].height);
      textureDescriptor->setUsage(MTL::TextureUsageRenderTarget | MTL::TextureUsageShaderRead);
      textureDescriptor->setStorageMode(MTL::StorageModePrivate);
      MTL::Texture* texture = device->newTexture(textureDescriptor);
      textureDescriptor->release();
      ASSERT(texture);
      shadowMapTextures.push_back(texture);

      // The render pass descriptor retains the texture.
      MTL::RenderPassDescriptor* renderPass = MTL::RenderPassDescriptor::alloc()->init();
      auto* depthAttachment = renderPass->depthAttachment();
      depthAttachment->setTexture(texture);
      depthAttachment->setLoadAction(MTL::LoadActionClear);
      depthAttachment->setStoreAction(MTL::StoreActionStore);
      depthAttachment->setClearDepth(1.0);
      shadowMapRenderPasses.push_back(renderPass);

      // The shader looks up the position in the same clip space as the one in which the shadow map is rendered.
      const Matrix4f lightPV = clipSpaceCorrection * shadowMaps[i].projection * shadowMaps[i].view;
      std::uint8_t* memory = static_cast<std::uint8_t*>(shadowMapBuffer->contents()) + i * shadowMapMemorySize;
      std::memcpy(memory, lightPV.data(), sizeof(float) * 16);
      const MTL::ResourceID resourceID = texture->gpuResourceID();
      std::memcpy(memory + shadowMapTextureOffset, &resourceID, sizeof(resourceID));

      clearCommandBuffer->renderCommandEncoder(renderPass)->endEncoding();
    }
    clearCommandBuffer->commit();
    clearCommandBuffer->waitUntilCompleted();
  }

  // Compile shaders.
  std::string lightDeclarationsCode = "#define DECLARE_LIGHTS";
  std::string lightCalculationsCode = "#define CALCULATE_LIGHTS";
  for(std::size_t i = 0; i < lights.size(); ++i)
  {
    const LightData& light = lights[i];
    const std::string name = "light" + std::to_string(i);
    // The direction from the fragment to the light (to determine the shadow bias).
    std::string lightDirection;
    switch(light.type)
    {
      case LightData::directional:
        lightDeclarationsCode += " constant DirLight " + name + " = {" + toMSL(light.color) + ", " + toMSL(light.direction) + "};";
        lightDirection = "metal::normalize(-" + name + ".direction)";
        break;
      case LightData::spot:
        lightDeclarationsCode += " constant SpotLight " + name + " = {" + toMSL(light.color) + ", " + toMSL(light.position) + ", " + std::to_string(light.constantAttenuation) + ", " + std::to_string(light.linearAttenuation) + ", " + std::to_string(light.quadraticAttenuation) + ", " + toMSL(light.direction) + ", " + std::to_string(light.cutoff) + "};";
        lightDirection = "metal::normalize(" + name + ".position - in.positionInWorld)";
        break;
      case LightData::point:
        lightDeclarationsCode += " constant PointLight " + name + " = {" + toMSL(light.color) + ", " + toMSL(light.position) + ", " + std::to_string(light.constantAttenuation) + ", " + std::to_string(light.linearAttenuation) + ", " + std::to_string(light.quadraticAttenuation) + "};";
        lightDirection = "metal::normalize(" + name + ".position - in.positionInWorld)";
        break;
    }
    std::string visibility = "1.0";
    if(light.shadowMap)
    {
      const std::string index = std::to_string(*light.shadowMap);
      visibility = "illumination(shadowMaps[" + index + "].map, shadowMaps[" + index + "].lightPV, in.positionInWorld, " + lightDirection + ", normalizedNormal)";
    }
    const std::string arguments = ", viewDir, albedo, F0, surface.roughness, surface.metallic, " + visibility + ");";
    switch(light.type)
    {
      case LightData::directional:
        lightCalculationsCode += " Lo += calcDirLight(" + name + ", normalizedNormal" + arguments;
        break;
      case LightData::spot:
        lightCalculationsCode += " Lo += calcSpotLight(" + name + ", in.positionInWorld, normalizedNormal" + arguments;
        break;
      case LightData::point:
        lightCalculationsCode += " Lo += calcPointLight(" + name + ", in.positionInWorld, normalizedNormal" + arguments;
        break;
    }
  }
  lightDeclarationsCode += "\n";
  lightCalculationsCode += "\n";

  for(std::size_t i = 0; i < numOfColorShaders; ++i)
  {
    std::string sourceCode = "#define GLOBAL_AMBIENT_LIGHT " + toMSL(globalAmbientLight) + "\n";
    if(i & 4)
    {
      sourceCode += "#define WITH_LIGHTING\n";
      if(!shadowMaps.empty())
        sourceCode += "#define WITH_SHADOWS\n";
    }
    if(i & 2)
      sourceCode += "#define WITH_TEXTURES\n";
    sourceCode += (i & 1) ? "#define NORMAL_QUALIFIER\n" : "#define NORMAL_QUALIFIER [[flat]]\n";
    sourceCode += lightDeclarationsCode + lightCalculationsCode + colorShaderSourceCode;

    if(MTL::Library* library = compileLibrary(sourceCode); library)
    {
      for(std::size_t j = 0; j < numOfSampleCounts; ++j)
        createRenderPipelineStates(library, false, j == 0 ? 1 : multisampleCount, &pipelineStates[(j * numOfShaders + i) * numOfVertexCategories]);
      library->release();
    }
  }
  if(MTL::Library* library = compileLibrary(depthOnlyShaderSourceCode); library)
  {
    createRenderPipelineStates(library, true, 1, &pipelineStates[depthOnlyShaderIndex * numOfVertexCategories]);
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
  depthStencilDescriptor->setDepthWriteEnabled(false);
  noDepthWriteState = device->newDepthStencilState(depthStencilDescriptor);
  depthStencilDescriptor->setDepthWriteEnabled(true);
  depthStencilDescriptor->setDepthCompareFunction(MTL::CompareFunctionAlways);
  clearDepthState = device->newDepthStencilState(depthStencilDescriptor);
  depthStencilDescriptor->release();

  // The pipelines for controller drawings.
  if(MTL::Library* library = compileLibrary(drawingShaderSourceCode); library)
  {
    const auto createPipeline = [this, library](const char* vertexName, const char* fragmentName, MTL::VertexDescriptor* vertexDescriptor, bool writeColor, int sampleCount)
    {
      MTL::RenderPipelineDescriptor* descriptor = MTL::RenderPipelineDescriptor::alloc()->init();
      descriptor->setRasterSampleCount(sampleCount);
      MTL::Function* vertexFunction = library->newFunction(toNSString(vertexName));
      MTL::Function* fragmentFunction = library->newFunction(toNSString(fragmentName));
      descriptor->setVertexFunction(vertexFunction);
      descriptor->setFragmentFunction(fragmentFunction);
      if(vertexDescriptor)
        descriptor->setVertexDescriptor(vertexDescriptor);
      descriptor->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);
      auto* attachment = descriptor->colorAttachments()->object(0);
      attachment->setPixelFormat(MTL::PixelFormatRGBA8Unorm);
      if(writeColor)
      {
        attachment->setBlendingEnabled(true);
        attachment->setSourceRGBBlendFactor(MTL::BlendFactorSourceAlpha);
        attachment->setDestinationRGBBlendFactor(MTL::BlendFactorOneMinusSourceAlpha);
        attachment->setRgbBlendOperation(MTL::BlendOperationAdd);
        // Keep the alpha of the destination.
        attachment->setSourceAlphaBlendFactor(MTL::BlendFactorZero);
        attachment->setDestinationAlphaBlendFactor(MTL::BlendFactorOne);
        attachment->setAlphaBlendOperation(MTL::BlendOperationAdd);
      }
      else
        attachment->setWriteMask(MTL::ColorWriteMaskNone);
      NS::Error* error = nullptr;
      MTL::RenderPipelineState* pipelineState = device->newRenderPipelineState(descriptor, &error);
      if(!pipelineState)
      {
        TRACE("%s", error ? error->localizedDescription()->utf8String() : "Unknown error while creating a Metal render pipeline state.");
        ASSERT(false);
      }
      fragmentFunction->release();
      vertexFunction->release();
      descriptor->release();
      return pipelineState;
    };

    // The vertices of points, lines and triangles.
    MTL::VertexDescriptor* streamDescriptor = MTL::VertexDescriptor::alloc()->init();
    const std::size_t offsets[] = {offsetof(DrawingList::Vertex, position), offsetof(DrawingList::Vertex, color), offsetof(DrawingList::Vertex, size)};
    const MTL::VertexFormat formats[] = {MTL::VertexFormatFloat3, MTL::VertexFormatUChar4Normalized, MTL::VertexFormatFloat};
    for(NS::UInteger i = 0; i < 3; ++i)
    {
      auto* attribute = streamDescriptor->attributes()->object(i);
      attribute->setOffset(offsets[i]);
      attribute->setBufferIndex(0);
      attribute->setFormat(formats[i]);
    }
    auto* layout = streamDescriptor->layouts()->object(0);
    layout->setStepFunction(MTL::VertexStepFunctionPerVertex);
    layout->setStepRate(1);
    layout->setStride(sizeof(DrawingList::Vertex));

    for(std::size_t i = 0; i < numOfSampleCounts; ++i)
    {
      const int sampleCount = i == 0 ? 1 : multisampleCount;
      MTL::RenderPipelineState** states = &drawingPipelineStates[i * numOfDrawingPipelineStates];
      states[streamPipeline] = createPipeline("drawingStreamVertex", "drawingColorFragment", streamDescriptor, true, sampleCount);
      states[flatPipeline] = createPipeline("drawingFlatVertex", "drawingColorFragment", vertexDescriptors[VertexPN::category], true, sampleCount);
      states[imagePipeline] = createPipeline("drawingImageVertex", "drawingImageFragment", vertexDescriptors[VertexPNT::category], true, sampleCount);
      states[clearDepthPipeline] = createPipeline("drawingClearDepthVertex", "drawingClearDepthFragment", nullptr, false, sampleCount);
    }
    streamDescriptor->release();
    library->release();
  }

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
  for(MTL::RenderPipelineState* pipelineState : drawingPipelineStates)
    if(pipelineState)
      pipelineState->release();
  drawingPipelineStates.fill(nullptr);
  if(clearDepthState)
    clearDepthState->release();
  if(noDepthWriteState)
    noDepthWriteState->release();
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
  for(MTL::RenderPassDescriptor* renderPass : shadowMapRenderPasses)
    renderPass->release();
  shadowMapRenderPasses.clear();
  for(MTL::Texture* texture : shadowMapTextures)
    texture->release();
  shadowMapTextures.clear();
  if(shadowMapBuffer)
    shadowMapBuffer->release();
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

  depthStencilState = noDepthWriteState = clearDepthState = nullptr;
  colorReadbackPipelineState = depthReadbackPipelineState = nullptr;
  surfaceBuffer = indexBuffer = vertexBuffer = shadowMapBuffer = nullptr;
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

void GraphicsContext::updateShadowMaps(const std::function<void()>& draw, bool, bool force)
{
  ASSERT(!commandBuffer);
  ASSERT(!encoder);

  if(shadowMaps.empty() || (!force && lastShadowMapUpdate == Simulation::simulation->simulationStep))
    return;
  lastShadowMapUpdate = Simulation::simulation->simulationStep;

  // The shadow maps are rendered with a separate command buffer, which is completed before the function returns. This way,
  // no render pass of an external renderer is interrupted and the shadow maps are ready when they are used.
  NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();
  commandBuffer = commandQueue->commandBuffer();
  renderDepthOnly = true;
  renderingShadowMap = true;
  for(std::size_t i = 0; i < shadowMaps.size(); ++i)
  {
    encoder = commandBuffer->renderCommandEncoder(shadowMapRenderPasses[i]);
    renderTargetHeight = static_cast<int>(shadowMaps[i].height);
    startRendering(shadowMaps[i].projection, shadowMaps[i].view, 0, 0, static_cast<int>(shadowMaps[i].width), renderTargetHeight);
    // Rendering the back faces avoids most self-shadowing artifacts.
    encoder->setCullMode(MTL::CullModeFront);
    draw();
    finishRendering();
    encoder->endEncoding();
    encoder = nullptr;
  }
  commandBuffer->commit();
  commandBuffer->waitUntilCompleted();
  commandBuffer = nullptr;
  renderingShadowMap = false;
  renderDepthOnly = false;
  pool->release();
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
  selectedPipelineStates = &pipelineStates[((renderDepthOnly ? 0 : sampleCountIndex * numOfShaders) + (renderDepthOnly ? depthOnlyShaderIndex : ((lighting ? 4 : 0) + (textures ? 2 : 0) + (smoothShading ? 1 : 0)))) * numOfVertexCategories];
  boundPipelineState = nullptr;

  if(viewportX >= 0)
  {
    // The origin of viewports is at the top left in Metal but at the bottom left in OpenGL.
    ASSERT(offscreenTarget || renderingShadowMap);
    MTL::Viewport viewport;
    viewport.originX = viewportX;
    viewport.originY = renderTargetHeight - viewportY - viewportHeight;
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
    if(lighting && shadowMapBuffer)
    {
      encoder->setFragmentBuffer(shadowMapBuffer, 0, 3);
      for(MTL::Texture* texture : shadowMapTextures)
        encoder->useResource(texture, MTL::ResourceUsageRead, MTL::RenderStageFragment);
    }
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
  renderTargetHeight = height;
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
  ASSERT(h <= renderTargetHeight);

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

bool GraphicsContext::startExternalRendering(const QRhiNativeHandles* nativeHandles, int sampleCount)
{
  ASSERT(sampleCount == 1 || sampleCount == multisampleCount);
  ASSERT(!commandBuffer);
  ASSERT(!encoder);
  ASSERT(!offscreenTarget);
  const auto* commandBufferHandles = static_cast<const QRhiMetalCommandBufferNativeHandles*>(nativeHandles);
  if(!commandBufferHandles || !commandBufferHandles->encoder)
    return false;
  commandBuffer = reinterpret_cast<MTL::CommandBuffer*>(commandBufferHandles->commandBuffer);
  encoder = reinterpret_cast<MTL::RenderCommandEncoder*>(commandBufferHandles->encoder);
  renderDepthOnly = false;
  sampleCountIndex = sampleCount == 1 ? 0 : 1;

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
  sampleCountIndex = 0;
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

void GraphicsContext::createRenderPipelineStates(MTL::Library* library, bool depthOnly, int sampleCount, MTL::RenderPipelineState** pipelineStates)
{
  MTL::RenderPipelineDescriptor* descriptor = MTL::RenderPipelineDescriptor::alloc()->init();
  descriptor->setRasterSampleCount(sampleCount);
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

void GraphicsContext::prepareDrawings(const DrawingList& drawings)
{
  ASSERT(commandBuffer);
  ASSERT(encoder);
  ASSERT(!selectedPipelineStates);
  ASSERT(!drawingBuffer && drawingTextures.empty());

  // The vertices are stored one after the other: lines, points, triangles.
  const std::size_t numOfVertices = drawings.lines.size() + drawings.points.size() + drawings.triangles.size();
  if(numOfVertices)
  {
    drawingBuffer = device->newBuffer(numOfVertices * sizeof(DrawingList::Vertex), MTL::ResourceCPUCacheModeDefaultCache | MTL::ResourceStorageModeShared);
    std::uint8_t* memory = static_cast<std::uint8_t*>(drawingBuffer->contents());
    for(const auto* vertices : {&drawings.lines, &drawings.points, &drawings.triangles})
    {
      std::memcpy(memory, vertices->data(), vertices->size() * sizeof(DrawingList::Vertex));
      memory += vertices->size() * sizeof(DrawingList::Vertex);
    }
  }

  std::vector<std::uint8_t> rgba;
  for(const DrawingList::Image& image : drawings.images)
  {
    MTL::TextureDescriptor* textureDescriptor = MTL::TextureDescriptor::texture2DDescriptor(MTL::PixelFormatRGBA8Unorm, image.width, image.height, false);
    textureDescriptor->setUsage(MTL::TextureUsageShaderRead);
    MTL::Texture* texture = device->newTexture(textureDescriptor);
    ASSERT(texture);
    rgba.resize(static_cast<std::size_t>(image.width) * image.height * 4);
    for(std::size_t i = 0; i < static_cast<std::size_t>(image.width) * image.height; ++i)
    {
      rgba[i * 4] = image.rgb[i * 3];
      rgba[i * 4 + 1] = image.rgb[i * 3 + 1];
      rgba[i * 4 + 2] = image.rgb[i * 3 + 2];
      rgba[i * 4 + 3] = 255;
    }
    texture->replaceRegion(MTL::Region(0, 0, image.width, image.height), 0, rgba.data(), image.width * 4);
    drawingTextures.push_back(texture);
  }
}

void GraphicsContext::drawDrawings(const DrawingList& drawings, const Matrix4f& projection, const Matrix4f& view, float alpha, bool opaquePass, bool fillPolygons)
{
  ASSERT(commandBuffer);
  ASSERT(encoder);
  ASSERT(!selectedPipelineStates);
  ASSERT(drawingSphereMesh && drawingQuadMesh);

  const Matrix4f pv = clipSpaceCorrection * projection * view;
  const std::size_t firstPoint = drawings.lines.size();
  const std::size_t firstTriangle = drawings.lines.size() + drawings.points.size();

  encoder->setFrontFacingWinding(MTL::WindingCounterClockwise);
  encoder->setCullMode(MTL::CullModeBack);
  encoder->setTriangleFillMode(fillPolygons ? MTL::TriangleFillModeFill : MTL::TriangleFillModeLines);
  const MTL::RenderPipelineState* boundState = nullptr;
  const auto bindPipeline = [this, &boundState](DrawingPipeline pipeline)
  {
    MTL::RenderPipelineState* pipelineState = drawingPipelineStates[sampleCountIndex * numOfDrawingPipelineStates + pipeline];
    if(pipelineState != boundState)
      encoder->setRenderPipelineState((boundState = pipelineState));
  };

  for(const DrawingList::Item& item : drawings.items)
  {
    if(item.renderOptions & (SimRobotCore3::Drawing::disableTransparency | SimRobotCore3::Drawing::disableOpacity))
    {
      if(opaquePass != ((item.renderOptions & SimRobotCore3::Drawing::disableTransparency) != 0))
        continue;
    }
    encoder->setDepthStencilState((item.renderOptions & SimRobotCore3::Drawing::disableDepth) ? noDepthWriteState : depthStencilState);

    const Matrix4f pvm = pv * item.transformation;

    if(item.numOfPoints || item.numOfLines || item.numOfTriangles)
    {
      bindPipeline(streamPipeline);
      encoder->setVertexBuffer(drawingBuffer, 0, 0);
      encoder->setVertexBytes(pvm.data(), sizeof(Matrix4f), 1);
      encoder->setVertexBytes(&alpha, sizeof(alpha), 2);
      if(item.numOfLines)
        encoder->drawPrimitives(MTL::PrimitiveTypeLine, item.firstLine, item.numOfLines);
      if(item.numOfPoints)
        encoder->drawPrimitives(MTL::PrimitiveTypePoint, firstPoint + item.firstPoint, item.numOfPoints);
      if(item.numOfTriangles)
        encoder->drawPrimitives(MTL::PrimitiveTypeTriangle, firstTriangle + item.firstTriangle, item.numOfTriangles);
    }

    if(item.numOfInstances)
    {
      bindPipeline(flatPipeline);
      encoder->setVertexBuffer(vertexBuffer, 0, 0);
      encoder->setVertexBytes(&alpha, sizeof(alpha), 3);
      for(std::size_t i = item.firstInstance; i < item.firstInstance + item.numOfInstances; ++i)
      {
        const DrawingList::Instance& instance = drawings.instances[i];
        const Matrix4f instancePVM = pvm * instance.transformation;
        const float color[4] = {instance.color[0] / 255.f, instance.color[1] / 255.f, instance.color[2] / 255.f, instance.color[3] / 255.f};
        encoder->setVertexBytes(instancePVM.data(), sizeof(Matrix4f), 1);
        encoder->setVertexBytes(color, sizeof(color), 2);
        encoder->drawIndexedPrimitives(getPrimitiveType(drawingSphereMesh->topology), drawingSphereMesh->indexBuffer->count, MTL::IndexTypeUInt32, indexBuffer, drawingSphereMesh->indexBuffer->offset, 1, drawingSphereMesh->vertexBuffer->base, 0);
      }
    }

    if(item.numOfImages)
    {
      bindPipeline(imagePipeline);
      encoder->setVertexBuffer(vertexBuffer, 0, 0);
      encoder->setFragmentBytes(&alpha, sizeof(alpha), 0);
      encoder->setCullMode(MTL::CullModeNone);
      for(std::size_t i = item.firstImage; i < item.firstImage + item.numOfImages; ++i)
      {
        const Matrix4f imagePVM = pvm * drawings.images[i].transformation;
        encoder->setVertexBytes(imagePVM.data(), sizeof(Matrix4f), 1);
        encoder->setFragmentTexture(drawingTextures[i], 0);
        encoder->drawIndexedPrimitives(getPrimitiveType(drawingQuadMesh->topology), drawingQuadMesh->indexBuffer->count, MTL::IndexTypeUInt32, indexBuffer, drawingQuadMesh->indexBuffer->offset, 1, drawingQuadMesh->vertexBuffer->base, 0);
      }
      encoder->setCullMode(MTL::CullModeBack);
    }
  }

  // Restore the state that the other rendering expects.
  encoder->setTriangleFillMode(MTL::TriangleFillModeFill);
  encoder->setDepthStencilState(depthStencilState);
}

void GraphicsContext::finishDrawings()
{
  ASSERT(commandBuffer);
  // The GPU uses the resources after the commands have been encoded, so they are released when the command buffer has been completed.
  MTL::Buffer* buffer = drawingBuffer;
  std::vector<MTL::Texture*> textures = std::move(drawingTextures);
  drawingBuffer = nullptr;
  drawingTextures.clear();
  if(buffer || !textures.empty())
    commandBuffer->addCompletedHandler([buffer, textures](MTL::CommandBuffer*)
    {
      if(buffer)
        buffer->release();
      for(MTL::Texture* texture : textures)
        texture->release();
    });
}

void GraphicsContext::clearDepth()
{
  ASSERT(commandBuffer);
  ASSERT(encoder);
  ASSERT(!selectedPipelineStates);
  encoder->setCullMode(MTL::CullModeNone);
  encoder->setDepthStencilState(clearDepthState);
  encoder->setRenderPipelineState(drawingPipelineStates[sampleCountIndex * numOfDrawingPipelineStates + clearDepthPipeline]);
  encoder->drawPrimitives(MTL::PrimitiveTypeTriangle, static_cast<NS::UInteger>(0), static_cast<NS::UInteger>(3));
  encoder->setCullMode(MTL::CullModeBack);
  encoder->setDepthStencilState(depthStencilState);
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
