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
#include "Graphics/SensorShader.h"
#include "Graphics/SkyShader.h"
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
  uint surfaceIndex [[flat]];
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

// Must match GraphicsContext::surfaceMemorySize and the offsets of the members of a surface.
struct Surface
{
  float4 albedoAlpha;
  float metallic;
  float roughness;
  float ambient;
  float textureVariation;
  float4 sheen; // The color and the roughness of the sheen.
  metal::texture2d<float> texture;
  metal::texture2d<float> normalMap;
  metal::texture2d<float> roughnessMap;
  float normalScale;
};

// Must match GraphicsContext::DrawList::Instance. The instance_id includes the base instance of the draw call.
struct InstanceData
{
  uint modelMatrix;
  uint surface;
};

vertex VertexOut vertexMain(VertexIn in [[stage_in]], uint instanceID [[instance_id]],
                            constant metal::float4x4& cameraPV [[buffer(1)]], device const metal::float4x4* modelMatrices [[buffer(2)]],
                            device const InstanceData* instances [[buffer(4)]], constant uint& forcedSurface [[buffer(5)]])
{
  const InstanceData instance = instances[instanceID];
  const metal::float4x4 modelMatrix = modelMatrices[instance.modelMatrix];
  VertexOut out;
  out.surfaceIndex = forcedSurface != 0xffffffffu ? forcedSurface : instance.surface;
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
// The offsets (in texels) of the lookups into the shadow map: a spiral that covers a disk with a radius of 1.5 texels.
constant float2 shadowFilter[12] = {float2(0.30619, 0.00000), float2(-0.39105, 0.35823), float2(0.05986, -0.68203), float2(0.49289, 0.64289), float2(-0.90452, -0.16000), float2(0.85684, -0.54505), float2(-0.28660, 1.06612), float2(-0.54657, -1.05238), float2(1.18584, 0.43307), float2(-1.23366, 0.50924), float2(0.59471, -1.27086), float2(0.43947, 1.40111)};

// Returns the fraction of the fragment that is lit. The shadow map is sampled with hardware percentage closer filtering
// (a bilinear comparison) at twelve positions that together cover about 5x5 texels. This is a port of the GLSL function, but
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
  for(int i = 0; i < 12; ++i)
    visibility += shadowMap.sample_compare(shadowSampler, position + shadowFilter[i] * texel, reference, metal::level(0));
  return visibility / 12.0;
}
#endif

// The sheen of fibers (cloth, turf): the lobe of Charlie with the visibility term of Neubelt and Pettineo (see the glTF
// extension KHR_materials_sheen), which is brightest at grazing angles. The roughness of the sheen is in the fourth component.
float3 calcSheen(float3 lightDir, float3 normal, float3 viewDir, float4 sheen)
{
  float3 halfDir = metal::normalize(viewDir + lightDir);
  float cosView = metal::max(metal::dot(normal, viewDir), 0.0);
  float cosLight = metal::max(metal::dot(normal, lightDir), 0.0);
  float cosHalf = metal::clamp(metal::dot(normal, halfDir), 0.0, 1.0);
  float inverseAlpha = 1.0 / metal::max(sheen.w * sheen.w, 0.0625);
  float sinHalf = metal::sqrt(metal::max(1.0 - cosHalf * cosHalf, 0.0001));
  float distribution = (2.0 + inverseAlpha) * metal::pow(sinHalf, inverseAlpha) / (2.0 * PI);
  float visibility = 1.0 / (4.0 * (cosLight + cosView - cosLight * cosView) + 0.0001);
  return sheen.rgb * distribution * visibility;
}

float3 calcLight(float3 irradiance, float3 lightDir, float3 normal, float3 viewDir, float3 albedo, float3 F0, float roughness, float metallic, float4 sheen, float visibility)
{
  float cosTheta = metal::max(metal::dot(normal, lightDir), 0.0);
  float3 brdf = calcBrdf(lightDir, normal, viewDir, albedo, F0, roughness, metallic);
  if(sheen.r + sheen.g + sheen.b > 0.0)
    brdf += calcSheen(lightDir, normal, viewDir, sheen);
  return brdf * irradiance * cosTheta * visibility;
}

float3 calcDirLight(DirLight light, float3 normal, float3 viewDir, float3 albedo, float3 F0, float roughness, float metallic, float4 sheen, float visibility)
{
  float3 lightDir = metal::normalize(-light.direction);
  float3 irradiance = light.color;

  return calcLight(irradiance, lightDir, normal, viewDir, albedo, F0, roughness, metallic, sheen, visibility);
}

float3 calcPointLight(PointLight light, float3 pos, float3 normal, float3 viewDir, float3 albedo, float3 F0, float roughness, float metallic, float4 sheen, float visibility)
{
  float3 lightDir = metal::normalize(light.position - pos);
  float distance = metal::length(light.position - pos);
  float attenuation = 1.0 / (light.constantAttenuation + light.linearAttenuation * distance + light.quadraticAttenuation * distance * distance);
  float3 irradiance = light.color * attenuation;

  return calcLight(irradiance, lightDir, normal, viewDir, albedo, F0, roughness, metallic, sheen, visibility);
}

float3 calcSpotLight(SpotLight light, float3 pos, float3 normal, float3 viewDir, float3 albedo, float3 F0, float roughness, float metallic, float4 sheen, float visibility)
{
  float3 lightDir = metal::normalize(light.position - pos);
  float distance = metal::length(light.position - pos);
  float attenuation = 1.0 / (light.constantAttenuation + light.linearAttenuation * distance + light.quadraticAttenuation * distance * distance);
  float cosTheta = metal::dot(lightDir, metal::normalize(-light.direction));
  float intensity = metal::clamp((cosTheta - light.cutoff) / (1 - light.cutoff), 0.0, 1.0);
  float3 irradiance = light.color * attenuation * intensity;

  return calcLight(irradiance, lightDir, normal, viewDir, albedo, F0, roughness, metallic, sheen, visibility);
}

DECLARE_LIGHTS
#endif

fragment float4 fragmentMain(VertexOut in [[stage_in]], device const Surface* surfaces [[buffer(0)]], constant float4& cameraPos [[buffer(1)]]
#ifdef WITH_SHADOWS
                             , device const ShadowMapData* shadowMaps [[buffer(3)]]
#endif
#ifdef WITH_SKY_LIGHT
                             , constant float4* skyLight [[buffer(6)]]
#endif
#ifdef WITH_BACKDROP
                             , metal::texturecube<float> environment [[texture(0)]]
#endif
                            )
{
#ifdef WITH_TEXTURES
  constexpr metal::sampler textureSampler(metal::filter::linear, metal::mip_filter::linear, metal::address::repeat, metal::max_anisotropy(2));
#endif
  device const Surface& surface = surfaces[in.surfaceIndex];
  float4 color;
#ifdef WITH_LIGHTING
  float3 normalizedNormal = metal::normalize(in.normalInWorld);
  float3 viewDir = metal::normalize(cameraPos.xyz - in.positionInWorld);

  float3 albedo = surface.albedoAlpha.rgb;
  color.a = surface.albedoAlpha.a;
  float roughness = surface.roughness;
  float4 sheen = surface.sheen;
#ifdef WITH_TEXTURES
  if(!metal::is_null_texture(surface.texture))
  {
    float4 modulation = surface.texture.sample(textureSampler, in.textureCoordinates);
    if(surface.textureVariation > 0.0)
    {
      // Hides the repetition of the texture: A second layer of the texture with another scale and orientation is added (with the
      // mean color subtracted, so that the contrast stays the same), and large patches are slightly brighter or darker.
      float3 mean = surface.texture.sample(textureSampler, in.textureCoordinates, metal::level(16)).rgb;
      float3 second = surface.texture.sample(textureSampler, metal::float2x2(0.5657, 0.4243, -0.4243, 0.5657) * in.textureCoordinates + float2(0.37, 0.19)).rgb;
      float3 layers = mean + (modulation.rgb + second - 2.0 * mean) * 0.7071;
      float2 cell = in.textureCoordinates * 0.11;
      float2 f = metal::fract(cell);
      f = f * f * (3.0 - 2.0 * f);
      float2 i = metal::floor(cell);
      float4 h = metal::fract(metal::sin(float4(metal::dot(i, float2(12.9898, 78.233)), metal::dot(i + float2(1.0, 0.0), float2(12.9898, 78.233)), metal::dot(i + float2(0.0, 1.0), float2(12.9898, 78.233)), metal::dot(i + float2(1.0, 1.0), float2(12.9898, 78.233)))) * 43758.5453);
      float macro = metal::mix(metal::mix(h.x, h.y, f.x), metal::mix(h.z, h.w, f.x), f.y);
      modulation.rgb = metal::max(layers * (1.0 + 0.35 * surface.textureVariation * (macro - 0.5)), 0.0);
    }
    albedo = albedo * modulation.rgb;
    color.a = color.a * modulation.a;
  }
  if(!metal::is_null_texture(surface.normalMap))
  {
    // The x and y components of the normal in tangent space (the second layer, which is needed to hide the repetition, is rotated).
    float2 slope = surface.normalMap.sample(textureSampler, in.textureCoordinates).xy * 2.0 - 1.0;
    if(surface.textureVariation > 0.0)
    {
      float2 second = surface.normalMap.sample(textureSampler, metal::float2x2(0.5657, 0.4243, -0.4243, 0.5657) * in.textureCoordinates + float2(0.37, 0.19)).xy * 2.0 - 1.0;
      slope = (slope + float2(0.8 * second.x + 0.6 * second.y, -0.6 * second.x + 0.8 * second.y)) * 0.7071;
    }
    slope *= surface.normalScale;
    float3 tangentNormal = float3(slope, metal::sqrt(metal::max(1.0 - metal::dot(slope, slope), 0.0001)));
    // The tangent frame comes from the derivatives of the position and the texture coordinates.
    float3 positionDx = metal::dfdx(in.positionInWorld);
    float3 positionDy = metal::dfdy(in.positionInWorld);
    float2 textureDx = metal::dfdx(in.textureCoordinates);
    float2 textureDy = metal::dfdy(in.textureCoordinates);
    float3 perpendicularY = metal::cross(positionDy, normalizedNormal);
    float3 perpendicularX = metal::cross(normalizedNormal, positionDx);
    float3 tangent = perpendicularY * textureDx.x + perpendicularX * textureDy.x;
    float3 bitangent = perpendicularY * textureDx.y + perpendicularX * textureDy.y;
    float scale = metal::rsqrt(metal::max(metal::max(metal::dot(tangent, tangent), metal::dot(bitangent, bitangent)), 1e-20));
    normalizedNormal = metal::normalize(tangent * scale * tangentNormal.x + bitangent * scale * tangentNormal.y + normalizedNormal * tangentNormal.z);
  }
  if(!metal::is_null_texture(surface.roughnessMap))
    roughness = metal::clamp(roughness * surface.roughnessMap.sample(textureSampler, in.textureCoordinates).r, 0.0, 1.0);
#endif
  albedo = metal::pow(albedo, float3(2.2)); // convert from sRGB to linear
  float3 F0 = float3(0.04);
  F0 = metal::mix(F0, albedo, surface.metallic);

  float3 Lo = float3(0.0);
  CALCULATE_LIGHTS
  // The ambient light comes from above (sky) and from below (ground) and is interpolated by the direction of the normal.
#ifdef WITH_BACKDROP
  // The light of the backdrop: the diffuse part comes from the irradiance (spherical harmonics), the specular part from the
  // prefiltered radiance in the direction of the reflection with the approximation of the BRDF of Karis.
  constexpr metal::sampler environmentSampler(metal::filter::linear, metal::mip_filter::linear, metal::address::clamp_to_edge);
  float3 ambientLight = backdropIrradiance(normalizedNormal);
  float cosView = metal::max(metal::dot(normalizedNormal, viewDir), 0.0);
  float4 brdfRoughness = roughness * float4(-1.0, -0.0275, -0.572, 0.022) + float4(1.0, 0.0425, 1.04, -0.04);
  float brdfBias = metal::min(brdfRoughness.x * brdfRoughness.x, metal::exp2(-9.28 * cosView)) * brdfRoughness.x + brdfRoughness.y;
  float2 brdfScaleBias = float2(-1.04, 1.04) * brdfBias + brdfRoughness.zw;
  float3 specularAlbedo = F0 * brdfScaleBias.x + brdfScaleBias.y;
  float3 prefiltered = environment.sample(environmentSampler, metal::reflect(-viewDir, normalizedNormal), metal::level(roughness * (ENVIRONMENT_LEVELS - 1.0))).rgb;
  float3 ambient = (ambientLight * albedo * (float3(1.0) - specularAlbedo) * (1.0 - surface.metallic) + prefiltered * specularAlbedo) * surface.ambient;
#else
  float3 ambientLight = metal::mix(GLOBAL_AMBIENT_LIGHT_GROUND, GLOBAL_AMBIENT_LIGHT, 0.5 + 0.5 * normalizedNormal.z);
  float3 ambient = ambientLight * albedo * surface.ambient;
#endif
  if(sheen.r + sheen.g + sheen.b > 0.0)
    ambient += sheen.rgb * ambientLight * metal::pow(1.0 - metal::max(metal::dot(normalizedNormal, viewDir), 0.0), 1.0 / metal::max(sheen.w * sheen.w, 0.0625)) * surface.ambient;
  color.rgb = (ambient + Lo) * EXPOSURE;
#ifdef FILMIC_TONE_MAPPING
  color.rgb = metal::clamp((color.rgb * (2.51 * color.rgb + 0.03)) / (color.rgb * (2.43 * color.rgb + 0.59) + 0.14), 0.0, 1.0); // ACES approximation
#else
  color.rgb = color.rgb / (color.rgb + float3(1.0)); // Reinhard tone mapping
#endif
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

struct InstanceIn
{
  float3 position [[attribute(0)]];
};

struct Instance
{
  metal::float4x4 transformation;
  float2 radii;
  uchar4 color;
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

vertex ColorIn drawingInstanceVertex(InstanceIn in [[stage_in]], uint instanceID [[instance_id]], device const Instance* instances [[buffer(1)]], constant metal::float4x4& pvm [[buffer(2)]], constant float& alphaScale [[buffer(3)]])
{
  const device Instance& instance = instances[instanceID];
  // The vertices below and above the xy-plane are scaled by different radii (which makes cones from a cylinder).
  float radius = in.position.z < 0.0 ? instance.radii.x : instance.radii.y;
  ColorIn out;
  out.position = pvm * instance.transformation * float4(in.position.xy * radius, in.position.z, 1.0);
  float4 color = float4(instance.color) / 255.0;
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
  float4 color = image.sample(imageSampler, in.textureCoordinates);
  if(color.a < 0.02)
    metal::discard_fragment(); // transparent parts must not write the depth buffer
  return float4(color.rgb, color.a * alphaScale);
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

static_assert(sizeof(GraphicsContextBase::PreparedDrawing::Instance) == 80, "The instances must have the layout of the struct Instance in the drawing shaders.");

static const char* depthOnlyShaderSourceCode = R"metal(
struct VertexIn
{
  float3 positionInModel [[attribute(0)]];
};

struct InstanceData
{
  uint modelMatrix;
  uint surface;
};

vertex float4 vertexMain(VertexIn in [[stage_in]], uint instanceID [[instance_id]],
                         constant metal::float4x4& cameraPV [[buffer(1)]], device const metal::float4x4* modelMatrices [[buffer(2)]],
                         device const InstanceData* instances [[buffer(4)]])
{
  return cameraPV * modelMatrices[instances[instanceID].modelMatrix] * float4(in.positionInModel, 1.0);
}
)metal";

// The shader for depth-only renderings of instances with textures with alpha: it discards the fragments that are transparent.
static const char* depthOnlyAlphaTestShaderSourceCode = R"metal(
struct VertexIn
{
  float3 positionInModel [[attribute(0)]];
  float3 normalInModel [[attribute(1)]];
  float2 textureCoordinates [[attribute(2)]];
};

struct VertexOut
{
  float4 positionInClip [[position]];
  float2 textureCoordinates;
  uint surfaceIndex [[flat]];
};

struct InstanceData
{
  uint modelMatrix;
  uint surface;
};

// Must match GraphicsContext::surfaceMemorySize and the offsets of the members of a surface.
struct Surface
{
  float4 albedoAlpha;
  float metallic;
  float roughness;
  float ambient;
  float textureVariation;
  float4 sheen; // The color and the roughness of the sheen.
  metal::texture2d<float> texture;
  metal::texture2d<float> normalMap;
  metal::texture2d<float> roughnessMap;
  float normalScale;
};

vertex VertexOut vertexMain(VertexIn in [[stage_in]], uint instanceID [[instance_id]],
                            constant metal::float4x4& cameraPV [[buffer(1)]], device const metal::float4x4* modelMatrices [[buffer(2)]],
                            device const InstanceData* instances [[buffer(4)]])
{
  const InstanceData instance = instances[instanceID];
  VertexOut out;
  out.positionInClip = cameraPV * modelMatrices[instance.modelMatrix] * float4(in.positionInModel, 1.0);
  out.textureCoordinates = in.textureCoordinates;
  out.surfaceIndex = instance.surface;
  return out;
}

fragment void fragmentMain(VertexOut in [[stage_in]], device const Surface* surfaces [[buffer(0)]])
{
  constexpr metal::sampler textureSampler(metal::filter::linear, metal::mip_filter::linear, metal::address::repeat, metal::max_anisotropy(2));
  device const Surface& surface = surfaces[in.surfaceIndex];
  if(!metal::is_null_texture(surface.texture) && surface.texture.sample(textureSampler, in.textureCoordinates).a * surface.albedoAlpha.a < 0.5)
    metal::discard_fragment();
}
)metal";

// These kernels convert a part of a render target into a buffer, with the top row first.
static const std::string readbackShaderSourceCode = std::string(R"metal(
using namespace metal;
struct ReadbackParameters
{
  uint width; // The width of the image.
  uint height; // The height of the image.
  uint firstRow; // The row of the render target with the top row of the image.
  uint seed; // The seed of the noise.
  float depthNumerator; // The conversion of depths d into distances: depthNumerator / (d + depthOffset).
  float depthOffset;
  uint modeled; // Whether the properties of the sensor are applied (otherwise the following values are ignored).
  float blur;
  float gainR;
  float gainG;
  float gainB;
  float saturation;
  float vignette;
  float noise;
};
)metal") + sensorShaderSharedCode + R"metal(
// Reads the color of the pixel at the position (counted from the top left of the image) with the properties of the sensor applied.
float3 readColor(texture2d<float, access::read> inTexture, constant ReadbackParameters& parameters, uint2 position)
{
  if(!parameters.modeled)
    return saturate(inTexture.read(uint2(position.x, parameters.firstRow + position.y)).rgb);
  float3 color;
  if(parameters.blur > 0.0)
  {
    // A Gaussian kernel of 3 x 3 pixels (the weights of the edge and the corner neighbors are given relative to the center).
    float edge = exp(-0.5 / (parameters.blur * parameters.blur));
    float corner = edge * edge;
    float3 sum = float3(0.0);
    for(int dy = -1; dy <= 1; ++dy)
      for(int dx = -1; dx <= 1; ++dx)
      {
        uint2 neighbor = uint2(clamp(int(position.x) + dx, 0, int(parameters.width) - 1), clamp(int(position.y) + dy, 0, int(parameters.height) - 1));
        // The corners are only read if their weight is noticeable.
        if(dx == 0 || dy == 0 || corner > 0.04)
          sum += (dx == 0 && dy == 0 ? 1.0 : dx == 0 || dy == 0 ? edge : corner) * inTexture.read(uint2(neighbor.x, parameters.firstRow + neighbor.y)).rgb;
      }
    if(corner <= 0.04)
      corner = 0.0;
    color = sum / (1.0 + 4.0 * edge + 4.0 * corner);
  }
  else
    color = inTexture.read(uint2(position.x, parameters.firstRow + position.y)).rgb;
  return saturate(applySensorModel(color, position, float2(parameters.width, parameters.height), parameters.seed, float3(parameters.gainR, parameters.gainG, parameters.gainB), parameters.saturation, parameters.vignette, parameters.noise));
}

uint3 readRGB(texture2d<float, access::read> inTexture, constant ReadbackParameters& parameters, uint2 position)
{
  return uint3(rint(readColor(inTexture, parameters, position) * 255.0));
}

kernel void readbackRGB(metal::texture2d<float, metal::access::read> inTexture [[texture(0)]],
                        device uchar* outBuffer [[buffer(0)]],
                        constant ReadbackParameters& parameters [[buffer(1)]],
                        uint2 gid [[thread_position_in_grid]])
{
  uint3 rgb = readRGB(inTexture, parameters, gid);
  uint index = (gid.y * parameters.width + gid.x) * 3;
  outBuffer[index] = uchar(rgb.r);
  outBuffer[index + 1] = uchar(rgb.g);
  outBuffer[index + 2] = uchar(rgb.b);
}

// Each thread converts two pixels. The coefficients are those of BT.601 with 14 fractional bits, U and V are averaged.
kernel void readbackYUYV(metal::texture2d<float, metal::access::read> inTexture [[texture(0)]],
                         device uchar* outBuffer [[buffer(0)]],
                         constant ReadbackParameters& parameters [[buffer(1)]],
                         uint2 gid [[thread_position_in_grid]])
{
  int3 rgb0 = int3(readRGB(inTexture, parameters, uint2(gid.x * 2, gid.y)));
  int3 rgb1 = int3(readRGB(inTexture, parameters, uint2(gid.x * 2 + 1, gid.y)));
  int y0 = (rgb0.r * 4898 + rgb0.g * 9617 + rgb0.b * 1867) >> 14;
  int y1 = (rgb1.r * 4898 + rgb1.g * 9617 + rgb1.b * 1867) >> 14;
  // The offset of 256 keeps the shifted values positive, so that they are rounded down.
  int u = (((rgb0.b - y0 + rgb1.b - y1) * 9240 + (256 << 15)) >> 15) - 128;
  int v = (((rgb0.r - y0 + rgb1.r - y1) * 11681 + (256 << 15)) >> 15) - 128;
  uint index = (gid.y * parameters.width + gid.x * 2) * 2;
  outBuffer[index] = uchar(metal::clamp(y0, 0, 255));
  outBuffer[index + 1] = uchar(metal::clamp(u, 0, 255));
  outBuffer[index + 2] = uchar(metal::clamp(y1, 0, 255));
  outBuffer[index + 3] = uchar(metal::clamp(v, 0, 255));
}

kernel void readbackDepth(metal::depth2d<float, metal::access::read> inTexture [[texture(0)]],
                          device float* outBuffer [[buffer(0)]],
                          constant ReadbackParameters& parameters [[buffer(1)]],
                          uint2 gid [[thread_position_in_grid]])
{
  float depth = inTexture.read(uint2(gid.x, parameters.firstRow + gid.y));
  outBuffer[gid.y * parameters.width + gid.x] = parameters.depthNumerator / (depth + parameters.depthOffset);
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

  const auto createDescriptor = [](std::size_t stride, const std::array<std::size_t, 3>& offsets, const std::array<MTL::VertexFormat, 3>& formats)
  {
    MTL::VertexDescriptor* descriptor = MTL::VertexDescriptor::alloc()->init();
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

  // Both vertex types provide all attributes. VertexPN reuses the position as texture coordinates (they are never used).
  vertexDescriptors[VertexPN::category] = createDescriptor(VertexPN::size, {0, 3 * sizeof(float), 0}, {MTL::VertexFormatFloat3, MTL::VertexFormatFloat3, MTL::VertexFormatFloat2});
  vertexDescriptors[VertexPNT::category] = createDescriptor(VertexPNT::size, {0, 3 * sizeof(float), 6 * sizeof(float)}, {MTL::VertexFormatFloat3, MTL::VertexFormatFloat3, MTL::VertexFormatFloat2});
  drawingVertexDescriptor = createDescriptor(sizeof(PreparedDrawing::Vertex), {offsetof(PreparedDrawing::Vertex, position), offsetof(PreparedDrawing::Vertex, color), offsetof(PreparedDrawing::Vertex, size)},
                                             {MTL::VertexFormatFloat3, MTL::VertexFormatUChar4Normalized, MTL::VertexFormatFloat});

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
  drawingVertexDescriptor->release();
}

bool GraphicsContext::compile(std::list<std::string>&)
{
  layoutResources();

  ASSERT(!device);
  device = MTL::CreateSystemDefaultDevice();
  ASSERT(device);

  createDeviceResources();
  return true;
}

void GraphicsContext::createDeviceResources()
{
  NS::AutoreleasePool* pool = NS::AutoreleasePool::alloc()->init();

  commandQueue = device->newCommandQueue();

  // The readback buffers are shared with the CPU, which reads them directly.
  mtlReadbackBuffers.reserve(readbackBuffers.size());
  for(ReadbackBuffer* buffer : readbackBuffers)
  {
    MTL::Buffer* mtlBuffer = mtlReadbackBuffers.emplace_back(device->newBuffer(buffer->size, MTL::ResourceCPUCacheModeDefaultCache | MTL::ResourceStorageModeShared));
    ASSERT(mtlBuffer);
    buffer->memory = static_cast<unsigned char*>(mtlBuffer->contents());
  }

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

  // Create the cube map of the backdrop (the memory is shared with the CPU, so the data can be copied right away).
  if(environmentMap)
  {
    MTL::TextureDescriptor* descriptor = MTL::TextureDescriptor::alloc()->init();
    descriptor->setTextureType(MTL::TextureTypeCube);
    descriptor->setPixelFormat(MTL::PixelFormatRGBA16Float);
    descriptor->setWidth(environmentMap->resolution);
    descriptor->setHeight(environmentMap->resolution);
    descriptor->setMipmapLevelCount(environmentMap->levels);
    descriptor->setUsage(MTL::TextureUsageShaderRead);
    descriptor->setStorageMode(MTL::StorageModeShared);
    environmentTexture = device->newTexture(descriptor);
    descriptor->release();
    ASSERT(environmentTexture);
    for(unsigned int level = 0; level < environmentMap->levels; ++level)
    {
      const unsigned int size = environmentMap->size(level);
      for(unsigned int side = 0; side < 6; ++side)
        environmentTexture->replaceRegion(MTL::Region(0, 0, size, size), level, side, environmentMap->sides[level * 6 + side].data(), size * 8, 0);
    }
  }

  // Upload surfaces.
  modelMatrixBuffer = nullptr;
  uploadedModelMatrixGeneration = 0;

  surfaceBuffer = device->newBuffer(std::max<std::size_t>(surfaces.size(), 1) * surfaceMemorySize, MTL::ResourceCPUCacheModeDefaultCache | MTL::ResourceStorageModeShared);
  std::memset(surfaceBuffer->contents(), 0, surfaceBuffer->length());
  for(const Surface* surface : surfaces)
  {
    static constexpr std::size_t verbatimPart = offsetof(Surface, ambient) - offsetof(Surface, albedo) + sizeof(Surface::ambient);
    std::uint8_t* memory = static_cast<std::uint8_t*>(surfaceBuffer->contents()) + surface->index * surfaceMemorySize;
    std::memcpy(memory, &surface->albedo, verbatimPart);
    std::memcpy(memory + verbatimPart, &surface->textureVariation, sizeof(surface->textureVariation));
    std::memcpy(memory + surfaceSheenOffset, surface->sheen, sizeof(surface->sheen));
    std::memcpy(memory + surfaceSheenOffset + sizeof(surface->sheen), &surface->sheenRoughness, sizeof(surface->sheenRoughness));
    std::memcpy(memory + surfaceNormalScaleOffset, &surface->normalScale, sizeof(surface->normalScale));
    const Texture* const textures[] = {surface->texture, surface->normalMap, surface->roughnessMap};
    for(std::size_t i = 0; i < 3; ++i)
      if(textures[i])
      {
        const MTL::ResourceID resourceID = mtlTextures[textures[i]->index]->gpuResourceID();
        std::memcpy(memory + surfaceTextureOffset + i * sizeof(resourceID), &resourceID, sizeof(resourceID));
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
    if(light.dynamic)
    {
      // The light of the sky is a directional light whose color, direction and shadow map matrix are passed to the shader.
      std::string visibility = "1.0";
      lightDirection = "metal::normalize(-skyLight[1].xyz)";
      if(light.shadowMap)
      {
        const std::string index = std::to_string(*light.shadowMap);
        visibility = "illumination(shadowMaps[" + index + "].map, metal::float4x4(skyLight[4], skyLight[5], skyLight[6], skyLight[7]), in.positionInWorld, " + lightDirection + ", normalizedNormal)";
      }
      lightCalculationsCode += " { DirLight " + name + " = {skyLight[0].xyz, skyLight[1].xyz}; Lo += calcDirLight(" + name + ", normalizedNormal, viewDir, albedo, F0, roughness, surface.metallic, sheen, " + visibility + "); }";
      continue;
    }
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
    const std::string arguments = ", viewDir, albedo, F0, roughness, surface.metallic, sheen, " + visibility + ");";
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
    sourceCode += "#define GLOBAL_AMBIENT_LIGHT_GROUND " + toMSL(globalAmbientLightGround) + "\n";
    sourceCode += "#define EXPOSURE " + std::to_string(exposure) + "\n";
    if(filmicToneMapping)
      sourceCode += "#define FILMIC_TONE_MAPPING\n";
    if(i & 4)
    {
      sourceCode += "#define WITH_LIGHTING\n";
      if(hasSky)
      {
        // The ambient light and the exposure change with the sky, so they come from the same values as the light of the sky.
        sourceCode += "#define WITH_SKY_LIGHT\n#undef GLOBAL_AMBIENT_LIGHT\n#undef GLOBAL_AMBIENT_LIGHT_GROUND\n#undef EXPOSURE\n";
        sourceCode += "#define GLOBAL_AMBIENT_LIGHT skyLight[2].xyz\n#define GLOBAL_AMBIENT_LIGHT_GROUND skyLight[3].xyz\n#define EXPOSURE skyLight[0].w\n";
      }
      if(environmentMap)
      {
        sourceCode += "#define WITH_BACKDROP\n#define ENVIRONMENT_LEVELS " + std::to_string(environmentMap->levels) + ".0\n";
        // The coefficients of the irradiance.
        sourceCode += "constant float3 backdropIrradianceCoefficients[9] = {";
        for(std::size_t j = 0; j < 9; ++j)
          sourceCode += (j ? ", " : "") + std::string("float3(") + std::to_string(environmentMap->irradiance[j].x()) + ", " + std::to_string(environmentMap->irradiance[j].y()) + ", " + std::to_string(environmentMap->irradiance[j].z()) + ")";
        sourceCode += "};\n";
        sourceCode += R"metal(
float3 backdropIrradiance(float3 n)
{
  float3 result = backdropIrradianceCoefficients[0] * 0.282095;
  result += backdropIrradianceCoefficients[1] * (0.488603 * n.y) + backdropIrradianceCoefficients[2] * (0.488603 * n.z) + backdropIrradianceCoefficients[3] * (0.488603 * n.x);
  result += backdropIrradianceCoefficients[4] * (1.092548 * n.x * n.y) + backdropIrradianceCoefficients[5] * (1.092548 * n.y * n.z) +
            backdropIrradianceCoefficients[6] * (0.315392 * (3.0 * n.z * n.z - 1.0)) + backdropIrradianceCoefficients[7] * (1.092548 * n.x * n.z) +
            backdropIrradianceCoefficients[8] * (0.546274 * (n.x * n.x - n.y * n.y));
  return metal::max(result, float3(0.0));
}
)metal";
      }
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
        createRenderPipelineStates(library, false, j == 1, &pipelineStates[(j * numOfShaders + i) * numOfVertexCategories]);
      library->release();
    }
  }
  if(MTL::Library* library = compileLibrary(depthOnlyShaderSourceCode); library)
  {
    createRenderPipelineStates(library, true, false, &pipelineStates[depthOnlyShaderIndex * numOfVertexCategories]);
    library->release();
  }
  if(MTL::Library* library = compileLibrary(depthOnlyAlphaTestShaderSourceCode); library)
  {
    MTL::RenderPipelineDescriptor* descriptor = MTL::RenderPipelineDescriptor::alloc()->init();
    MTL::Function* vertexFunction = library->newFunction(toNSString("vertexMain"));
    MTL::Function* fragmentFunction = library->newFunction(toNSString("fragmentMain"));
    descriptor->setVertexFunction(vertexFunction);
    descriptor->setFragmentFunction(fragmentFunction);
    descriptor->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);
    descriptor->setVertexDescriptor(vertexDescriptors[VertexPNT::category]);
    NS::Error* error = nullptr;
    depthOnlyAlphaTestPipelineState = device->newRenderPipelineState(descriptor, &error);
    if(!depthOnlyAlphaTestPipelineState)
    {
      TRACE("%s", error ? error->localizedDescription()->utf8String() : "Unknown error while creating a Metal render pipeline state.");
      ASSERT(false);
    }
    fragmentFunction->release();
    vertexFunction->release();
    descriptor->release();
    library->release();
  }
  if(MTL::Library* library = compileLibrary(readbackShaderSourceCode); library)
  {
    readbackPipelineStates[rgbImage] = createComputePipelineState(library, "readbackRGB");
    readbackPipelineStates[yuyvImage] = createComputePipelineState(library, "readbackYUYV");
    readbackPipelineStates[depthImage] = createComputePipelineState(library, "readbackDepth");
    library->release();
  }
  if(hasSky)
  {
    const std::string skySourceCode = std::string(filmicToneMapping ? "#define FILMIC_TONE_MAPPING\n" : "") + R"metal(
#include <metal_stdlib>
using namespace metal;
struct SkyVertexOut
{
  float4 position [[position]];
  float2 ndc;
};

vertex SkyVertexOut skyVertex(uint vertexID [[vertex_id]])
{
  // A triangle that covers everything, at the far plane.
  float2 position = float2(vertexID == 1 ? 3.0 : -1.0, vertexID == 2 ? 3.0 : -1.0);
  SkyVertexOut out;
  out.position = float4(position, 1.0, 1.0);
  out.ndc = position;
  return out;
}
)metal" + skyShaderCode(false) + R"metal(
fragment float4 skyFragment(SkyVertexOut in [[stage_in]], constant SkyParams& params [[buffer(7)]])
{
  float3 viewDirection = float3(in.ndc.x / params.v[11].x, in.ndc.y / params.v[11].y, -1.0);
  float3 direction = normalize(float3(dot(params.v[8].xyz, viewDirection), dot(params.v[9].xyz, viewDirection), dot(params.v[10].xyz, viewDirection)));
  float3 color = skyColor(direction, params) * params.v[3].w;
#ifdef FILMIC_TONE_MAPPING
  color = clamp((color * (2.51 * color + 0.03)) / (color * (2.43 * color + 0.59) + 0.14), 0.0, 1.0);
#else
  color = color / (color + float3(1.0));
#endif
  return float4(pow(color, float3(1.0 / 2.2)), 1.0);
}
)metal";
    if(MTL::Library* library = compileLibrary(skySourceCode.c_str()); library)
    {
      for(std::size_t i = 0; i < numOfSampleCounts; ++i)
        drawingPipelineStates[i * numOfDrawingPipelineStates + skyPipeline] = createDrawingPipelineState(library, "skyVertex", "skyFragment", nullptr, true, i == 1, true);
      library->release();
    }
  }
  if(environmentMap)
  {
    const std::string backdropSourceCode = "#define EXPOSURE " + std::to_string(exposure) + "\n" + std::string(filmicToneMapping ? "#define FILMIC_TONE_MAPPING\n" : "") + R"metal(
#include <metal_stdlib>
using namespace metal;
struct BackdropVertexOut
{
  float4 position [[position]];
  float2 ndc;
};

vertex BackdropVertexOut backdropVertex(uint vertexID [[vertex_id]])
{
  // A triangle that covers everything, at the far plane.
  float2 position = float2(vertexID == 1 ? 3.0 : -1.0, vertexID == 2 ? 3.0 : -1.0);
  BackdropVertexOut out;
  out.position = float4(position, 1.0, 1.0);
  out.ndc = position;
  return out;
}
)metal" + skyShaderCode(false) + R"metal(
// The values are the same as those of the sky (see Graphics/SkyShader.h).
fragment float4 backdropFragment(BackdropVertexOut in [[stage_in]], constant SkyParams& params [[buffer(7)]], texturecube<float> environment [[texture(0)]])
{
  constexpr sampler environmentSampler(filter::linear, address::clamp_to_edge);
  float3 viewDirection = float3(in.ndc.x / params.v[11].x, in.ndc.y / params.v[11].y, -1.0);
  float3 direction = normalize(float3(dot(params.v[8].xyz, viewDirection), dot(params.v[9].xyz, viewDirection), dot(params.v[10].xyz, viewDirection)));
  GroundSample ground = groundSample(direction, params);

  // The cube map was computed at the center of the room, so the point where the ray hits the room is looked up (the walls and
  // the ceiling are at the right place for the viewer).
  float3 cameraPosition = params.v[14].xyz;
  float3 sides = float3(params.v[0].x, params.v[0].y, direction.z > 0.0 ? params.v[0].z : 0.0) * float3(direction.x > 0.0 ? 1.0 : -1.0, direction.y > 0.0 ? 1.0 : -1.0, 1.0);
  float3 times = (sides - cameraPosition) / (direction + select(float3(-1e-6), float3(1e-6), direction >= 0.0));
  float hitTime = max(min(times.x, times.y), 0.1);
  if(direction.z > 0.0)
    hitTime = min(hitTime, max(times.z, 0.1));
  float3 lookup = normalize(cameraPosition + direction * hitTime - float3(0.0, 0.0, params.v[0].w));
  float3 color = environment.sample(environmentSampler, params.v[14].w > 0.5 ? lookup : direction, level(0)).rgb;
  if(params.v[14].w > 0.5 && direction.z < 0.0 && ground.t < hitTime)
    color = groundColor(ground, params);
  color *= EXPOSURE;
#ifdef FILMIC_TONE_MAPPING
  color = clamp((color * (2.51 * color + 0.03)) / (color * (2.43 * color + 0.59) + 0.14), 0.0, 1.0);
#else
  color = color / (color + float3(1.0));
#endif
  return float4(pow(color, float3(1.0 / 2.2)), 1.0);
}
)metal";
    if(MTL::Library* library = compileLibrary(backdropSourceCode); library)
    {
      for(std::size_t i = 0; i < numOfSampleCounts; ++i)
        drawingPipelineStates[i * numOfDrawingPipelineStates + backdropPipeline] = createDrawingPipelineState(library, "backdropVertex", "backdropFragment", nullptr, true, i == 1, true);
      library->release();
    }
  }
  if(MTL::Library* library = compileLibrary(drawingShaderSourceCode); library)
  {
    for(std::size_t i = 0; i < numOfSampleCounts; ++i)
    {
      MTL::RenderPipelineState** states = &drawingPipelineStates[i * numOfDrawingPipelineStates];
      states[streamPipeline] = createDrawingPipelineState(library, "drawingStreamVertex", "drawingColorFragment", drawingVertexDescriptor, true, i == 1);
      states[instancePipeline] = createDrawingPipelineState(library, "drawingInstanceVertex", "drawingColorFragment", vertexDescriptors[VertexPN::category], true, i == 1);
      states[imagePipeline] = createDrawingPipelineState(library, "drawingImageVertex", "drawingImageFragment", vertexDescriptors[VertexPNT::category], true, i == 1);
      states[clearDepthPipeline] = createDrawingPipelineState(library, "drawingClearDepthVertex", "drawingClearDepthFragment", nullptr, false, i == 1);
    }
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

  pool->release();
}

void GraphicsContext::destroyDeviceResources()
{
  waitForOffscreenRendering(numOfSubmissions);
  waitForShadowMaps();
  for(MTL::Buffer* buffer : mtlReadbackBuffers)
    buffer->release();
  mtlReadbackBuffers.clear();
  for(const auto& pair : offscreenTargets)
    pair.second.renderPass->release();
  offscreenTargets.clear();
  for(const auto& pair : drawingTextureCache)
    pair.second.texture->release();
  drawingTextureCache.clear();
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
  for(MTL::ComputePipelineState* pipelineState : readbackPipelineStates)
    if(pipelineState)
      pipelineState->release();
  readbackPipelineStates.fill(nullptr);
  for(MTL::RenderPipelineState* pipelineState : pipelineStates)
    if(pipelineState)
      pipelineState->release();
  pipelineStates.fill(nullptr);
  if(depthOnlyAlphaTestPipelineState)
    depthOnlyAlphaTestPipelineState->release();
  depthOnlyAlphaTestPipelineState = nullptr;
  for(MTL::RenderPassDescriptor* renderPass : shadowMapRenderPasses)
    renderPass->release();
  shadowMapRenderPasses.clear();
  for(MTL::Texture* texture : shadowMapTextures)
    texture->release();
  shadowMapTextures.clear();
  if(shadowMapBuffer)
    shadowMapBuffer->release();
  for(auto& entry : drawLists)
    for(DrawList::Pass* pass : {&entry.second->color, &entry.second->depthOnly})
      if(pass->native)
      {
        reinterpret_cast<MTL::Buffer*>(pass->native)->release();
        pass->native = 0;
      }
  if(modelMatrixBuffer)
    modelMatrixBuffer->release();
  modelMatrixBuffer = nullptr;
  {
    std::lock_guard lock(retiredBuffers->mutex);
    retiredBuffers->closed = true;
  }
  retiredBuffers->releaseAll();
  surfaceBuffer->release();
  for(MTL::Texture* texture : mtlTextures)
    if(texture)
      texture->release();
  mtlTextures.clear();
  if(environmentTexture)
    environmentTexture->release();
  environmentTexture = nullptr;
  if(textureHeap)
    textureHeap->release();
  if(indexBuffer)
    indexBuffer->release();
  if(vertexBuffer)
    vertexBuffer->release();
  commandQueue->release();
  device->release();

  depthStencilState = noDepthWriteState = clearDepthState = nullptr;
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

void GraphicsContext::updateShadowMaps(const std::function<void()>& draw, bool offscreen, bool force)
{
  ASSERT(!commandBuffer);
  ASSERT(!encoder);

  if(shadowMaps.empty())
    return;
  if(!force && lastShadowMapUpdate == Simulation::simulation->simulationStep)
  {
    // The shadow maps of this step may have been rendered for off-screen renderings, which do not wait for them.
    if(!offscreen)
      waitForShadowMaps();
    return;
  }
  lastShadowMapUpdate = Simulation::simulation->simulationStep;

  // The shader looks up the position in the same clip space as the one in which the shadow map is rendered.
  if(fitShadowMaps(draw))
    for(std::size_t i = 0; i < shadowMaps.size(); ++i)
    {
      const Matrix4f lightPV = clipSpaceCorrection * shadowMaps[i].projection * shadowMaps[i].view;
      std::memcpy(static_cast<std::uint8_t*>(shadowMapBuffer->contents()) + i * shadowMapMemorySize, lightPV.data(), sizeof(float) * 16);
    }

  // The shadow maps are rendered with a separate command buffer, so that no render pass of an external renderer is
  // interrupted. For an external renderer (which uses another command queue), it is completed before the function returns,
  // so that the shadow maps are ready when they are used. Off-screen renderings use the same command queue, so they are
  // executed after it (Metal tracks the dependency on the shadow maps), and the CPU does not have to wait.
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
  // A command buffer on the same queue is completed after the ones that were committed before it.
  if(pendingShadowMapCommandBuffer)
  {
    pendingShadowMapCommandBuffer->release();
    pendingShadowMapCommandBuffer = nullptr;
  }
  if(offscreen)
    pendingShadowMapCommandBuffer = commandBuffer->retain();
  else
    commandBuffer->waitUntilCompleted();
  commandBuffer = nullptr;
  renderingShadowMap = false;
  renderDepthOnly = false;
  pool->release();
}

void GraphicsContext::waitForShadowMaps()
{
  if(!pendingShadowMapCommandBuffer)
    return;
  pendingShadowMapCommandBuffer->waitUntilCompleted();
  pendingShadowMapCommandBuffer->release();
  pendingShadowMapCommandBuffer = nullptr;
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
  startCulling(projection, view, viewportX >= 0 ? viewportHeight : 0);
  currentProjection = projection;
  currentView = view;
  currentLighting = lighting;
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
  bindModelMatrices();
  if(renderDepthOnly)
  {
    // The surfaces are needed to discard the fragments of instances with textures with alpha.
    encoder->setFragmentBuffer(surfaceBuffer, 0, 0);
    if(textureHeap)
      encoder->useHeap(textureHeap, MTL::RenderStageFragment);
  }
  else
  {
    Vector4f pos;
    pos << -view.topLeftCorner<3, 3>().transpose() * view.topRightCorner<3, 1>(), 0.f;
    encoder->setFragmentBytes(pos.data(), sizeof(Vector4f), 1);
    encoder->setFragmentBuffer(surfaceBuffer, 0, 0);
    if(textureHeap)
      encoder->useHeap(textureHeap, MTL::RenderStageFragment);
    if(lighting && environmentTexture)
      encoder->setFragmentTexture(environmentTexture, 0);
    if(lighting && hasSky)
    {
      // The shadow map matrix is in the clip space of Metal.
      std::array<Vector4f, 8> values = getSkyLightValues();
      Matrix4f lightPV;
      lightPV << values[4], values[5], values[6], values[7];
      lightPV = clipSpaceCorrection * lightPV;
      for(int i = 0; i < 4; ++i)
        values[4 + i] = lightPV.col(i);
      encoder->setFragmentBytes(values.data(), sizeof(values), 6);
    }
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
  {
    const std::uint32_t index = static_cast<std::uint32_t>(forcedSurface->index);
    encoder->setVertexBytes(&index, sizeof(index), 5);
  }
}

void GraphicsContext::releaseWhenCompleted(MTL::Buffer* buffer)
{
  // The command buffer of a QRhiWidget has unretained references (it does not keep the resources that it uses alive), and the
  // command buffers of the other queue (the one that renders the cameras) and earlier ones may still be running, so the buffer is
  // kept for a second after the current command buffer has completed (a frame takes a few milliseconds).
  const auto retire = [](RetiredBuffers& retired, MTL::Buffer* buffer)
  {
    const auto now = std::chrono::steady_clock::now();
    std::lock_guard lock(retired.mutex);
    if(retired.closed)
    {
      buffer->release();
      return;
    }
    retired.buffers.emplace_back(buffer, now);
    std::size_t numOfOld = 0;
    while(numOfOld < retired.buffers.size() && now - retired.buffers[numOfOld].second > std::chrono::seconds(1))
      retired.buffers[numOfOld++].first->release();
    retired.buffers.erase(retired.buffers.begin(), retired.buffers.begin() + static_cast<std::ptrdiff_t>(numOfOld));
  };
  if(commandBuffer)
    commandBuffer->addCompletedHandler([retire, retired = retiredBuffers, buffer](MTL::CommandBuffer*){retire(*retired, buffer);});
  else
    retire(*retiredBuffers, buffer);
}

void GraphicsContext::bindModelMatrices()
{
  if(!modelMatrixBuffer || uploadedModelMatrixGeneration != modelMatrixGeneration)
  {
    if(modelMatrixBuffer)
      releaseWhenCompleted(modelMatrixBuffer);
    modelMatrixBuffer = device->newBuffer(modelMatrixArray.data(), std::max<std::size_t>(modelMatrixArray.size(), 1) * sizeof(Matrix4f), MTL::ResourceCPUCacheModeDefaultCache | MTL::ResourceStorageModeShared);
    uploadedModelMatrixGeneration = modelMatrixGeneration;
  }
  encoder->setVertexBuffer(modelMatrixBuffer, 0, 2);
  modelMatrixBound = true;
}

void GraphicsContext::drawList(const DrawList* drawList)
{
  if(fittingShadowMaps)
  {
    accumulateBounds(drawList->depthOnly);
    return;
  }
  ASSERT(encoder);
  ASSERT(selectedPipelineStates);
  ASSERT(!recordingList);
  const DrawList::Pass& pass = renderDepthOnly ? drawList->depthOnly : drawList->color;
  if(pass.batches.empty())
    return;
  if(!modelMatrixBound || uploadedModelMatrixGeneration != modelMatrixGeneration)
    bindModelMatrices();

  // Only the instances that are inside the view are drawn (if they are all, the instances of the pass are used as they are).
  bool culled = false;
  if(cullingActive)
  {
    cullInstances(pass, !renderDepthOnly || renderingShadowMap, visibleInstances);
    culled = !visibleInstances.all;
  }
  if(culled)
  {
    if(visibleInstances.instances.empty())
      return;
    const std::size_t size = visibleInstances.instances.size() * sizeof(DrawList::Instance);
    if(size <= maxSetBytesSize)
      encoder->setVertexBytes(visibleInstances.instances.data(), size, 4);
    else
    {
      MTL::Buffer* buffer = device->newBuffer(visibleInstances.instances.data(), size, MTL::ResourceCPUCacheModeDefaultCache | MTL::ResourceStorageModeShared);
      encoder->setVertexBuffer(buffer, 0, 4);
      releaseWhenCompleted(buffer);
    }
  }
  else
  {
    if(!pass.native)
      pass.native = reinterpret_cast<std::uintptr_t>(device->newBuffer(pass.instances.data(), pass.instances.size() * sizeof(DrawList::Instance), MTL::ResourceCPUCacheModeDefaultCache | MTL::ResourceStorageModeShared));
    encoder->setVertexBuffer(reinterpret_cast<const MTL::Buffer*>(pass.native), 0, 4);
  }
  if(!renderDepthOnly && !forcedSurface)
  {
    const std::uint32_t noSurface = 0xffffffffu;
    encoder->setVertexBytes(&noSurface, sizeof(noSurface), 5);
  }
  const std::size_t numOfDraws = culled ? visibleInstances.ranges.size() : pass.batches.size();
  for(std::size_t i = 0; i < numOfDraws; ++i)
  {
    // Without culling, the batches are drawn as they are.
    const VisibleInstances::Range range = culled ? visibleInstances.ranges[i] : VisibleInstances::Range{static_cast<std::uint32_t>(i), 0, pass.batches[i].firstInstance, pass.batches[i].numOfInstances};
    const DrawList::Batch& batch = pass.batches[range.batch];
    const Mesh* mesh = batch.mesh;
    MTL::RenderPipelineState* pipelineState = batch.alphaTest && renderingShadowMap ? depthOnlyAlphaTestPipelineState : selectedPipelineStates[mesh->vertexBuffer->category];
    if(pipelineState != boundPipelineState)
      encoder->setRenderPipelineState((boundPipelineState = pipelineState));
    if(mesh->indexBuffer)
    {
      const IndexBuffer* indices = getIndexBuffer(*mesh, range.lod);
      encoder->drawIndexedPrimitives(getPrimitiveType(mesh->topology), indices->count, MTL::IndexTypeUInt32, indexBuffer, indices->offset, range.numOfInstances, mesh->vertexBuffer->base, range.firstInstance);
    }
    else
      encoder->drawPrimitives(getPrimitiveType(mesh->topology), mesh->vertexBuffer->base, mesh->vertexBuffer->count, range.numOfInstances, range.firstInstance);
  }
}

void GraphicsContext::finishRendering()
{
  ASSERT(encoder);
  ASSERT(selectedPipelineStates);
  ASSERT(!forcedSurface);
  selectedPipelineStates = nullptr;
  boundPipelineState = nullptr;
}

bool GraphicsContext::startOffscreenRendering(int width, int height, bool depthOnly, bool multisample)
{
  ASSERT(device);
  ASSERT(width > 0 && height > 0);
  ASSERT(!commandBuffer);
  ASSERT(!encoder);
  ASSERT(!offscreenTarget);

  autoreleasePool = NS::AutoreleasePool::alloc()->init();

  offscreenTarget = &getOffscreenTarget(width, height, depthOnly, multisample && !depthOnly);
  sampleCountIndex = multisample && !depthOnly ? 1 : 0;
  renderTargetHeight = height;
  commandBuffer = commandQueue->commandBuffer();
  encoder = commandBuffer->renderCommandEncoder(offscreenTarget->renderPass);
  renderDepthOnly = depthOnly;

  return true;
}

std::uint64_t GraphicsContext::finishOffscreenRendering()
{
  ASSERT(commandBuffer);
  ASSERT(encoder);
  ASSERT(offscreenTarget);
  ASSERT(!selectedPipelineStates);

  encoder->endEncoding();
  encoder = nullptr;

  if(!offscreenReadbacks.empty())
  {
    // The layout of the struct ReadbackParameters in the kernels.
    struct ReadbackParameters
    {
      std::uint32_t width;
      std::uint32_t height;
      std::uint32_t firstRow;
      std::uint32_t seed;
      float depthNumerator;
      float depthOffset;
      std::uint32_t modeled;
      float blur;
      float gain[3];
      float saturation;
      float vignette;
      float noise;
    };

    MTL::Texture* colorTexture = offscreenTarget->readTexture;
    MTL::ComputeCommandEncoder* computeEncoder = commandBuffer->computeCommandEncoder(MTL::DispatchTypeConcurrent);
    for(const OffscreenReadback& readback : offscreenReadbacks)
    {
      const ReadbackBuffer& buffer = *readback.buffer;
      // Depth images can only be read from depth-only render targets (otherwise the depth is not stored) and vice versa.
      ASSERT((buffer.format == depthImage) == renderDepthOnly);
      ASSERT(readback.y >= 0 && readback.y + static_cast<int>(buffer.height) <= renderTargetHeight);
      MTL::Texture* texture = renderDepthOnly ? offscreenTarget->renderPass->depthAttachment()->texture() : colorTexture;
      ASSERT(texture->width() >= buffer.width);
      // The rows of textures are counted from the top in Metal.
      const SensorModel& sensorModel = buffer.sensorModel;
      const bool modeled = buffer.format != depthImage && !sensorModel.isIdeal();
      const ReadbackParameters parameters = {buffer.width, buffer.height, static_cast<std::uint32_t>(renderTargetHeight - readback.y - static_cast<int>(buffer.height)),
                                             static_cast<std::uint32_t>(numOfSubmissions * 2654435761u) + static_cast<std::uint32_t>(buffer.index) * 0x9e3779b9u, buffer.depthNumerator, buffer.depthOffset,
                                             modeled ? 1u : 0u, sensorModel.blur, {sensorModel.gain[0], sensorModel.gain[1], sensorModel.gain[2]}, sensorModel.saturation, sensorModel.vignette, sensorModel.noise};
      computeEncoder->setComputePipelineState(readbackPipelineStates[buffer.format]);
      computeEncoder->setTexture(texture, 0);
      computeEncoder->setBuffer(mtlReadbackBuffers[buffer.index], 0, 0);
      computeEncoder->setBytes(&parameters, sizeof(parameters), 1);
      computeEncoder->dispatchThreads(MTL::Size(buffer.format == yuyvImage ? buffer.width / 2 : buffer.width, buffer.height, 1), MTL::Size(16, 16, 1));
    }
    computeEncoder->endEncoding();
    offscreenReadbacks.clear();
  }

  // The command buffer is autoreleased, so it must be retained until it has been waited for.
  sampleCountIndex = 0;
  commandBuffer->commit();
  submissions.emplace_back(++numOfSubmissions, commandBuffer->retain());
  commandBuffer = nullptr;
  offscreenTarget = nullptr;

  autoreleasePool->release();
  autoreleasePool = nullptr;

  return numOfSubmissions;
}

void GraphicsContext::waitForOffscreenRendering(std::uint64_t submission)
{
  while(!submissions.empty() && submissions.front().first <= submission)
  {
    submissions.front().second->waitUntilCompleted();
    submissions.front().second->release();
    submissions.pop_front();
  }
}

bool GraphicsContext::startExternalRendering(const QRhiNativeHandles* nativeHandles, bool multisample)
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
  sampleCountIndex = multisample ? 1 : 0;

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

void GraphicsContext::createRenderPipelineStates(MTL::Library* library, bool depthOnly, bool multisample, MTL::RenderPipelineState** pipelineStates)
{
  MTL::RenderPipelineDescriptor* descriptor = MTL::RenderPipelineDescriptor::alloc()->init();
  descriptor->setRasterSampleCount(multisample ? multisampleCount : 1);
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
  ASSERT(!drawingBuffer && !drawingInstanceBuffer && drawingTextures.empty());

  // Release the textures of drawings that do not exist anymore. Command buffers that still use them have retained them.
  for(auto i = drawingTextureCache.begin(); i != drawingTextureCache.end();)
    if(i->second.drawing.expired())
    {
      i->second.texture->release();
      i = drawingTextureCache.erase(i);
    }
    else
      ++i;

  if(drawings.numOfVertices)
  {
    drawingBuffer = device->newBuffer(drawings.numOfVertices * sizeof(PreparedDrawing::Vertex), MTL::ResourceCPUCacheModeDefaultCache | MTL::ResourceStorageModeShared);
    std::uint8_t* memory = static_cast<std::uint8_t*>(drawingBuffer->contents());
    for(const DrawingList::Item& item : drawings.items)
      std::memcpy(memory + item.firstVertex * sizeof(PreparedDrawing::Vertex), item.drawing->vertices.data(), item.drawing->vertices.size() * sizeof(PreparedDrawing::Vertex));
  }
  if(drawings.numOfInstances)
  {
    drawingInstanceBuffer = device->newBuffer(drawings.numOfInstances * sizeof(PreparedDrawing::Instance), MTL::ResourceCPUCacheModeDefaultCache | MTL::ResourceStorageModeShared);
    std::uint8_t* memory = static_cast<std::uint8_t*>(drawingInstanceBuffer->contents());
    for(const DrawingList::Item& item : drawings.items)
      std::memcpy(memory + item.firstInstance * sizeof(PreparedDrawing::Instance), item.drawing->instances.data(), item.drawing->instances.size() * sizeof(PreparedDrawing::Instance));
  }

  // Images are uploaded when they are drawn for the first time.
  drawingTextures.reserve(drawings.numOfImages);
  for(const DrawingList::Item& item : drawings.items)
    for(const PreparedDrawing::Image& image : item.drawing->images)
    {
      const auto [i, inserted] = drawingTextureCache.try_emplace(image.id);
      if(inserted)
      {
        MTL::TextureDescriptor* textureDescriptor = MTL::TextureDescriptor::texture2DDescriptor(MTL::PixelFormatRGBA8Unorm, image.width, image.height, false);
        textureDescriptor->setUsage(MTL::TextureUsageShaderRead);
        i->second.texture = device->newTexture(textureDescriptor);
        ASSERT(i->second.texture);
        i->second.texture->replaceRegion(MTL::Region(0, 0, image.width, image.height), 0, image.rgba.data(), image.width * 4);
        i->second.drawing = item.drawing;
      }
      drawingTextures.push_back(i->second.texture->retain());
    }
}

void GraphicsContext::drawDrawings(const DrawingList& drawings, const Matrix4f& projection, const Matrix4f& view, float alpha, bool opaquePass, bool fillPolygons)
{
  ASSERT(commandBuffer);
  ASSERT(encoder);
  ASSERT(!selectedPipelineStates);
  ASSERT(drawingSphereMesh && drawingCylinderMesh && drawingQuadMesh);

  const Matrix4f pv = clipSpaceCorrection * projection * view;

  encoder->setFrontFacingWinding(MTL::WindingCounterClockwise);
  encoder->setCullMode(MTL::CullModeBack);
  encoder->setTriangleFillMode(fillPolygons ? MTL::TriangleFillModeFill : MTL::TriangleFillModeLines);
  boundPipelineState = nullptr;
  const auto bindPipeline = [this](DrawingPipeline pipeline)
  {
    MTL::RenderPipelineState* pipelineState = drawingPipelineStates[sampleCountIndex * numOfDrawingPipelineStates + pipeline];
    if(pipelineState != boundPipelineState)
      encoder->setRenderPipelineState((boundPipelineState = pipelineState));
  };
  // Draws instances of a mesh of the context (only the positions of its vertices are used).
  const auto drawInstances = [this](const Mesh* mesh, std::size_t firstInstance, std::size_t numOfInstances)
  {
    encoder->drawIndexedPrimitives(getPrimitiveType(mesh->topology), mesh->indexBuffer->count, MTL::IndexTypeUInt32, indexBuffer, mesh->indexBuffer->offset, numOfInstances, mesh->vertexBuffer->base, firstInstance);
  };

  for(const DrawingList::Item& item : drawings.items)
  {
    const PreparedDrawing& drawing = *item.drawing;
    if(drawing.renderOptions & (SimRobotCore3::Drawing::disableTransparency | SimRobotCore3::Drawing::disableOpacity))
    {
      if(opaquePass != ((drawing.renderOptions & SimRobotCore3::Drawing::disableTransparency) != 0))
        continue;
    }
    encoder->setDepthStencilState((drawing.renderOptions & SimRobotCore3::Drawing::disableDepth) ? noDepthWriteState : depthStencilState);

    const Matrix4f pvm = pv * item.transformation;

    if(!drawing.vertices.empty())
    {
      const std::size_t firstPoint = item.firstVertex + drawing.numOfLineVertices;
      const std::size_t firstTriangleVertex = firstPoint + drawing.numOfPoints;
      bindPipeline(streamPipeline);
      encoder->setVertexBuffer(drawingBuffer, 0, 0);
      encoder->setVertexBytes(pvm.data(), sizeof(Matrix4f), 1);
      encoder->setVertexBytes(&alpha, sizeof(alpha), 2);
      if(drawing.numOfLineVertices)
        encoder->drawPrimitives(MTL::PrimitiveTypeLine, item.firstVertex, drawing.numOfLineVertices);
      if(drawing.numOfPoints)
        encoder->drawPrimitives(MTL::PrimitiveTypePoint, firstPoint, drawing.numOfPoints);
      if(drawing.numOfTriangleVertices)
        encoder->drawPrimitives(MTL::PrimitiveTypeTriangle, firstTriangleVertex, drawing.numOfTriangleVertices);
    }

    if(!drawing.instances.empty())
    {
      bindPipeline(instancePipeline);
      encoder->setVertexBuffer(vertexBuffer, 0, 0);
      encoder->setVertexBuffer(drawingInstanceBuffer, 0, 1);
      encoder->setVertexBytes(pvm.data(), sizeof(Matrix4f), 2);
      encoder->setVertexBytes(&alpha, sizeof(alpha), 3);
      if(drawing.numOfSpheres)
        drawInstances(drawingSphereMesh, item.firstInstance, drawing.numOfSpheres);
      if(drawing.numOfCylinders)
        drawInstances(drawingCylinderMesh, item.firstInstance + drawing.numOfSpheres, drawing.numOfCylinders);
    }

    if(!drawing.images.empty())
    {
      bindPipeline(imagePipeline);
      encoder->setVertexBuffer(vertexBuffer, 0, 0);
      encoder->setFragmentBytes(&alpha, sizeof(alpha), 0);
      encoder->setCullMode(MTL::CullModeNone);
      for(std::size_t i = 0; i < drawing.images.size(); ++i)
      {
        const Matrix4f imagePVM = pv * imageToWorld(item, drawing.images[i], view);
        encoder->setVertexBytes(imagePVM.data(), sizeof(Matrix4f), 1);
        encoder->setFragmentTexture(drawingTextures[item.firstImage + i], 0);
        encoder->drawIndexedPrimitives(getPrimitiveType(drawingQuadMesh->topology), drawingQuadMesh->indexBuffer->count, MTL::IndexTypeUInt32, indexBuffer, drawingQuadMesh->indexBuffer->offset, 1, drawingQuadMesh->vertexBuffer->base, 0);
      }
      encoder->setCullMode(MTL::CullModeBack);
    }
  }

  // Restore the state that the other rendering expects.
  encoder->setTriangleFillMode(MTL::TriangleFillModeFill);
  encoder->setDepthStencilState(depthStencilState);
  boundPipelineState = nullptr;
}

void GraphicsContext::finishDrawings()
{
  ASSERT(commandBuffer);
  // The GPU uses the resources after the commands have been encoded, so they are released when the command buffer has been completed.
  MTL::Buffer* buffer = drawingBuffer;
  MTL::Buffer* instanceBuffer = drawingInstanceBuffer;
  std::vector<MTL::Texture*> textures = std::move(drawingTextures);
  drawingBuffer = drawingInstanceBuffer = nullptr;
  drawingTextures.clear();
  if(buffer || instanceBuffer || !textures.empty())
    commandBuffer->addCompletedHandler([buffer, instanceBuffer, textures](MTL::CommandBuffer*)
    {
      if(buffer)
        buffer->release();
      if(instanceBuffer)
        instanceBuffer->release();
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

void GraphicsContext::drawSky()
{
  ASSERT(encoder);
  ASSERT(selectedPipelineStates);
  if((!hasSky && !environmentTexture) || !currentLighting || renderDepthOnly)
    return;
  MTL::RenderPipelineState* pipelineState = drawingPipelineStates[sampleCountIndex * numOfDrawingPipelineStates + (hasSky ? skyPipeline : backdropPipeline)];
  if(!pipelineState)
    return;
  const std::array<Vector4f, numOfSkyUniforms> values = getSkyUniforms(currentProjection, currentView);
  encoder->setCullMode(MTL::CullModeNone);
  encoder->setDepthStencilState(noDepthWriteState);
  encoder->setRenderPipelineState(pipelineState);
  encoder->setFragmentBytes(values.data(), sizeof(values), 7);
  if(!hasSky)
    encoder->setFragmentTexture(environmentTexture, 0);
  encoder->drawPrimitives(MTL::PrimitiveTypeTriangle, static_cast<NS::UInteger>(0), static_cast<NS::UInteger>(3));
  encoder->setCullMode(MTL::CullModeBack);
  encoder->setDepthStencilState(depthStencilState);
  boundPipelineState = nullptr;
}

MTL::RenderPipelineState* GraphicsContext::createDrawingPipelineState(MTL::Library* library, const char* vertexName, const char* fragmentName, MTL::VertexDescriptor* vertexDescriptor, bool writeColor, bool multisample, bool opaque)
{
  MTL::RenderPipelineDescriptor* descriptor = MTL::RenderPipelineDescriptor::alloc()->init();
  descriptor->setRasterSampleCount(multisample ? multisampleCount : 1);
  MTL::Function* vertexFunction = library->newFunction(toNSString(vertexName));
  MTL::Function* fragmentFunction = library->newFunction(toNSString(fragmentName));
  descriptor->setVertexFunction(vertexFunction);
  descriptor->setFragmentFunction(fragmentFunction);
  if(vertexDescriptor)
    descriptor->setVertexDescriptor(vertexDescriptor);
  descriptor->setDepthAttachmentPixelFormat(MTL::PixelFormatDepth32Float);
  auto* attachment = descriptor->colorAttachments()->object(0);
  attachment->setPixelFormat(MTL::PixelFormatRGBA8Unorm);
  if(writeColor && !opaque)
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
  else if(!writeColor)
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

GraphicsContext::OffscreenTarget& GraphicsContext::getOffscreenTarget(int width, int height, bool depthOnly, bool multisample)
{
  ASSERT(!depthOnly || !multisample);

  // Considering weak graphics cards clearing is faster when the render targets are not greater than they have to be.
  // So we create an individual render target for each size in demand.
  OffscreenTarget& target = offscreenTargets[static_cast<std::uint64_t>(width) << 32 | static_cast<std::uint64_t>(height) << 2 | (multisample ? 2 : 0) | (depthOnly ? 1 : 0)];
  if(target.renderPass)
    return target;

  target.renderPass = MTL::RenderPassDescriptor::alloc()->init();

  // The multisampled attachments only exist while the render pass is executed (on the tiles of the GPU).
  const auto createAttachmentTexture = [this, width, height, multisample](MTL::PixelFormat pixelFormat, bool resolved)
  {
    MTL::TextureDescriptor* descriptor = MTL::TextureDescriptor::alloc()->init();
    descriptor->setPixelFormat(pixelFormat);
    descriptor->setWidth(width);
    descriptor->setHeight(height);
    if(multisample && !resolved)
    {
      descriptor->setTextureType(MTL::TextureType2DMultisample);
      descriptor->setSampleCount(multisampleCount);
      descriptor->setUsage(MTL::TextureUsageRenderTarget);
      descriptor->setStorageMode(MTL::StorageModeMemoryless);
    }
    else
    {
      descriptor->setTextureType(MTL::TextureType2D);
      descriptor->setUsage(MTL::TextureUsageRenderTarget | MTL::TextureUsageShaderRead);
      descriptor->setStorageMode(MTL::StorageModePrivate);
    }
    MTL::Texture* texture = device->newTexture(descriptor);
    descriptor->release();
    ASSERT(texture);
    return texture;
  };

  // The render pass descriptor retains the attachment textures, so they are released after being set.
  if(!depthOnly)
  {
    MTL::Texture* colorTexture = createAttachmentTexture(MTL::PixelFormatRGBA8Unorm, false);
    auto* colorAttachment = target.renderPass->colorAttachments()->object(0);
    colorAttachment->setTexture(colorTexture);
    colorAttachment->setLoadAction(MTL::LoadActionClear);
    colorAttachment->setClearColor(MTL::ClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]));
    if(multisample)
    {
      // The resolved image is the one that is read. The samples are discarded.
      MTL::Texture* resolveTexture = createAttachmentTexture(MTL::PixelFormatRGBA8Unorm, true);
      colorAttachment->setResolveTexture(resolveTexture);
      colorAttachment->setStoreAction(MTL::StoreActionMultisampleResolve);
      target.readTexture = resolveTexture;
      resolveTexture->release();
    }
    else
    {
      colorAttachment->setStoreAction(MTL::StoreActionStore);
      target.readTexture = colorTexture;
    }
    colorTexture->release();
  }

  MTL::Texture* depthTexture = createAttachmentTexture(MTL::PixelFormatDepth32Float, false);
  auto* depthAttachment = target.renderPass->depthAttachment();
  depthAttachment->setTexture(depthTexture);
  depthAttachment->setLoadAction(MTL::LoadActionClear);
  depthAttachment->setStoreAction(depthOnly ? MTL::StoreActionStore : MTL::StoreActionDontCare);
  depthAttachment->setClearDepth(1.0);
  depthTexture->release();

  return target;
}

#endif
