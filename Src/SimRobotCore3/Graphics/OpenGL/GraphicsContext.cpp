/**
 * @file Graphics/OpenGL/GraphicsContext.cpp
 *
 * This file implements a class that handles graphics using OpenGL 4.5 Core.
 *
 * @author Arne Hasselbring
 */

#ifdef SIMROBOTCORE3_USE_OPENGL

#include "GraphicsContext.h"
#include "Graphics/Light.h"
#include "Graphics/LightSpace.h"
#include "Platform/Assert.h"
#include "Simulation/Simulation.h"
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions_4_5_Core>
#include <algorithm>
#include <bit>
#include <cstddef>

// The following shader source code is based on https://learnopengl.com/Lighting/Multiple-lights
// and https://learnopengl.com/PBR/Lighting.

static const char* vertexShaderSourceCode = R"glsl(
layout(location = 0) in vec3 inPosInModel;
layout(location = 1) in vec3 inNormalInModel;
layout(location = 2) in vec2 inTexCoords;

out vec3 FragPosInWorld;
NORMAL_QUALIFIER out vec3 NormalInWorld;
out vec2 TexCoords;

uniform mat4 cameraPV;
uniform mat4 modelMatrix;

void main()
{
  FragPosInWorld = vec3(modelMatrix * vec4(inPosInModel, 1.0));
  NormalInWorld = mat3(modelMatrix) * inNormalInModel;
  TexCoords = inTexCoords;
  gl_Position = cameraPV * vec4(FragPosInWorld, 1.0);
}
)glsl";

static const char* depthOnlyVertexShaderSourceCode = R"glsl(
layout(location = 0) in vec3 inPosInModel;

uniform mat4 cameraPV;
uniform mat4 modelMatrix;

void main()
{
  gl_Position = cameraPV * modelMatrix * vec4(inPosInModel, 1.0);
}
)glsl";

static const char* fragmentShaderSourceCode = R"glsl(
struct DirLight
{
  vec3 color;
  vec3 direction;
};

struct PointLight
{
  vec3 color;
  vec3 position;
  float constantAttenuation;
  float linearAttenuation;
  float quadraticAttenuation;
};

struct SpotLight
{
  vec3 color;
  vec3 position;
  float constantAttenuation;
  float linearAttenuation;
  float quadraticAttenuation;
  vec3 direction;
  float cutoff;
};

struct Surface
{
  vec3 albedo;
  float alpha;
  float metallic;
  float roughness;
  float ambient;
  bool hasTexture;
};

in vec3 FragPosInWorld;
NORMAL_QUALIFIER in vec3 NormalInWorld;
in vec2 TexCoords;

uniform vec3 cameraPos;
uniform uint surfaceIndex;
#ifdef WITH_TEXTURES
layout(binding = 0) uniform sampler2D albedoTexture;
#endif
layout(std140, binding = 0) uniform Surfaces
{
  Surface surfaces[NUM_OF_SURFACES];
};

out vec4 FragColor;

const float PI = 3.14159265359;

float distributionTrowbridgeReitz(in vec3 normal, in vec3 halfDir, in float roughness)
{
  float alpha = roughness * roughness;
  float alphaSqr = alpha * alpha;
  float cosTheta = max(dot(normal, halfDir), 0.0);
  float denominator = cosTheta * cosTheta * (alphaSqr - 1.0) + 1.0;
  return alphaSqr / (PI * denominator * denominator);
}

float geometrySchlickGGX(in float cosTheta, in float roughness)
{
  float r = roughness + 1.0;
  float k = r * r / 8.0;
  return cosTheta / (cosTheta * (1.0 - k) + k);
}

float geometrySmith(in vec3 normal, in float cosView, in float cosLight, in float roughness)
{
  return geometrySchlickGGX(cosView, roughness) * geometrySchlickGGX(cosLight, roughness);
}

vec3 fresnelSchlick(in float cosTheta, in vec3 F0)
{
  return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 calcBrdf(in vec3 lightDir, in vec3 normal, in vec3 viewDir, in vec3 albedo, in vec3 F0)
{
  vec3 halfDir = normalize(viewDir + lightDir);
  float cosView = max(dot(normal, viewDir), 0.0);
  float cosLight = max(dot(normal, lightDir), 0.0);

  float NDF = distributionTrowbridgeReitz(normal, halfDir, surfaces[surfaceIndex].roughness);
  float G = geometrySmith(normal, cosView, cosLight, surfaces[surfaceIndex].roughness);
  vec3 F = fresnelSchlick(clamp(dot(halfDir, viewDir), 0.0, 1.0), F0);

  vec3 numerator = NDF * G * F;
  float denominator = 4.0 * cosView * cosLight + 0.0001;
  vec3 specular = numerator / denominator;

  vec3 kS = F;
  vec3 kD = (vec3(1.0) - kS) * (1.0 - surfaces[surfaceIndex].metallic);

  return kD * albedo / PI + specular;
}

// Returns the fraction of the fragment that is lit. The shadow map is sampled with hardware percentage closer filtering
// (a bilinear comparison) at four positions that together cover 3x3 texels.
float illumination(in sampler2DShadow shadowMap, in mat4 lightPV, in vec3 lightDir, in vec3 normal)
{
  float cosTheta = dot(normal, lightDir);
  if (cosTheta <= 0.0)
    return 1.0; // The surface faces away from the light, so there is no light to be blocked anyway.
  vec4 fragPosInLight = lightPV * vec4(FragPosInWorld, 1.0);
  if (fragPosInLight.w <= 0.0)
    return 1.0;
  // The size of a texel at the fragment in world coordinates (w is the distance for a perspective projection and 1 for an orthographic one).
  vec2 size = vec2(textureSize(shadowMap, 0));
  // The first two rows of the matrix are the scaled axes of the light plus the third axis for a projection that is not centered.
  vec3 row0 = vec3(lightPV[0][0], lightPV[1][0], lightPV[2][0]);
  vec3 row1 = vec3(lightPV[0][1], lightPV[1][1], lightPV[2][1]);
  vec3 depthAxis = -vec3(lightPV[0][3], lightPV[1][3], lightPV[2][3]); // zero for an orthographic projection
  vec2 scale = vec2(sqrt(max(dot(row0, row0) - pow(dot(row0, depthAxis), 2.0), 1e-12)),
                    sqrt(max(dot(row1, row1) - pow(dot(row1, depthAxis), 2.0), 1e-12)));
  vec2 texelInWorld = 2.0 * fragPosInLight.w / (size * scale);
  // Moving the lookup position along the normal avoids self-shadowing, the more the surface is inclined towards the light.
  vec3 offsetPos = FragPosInWorld + normal * (1.5 * max(texelInWorld.x, texelInWorld.y) * sqrt(max(1.0 - cosTheta * cosTheta, 0.0)));
  fragPosInLight = lightPV * vec4(offsetPos, 1.0);
  vec3 coordinates = 0.5 * fragPosInLight.xyz / fragPosInLight.w + 0.5;
  if (coordinates.z > 1.0)
    return 1.0;
  vec2 texel = 1.0 / size;
  float reference = coordinates.z - 0.0005;
  float visibility = 0.0;
  // The explicit level of detail is required because this code is not executed uniformly.
  for (int x = -1; x <= 1; x += 2)
  {
    for (int y = -1; y <= 1; y += 2)
      visibility += textureLod(shadowMap, vec3(coordinates.xy + 0.5 * vec2(x, y) * texel, reference), 0.0);
  }
  return 0.25 * visibility;
}

vec3 calcLight(in vec3 irradiance, in vec3 lightDir, in vec3 normal, in vec3 viewDir, in vec3 albedo, in vec3 F0, in float visibility)
{
  float cosTheta = max(dot(normal, lightDir), 0.0);
  return calcBrdf(lightDir, normal, viewDir, albedo, F0) * irradiance * cosTheta * visibility;
}

vec3 calcDirLight(const in DirLight light, in vec3 normal, in vec3 viewDir, in vec3 albedo, in vec3 F0, in float visibility)
{
  vec3 lightDir = normalize(-light.direction);
  vec3 irradiance = light.color;

  return calcLight(irradiance, lightDir, normal, viewDir, albedo, F0, visibility);
}

vec3 calcPointLight(const in PointLight light, in vec3 pos, in vec3 normal, in vec3 viewDir, in vec3 albedo, in vec3 F0, in float visibility)
{
  vec3 lightDir = normalize(light.position - pos);
  float distance = length(light.position - pos);
  float attenuation = 1.0 / (light.constantAttenuation + light.linearAttenuation * distance + light.quadraticAttenuation * distance * distance);
  vec3 irradiance = light.color * attenuation;

  return calcLight(irradiance, lightDir, normal, viewDir, albedo, F0, visibility);
}

vec3 calcSpotLight(const in SpotLight light, in vec3 pos, in vec3 normal, in vec3 viewDir, in vec3 albedo, in vec3 F0, in float visibility)
{
  vec3 lightDir = normalize(light.position - pos);
  float distance = length(light.position - pos);
  float attenuation = 1.0 / (light.constantAttenuation + light.linearAttenuation * distance + light.quadraticAttenuation * distance * distance);
  float cosTheta = dot(lightDir, normalize(-light.direction));
  float intensity = clamp((cosTheta - light.cutoff) / (1 - light.cutoff), 0.0, 1.0);
  vec3 irradiance = light.color * attenuation * intensity;

  return calcLight(irradiance, lightDir, normal, viewDir, albedo, F0, visibility);
}

#ifdef WITH_LIGHTING
DECLARE_LIGHTS
#endif

void main()
{
  vec4 color;
#ifdef WITH_LIGHTING
  vec3 normalizedNormal = normalize(NormalInWorld);
  vec3 viewDir = normalize(cameraPos - FragPosInWorld);

  vec3 albedo = surfaces[surfaceIndex].albedo;
  color.a = surfaces[surfaceIndex].alpha;
#ifdef WITH_TEXTURES
  if (surfaces[surfaceIndex].hasTexture)
  {
    vec4 modulation = texture(albedoTexture, TexCoords);
    albedo = albedo * modulation.rgb;
    color.a = color.a * modulation.a;
  }
#endif
  albedo = pow(albedo, vec3(2.2)); // convert from sRGB to linear
  vec3 F0 = vec3(0.04);
  F0 = mix(F0, albedo, surfaces[surfaceIndex].metallic);

  vec3 Lo = vec3(0.0);
  CALCULATE_LIGHTS
  vec3 ambient = GLOBAL_AMBIENT_LIGHT * albedo * surfaces[surfaceIndex].ambient;
  color.rgb = ambient + Lo;
  color.rgb = color.rgb / (color.rgb + vec3(1.0)); // Reinhard tone mapping
  color.rgb = pow(color.rgb, vec3(1.0 / 2.2)); // convert from linear to sRGB
#else
  color = vec4(surfaces[surfaceIndex].albedo, surfaces[surfaceIndex].alpha);
#ifdef WITH_TEXTURES
  if (surfaces[surfaceIndex].hasTexture)
    color = color * texture(albedoTexture, TexCoords);
#endif
#endif
  if (color.a < 0.01)
  {
    discard;
  }
  FragColor = color;
}
)glsl";

// The shaders of controller drawings (which are not lit): vertices with colors and sizes, instances of a mesh with colors, and a textured mesh.
static const char* drawingStreamVertexShaderSourceCode = R"glsl(
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec4 inColor;
layout(location = 2) in float inSize;

uniform mat4 pvm;
uniform float alphaScale;

out vec4 Color;

void main()
{
  Color = vec4(inColor.rgb, inColor.a * alphaScale);
  gl_Position = pvm * vec4(inPos, 1.0);
  gl_PointSize = inSize;
}
)glsl";

static const char* drawingInstanceVertexShaderSourceCode = R"glsl(
layout(location = 0) in vec3 inPos;
layout(location = 3) in mat4 inTransformation;
layout(location = 7) in vec2 inRadii;
layout(location = 8) in vec4 inColor;

uniform mat4 pvm;
uniform float alphaScale;

out vec4 Color;

void main()
{
  Color = vec4(inColor.rgb, inColor.a * alphaScale);
  // The vertices below and above the xy-plane are scaled by different radii (which makes cones from a cylinder).
  float radius = inPos.z < 0.0 ? inRadii.x : inRadii.y;
  gl_Position = pvm * inTransformation * vec4(inPos.xy * radius, inPos.z, 1.0);
}
)glsl";

static const char* drawingImageVertexShaderSourceCode = R"glsl(
layout(location = 0) in vec3 inPos;
layout(location = 2) in vec2 inTexCoords;

uniform mat4 pvm;

out vec2 TexCoords;

void main()
{
  TexCoords = inTexCoords;
  gl_Position = pvm * vec4(inPos, 1.0);
}
)glsl";

static const char* drawingColorFragmentShaderSourceCode = R"glsl(
in vec4 Color;
out vec4 FragColor;

void main()
{
  FragColor = Color;
}
)glsl";

static const char* drawingImageFragmentShaderSourceCode = R"glsl(
in vec2 TexCoords;
layout(binding = 0) uniform sampler2D tex;
uniform float alphaScale;
out vec4 FragColor;

void main()
{
  vec4 color = texture(tex, TexCoords);
  FragColor = vec4(color.rgb, color.a * alphaScale);
}
)glsl";

static const char* depthOnlyFragmentShaderSourceCode = R"glsl(
void main()
{
  // TODO: Later, this shader could compute gl_FragDepth including the postprocessing from the DepthImageSensor.
}
)glsl";

#ifndef NDEBUG
/** Reports a message of the OpenGL implementation (see \c glDebugMessageCallback). */
static void QT_APIENTRY reportDebugMessage(GLenum, GLenum, GLuint, GLenum, GLsizei, const GLchar* message, const void*)
{
  TRACE("OpenGL: %s", message);
}
#endif

GraphicsContext::GraphicsContext()
{
}

GraphicsContext::~GraphicsContext()
{
  ASSERT(!data);
  ASSERT(!shader);
  ASSERT(!forcedSurface);

  if(offscreenContext && offscreenSurface)
  {
    ASSERT(perContextData.size() == 1);
    ASSERT(perContextData.begin()->first == offscreenContext);
    offscreenContext->makeCurrent(offscreenSurface);
    destroyGraphics();
  }
  ASSERT(perContextData.empty());
  for(const auto& pair : offscreenBuffers)
    delete pair.second;
  offscreenBuffers.clear();
  delete offscreenContext;
  delete offscreenSurface;

}

bool GraphicsContext::compile(std::list<std::string>& errors)
{
  layoutResources();

  ASSERT(!offscreenSurface && !offscreenContext);

  offscreenSurface = new QOffscreenSurface;
  offscreenSurface->create();

  offscreenContext = new QOpenGLContext;
  offscreenContext->setShareContext(QOpenGLContext::globalShareContext());
  // If the requested version is not supported, an older one may be created instead.
  if(!offscreenContext->create() || offscreenContext->format().version() < std::make_pair(4, 5))
  {
    errors.push_back("SimRobotCore3 requires OpenGL 4.5 (core profile), which the graphics driver does not provide.");
    delete offscreenContext;
    delete offscreenSurface;
    offscreenContext = nullptr;
    offscreenSurface = nullptr;
    return false;
  }
  offscreenContext->makeCurrent(offscreenSurface);

  createGraphics();
  return true;
}

void GraphicsContext::createGraphics()
{
  const auto* context = QOpenGLContext::currentContext();

  // Check if the context is already initialized.
  if(perContextData.find(context) != perContextData.end())
  {
    ++perContextData[context].referenceCounter;
    return;
  }

  // Find a context with which this one is sharing (in that case, we don't need to upload things to memory again, just create VAOs and maybe some other stuff).
  const auto shareDataIt = std::find_if(perContextData.begin(), perContextData.end(), [context](const auto& data)
  {
    return QOpenGLContext::areSharing(const_cast<QOpenGLContext*>(context), const_cast<QOpenGLContext*>(data.first));
  });
  const PerContextData* shareData = shareDataIt != perContextData.end() ? &shareDataIt->second : nullptr;
  PerContextData& data = perContextData[context];
  data.referenceCounterIndex = shareData ? shareData->referenceCounterIndex : [this]
  {
    for(std::size_t i = 0; i < referenceCounters.size(); ++i)
      if(!referenceCounters[i])
        return i;
    referenceCounters.push_back(0);
    return referenceCounters.size() - 1;
  }();
  ++referenceCounters[data.referenceCounterIndex];

  ASSERT(!f);
  if(shareData)
    data.f = shareData->f;
  else
  {
    data.f = new QOpenGLFunctions_4_5_Core;
    VERIFY(data.f->initializeOpenGLFunctions());
  }
  f = data.f;

#ifndef NDEBUG
  // Report the errors and warnings of the OpenGL implementation (the debug output is a state of each context).
  f->glEnable(GL_DEBUG_OUTPUT);
  f->glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
  f->glDebugMessageCallback(&reportDebugMessage, nullptr);
  f->glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_LOW, 0, nullptr, GL_FALSE);
  f->glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
#endif

  // Enable depth test.
  f->glClearDepth(1.0f);
  f->glDepthFunc(GL_LEQUAL);
  f->glEnable(GL_DEPTH_TEST);

  // Avoid rendering the backside of surfaces.
  f->glEnable(GL_CULL_FACE);
  f->glCullFace(GL_BACK);
  f->glFrontFace(GL_CCW);

  // Set clear color.
  f->glClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);

  // Buffer objects are shared between contexts.
  if(shareData)
  {
    data.vbo = shareData->vbo;
    data.ebo = shareData->ebo;
    data.surfaceUBO = shareData->surfaceUBO;
  }
  else
  {
    f->glCreateBuffers(1, &data.vbo);
    f->glNamedBufferStorage(data.vbo, static_cast<GLsizeiptr>(vertexBufferTotalSize), nullptr, GL_DYNAMIC_STORAGE_BIT);
    for(const auto& category : vertexBuffers)
      for(const auto* buffer : category)
        f->glNamedBufferSubData(data.vbo, buffer->offset, buffer->size(), buffer->data);

    f->glCreateBuffers(1, &data.ebo);
    f->glNamedBufferStorage(data.ebo, static_cast<GLsizeiptr>(indexBufferTotalSize), nullptr, GL_DYNAMIC_STORAGE_BIT);
    for(const auto* buffer : indexBuffers)
      f->glNamedBufferSubData(data.ebo, buffer->offset, buffer->size(), buffer->indices.data());

    f->glCreateBuffers(1, &data.surfaceUBO);
    f->glNamedBufferStorage(data.surfaceUBO, static_cast<GLsizeiptr>(surfaces.size() * surfaceMemorySize), nullptr, GL_DYNAMIC_STORAGE_BIT);
    for(std::size_t i = 0; i < surfaces.size(); ++i)
    {
      static constexpr std::size_t verbatimPart = offsetof(Surface, ambient) - offsetof(Surface, albedo) + sizeof(Surface::ambient);
      unsigned char buf[surfaceMemorySize];
      std::memcpy(buf, &surfaces[i]->albedo, verbatimPart);
      *reinterpret_cast<unsigned int*>(buf + verbatimPart) = surfaces[i]->texture != nullptr;
      f->glNamedBufferSubData(data.surfaceUBO, i * surfaceMemorySize, surfaceMemorySize, buf);
    }
  }

  // VAOs are never shared between contexts, so they must be created here. All of them use the same VBO and EBO.
  data.vao.resize(vertexBuffers.size());
  f->glCreateVertexArrays(static_cast<GLsizei>(data.vao.size()), data.vao.data());
  for(std::size_t vaoIndex = 0; vaoIndex < data.vao.size(); ++vaoIndex)
  {
    f->glVertexArrayElementBuffer(data.vao[vaoIndex], data.ebo);
    vertexAttributeSetups[vaoIndex](*f, data.vao[vaoIndex], data.vbo);
  }

  // The VAOs and VBOs of controller drawings. The contents of the VBOs are uploaded for every frame.
  f->glCreateBuffers(1, &data.drawingVBO);
  f->glCreateVertexArrays(1, &data.drawingVAO);
  setupVertexAttributesDrawing(*f, data.drawingVAO, data.drawingVBO);
  // The instances use the meshes in the VBO and EBO of the context.
  f->glCreateBuffers(1, &data.drawingInstanceVBO);
  f->glCreateVertexArrays(1, &data.drawingInstanceVAO);
  f->glVertexArrayElementBuffer(data.drawingInstanceVAO, data.ebo);
  setupVertexAttributesPN(*f, data.drawingInstanceVAO, data.vbo);
  setupVertexAttributesDrawingInstances(*f, data.drawingInstanceVAO, data.drawingInstanceVBO);

  // Shaders and textures are shared as well.
  if(shareData)
  {
    data.textureIDs = shareData->textureIDs;
    data.shadowMapTextures = shareData->shadowMapTextures;
    data.shaders = shareData->shaders;
    data.drawingShaders = shareData->drawingShaders;
    data.drawingTextureCache = shareData->drawingTextureCache;
  }
  else
  {
    // Upload textures (with all mipmap levels). Textures that could not be loaded are left empty (no surface uses them).
    data.textureIDs.resize(textures.size());
    f->glCreateTextures(GL_TEXTURE_2D, static_cast<GLsizei>(textures.size()), data.textureIDs.data());
    for(const auto& pair : textures)
    {
      const Texture* texture = pair.second;
      if(texture->data.empty())
        continue;
      const GLuint id = data.textureIDs[texture->index];
      const auto width = static_cast<GLsizei>(texture->width);
      const auto height = static_cast<GLsizei>(texture->height);
      f->glTextureStorage2D(id, static_cast<GLsizei>(std::bit_width(std::max(texture->width, texture->height))), texture->hasAlpha ? GL_RGBA8 : GL_RGB8, width, height);
      f->glTextureSubImage2D(id, 0, 0, 0, width, height, GL_BGRA, GL_UNSIGNED_BYTE, texture->data.data());
      f->glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      f->glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      f->glGenerateTextureMipmap(id);
    }

    // Create the shadow maps. Everything in the lights' frustums is lit as long as they have not been rendered.
    data.shadowMapTextures.resize(shadowMaps.size());
    f->glCreateTextures(GL_TEXTURE_2D, static_cast<GLsizei>(shadowMaps.size()), data.shadowMapTextures.data());
    for(std::size_t i = 0; i < shadowMaps.size(); ++i)
    {
      const GLuint id = data.shadowMapTextures[i];
      f->glTextureStorage2D(id, 1, GL_DEPTH_COMPONENT32F, static_cast<GLsizei>(shadowMaps[i].width), static_cast<GLsizei>(shadowMaps[i].height));
      f->glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      f->glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      f->glTextureParameteri(id, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
      f->glTextureParameteri(id, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
      f->glTextureParameteri(id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
      f->glTextureParameteri(id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
      static const float borderColor[] = {1.f, 1.f, 1.f, 1.f};
      f->glTextureParameterfv(id, GL_TEXTURE_BORDER_COLOR, borderColor);
    }

    // Compile shaders.
    for(unsigned int i = 0; i < 8; ++i)
      data.shaders[i] = compileColorShader(i & 4, i & 2, i & 1);
    data.shaders[8] = compileDepthOnlyShader();
    data.drawingShaders[streamDrawingShader] = compileDrawingShader(drawingStreamVertexShaderSourceCode, drawingColorFragmentShaderSourceCode);
    data.drawingShaders[instanceDrawingShader] = compileDrawingShader(drawingInstanceVertexShaderSourceCode, drawingColorFragmentShaderSourceCode);
    data.drawingShaders[imageDrawingShader] = compileDrawingShader(drawingImageVertexShaderSourceCode, drawingImageFragmentShaderSourceCode);
    data.drawingTextureCache = std::make_shared<std::unordered_map<std::uint64_t, DrawingTexture>>();
  }

  // Framebuffer objects are not shared.
  data.shadowMapFBOs.resize(shadowMaps.size());
  f->glCreateFramebuffers(static_cast<GLsizei>(shadowMaps.size()), data.shadowMapFBOs.data());
  for(std::size_t i = 0; i < shadowMaps.size(); ++i)
  {
    const GLuint fbo = data.shadowMapFBOs[i];
    f->glNamedFramebufferTexture(fbo, GL_DEPTH_ATTACHMENT, data.shadowMapTextures[i], 0);
    f->glNamedFramebufferDrawBuffer(fbo, GL_NONE);
    f->glNamedFramebufferReadBuffer(fbo, GL_NONE);
    ASSERT(f->glCheckNamedFramebufferStatus(fbo, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    if(!shareData)
    {
      static const GLfloat farDepth = 1.f;
      f->glClearNamedFramebufferfv(fbo, GL_DEPTH, 0, &farDepth);
    }
  }

  f = nullptr;
}

void GraphicsContext::destroyGraphics()
{
  const auto* context = QOpenGLContext::currentContext();

  if(perContextData.find(context) == perContextData.end())
    return;

  auto& data = perContextData[context];
  if(--data.referenceCounter)
    return;

  data.f->glDeleteVertexArrays(static_cast<GLsizei>(data.vao.size()), data.vao.data());
  data.f->glDeleteVertexArrays(1, &data.drawingVAO);
  data.f->glDeleteBuffers(1, &data.drawingVBO);
  data.f->glDeleteVertexArrays(1, &data.drawingInstanceVAO);
  data.f->glDeleteBuffers(1, &data.drawingInstanceVBO);
  data.f->glDeleteFramebuffers(static_cast<GLsizei>(data.shadowMapFBOs.size()), data.shadowMapFBOs.data());
  if(--referenceCounters[data.referenceCounterIndex] == 0)
  {
    data.f->glDeleteBuffers(1, &data.vbo);
    data.f->glDeleteBuffers(1, &data.ebo);
    data.f->glDeleteBuffers(1, &data.surfaceUBO);
    data.f->glDeleteTextures(static_cast<GLsizei>(data.textureIDs.size()), data.textureIDs.data());
    data.f->glDeleteTextures(static_cast<GLsizei>(data.shadowMapTextures.size()), data.shadowMapTextures.data());
    for(const auto& shader : data.shaders)
      data.f->glDeleteProgram(shader.program);
    for(const auto& drawingShader : data.drawingShaders)
      data.f->glDeleteProgram(drawingShader.program);
    for(const auto& pair : *data.drawingTextureCache)
      data.f->glDeleteTextures(1, &pair.second.id);
    delete data.f;
  }

  perContextData.erase(context);
}

void GraphicsContext::updateShadowMaps(const std::function<void()>& draw, bool offscreen, bool force)
{
  ASSERT(!data);
  ASSERT(!f);
  ASSERT(!shader);

  if(shadowMaps.empty() || (!force && lastShadowMapUpdate == Simulation::simulation->simulationStep))
    return;
  lastShadowMapUpdate = Simulation::simulation->simulationStep;

  if(offscreen)
  {
    ASSERT(offscreenContext && offscreenSurface);
    offscreenContext->makeCurrent(offscreenSurface);
  }

  const auto it = perContextData.find(QOpenGLContext::currentContext());
  ASSERT(it != perContextData.end());
  data = &it->second;
  f = data->f;

  GLint previousFramebuffer;
  GLint previousViewport[4];
  f->glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFramebuffer);
  f->glGetIntegerv(GL_VIEWPORT, previousViewport);

  renderDepthOnly = true;
  // Rendering the back faces avoids most self-shadowing artifacts.
  f->glCullFace(GL_FRONT);
  for(std::size_t i = 0; i < shadowMaps.size(); ++i)
  {
    f->glBindFramebuffer(GL_FRAMEBUFFER, data->shadowMapFBOs[i]);
    f->glClear(GL_DEPTH_BUFFER_BIT);
    startRendering(shadowMaps[i].projection, shadowMaps[i].view, 0, 0, static_cast<int>(shadowMaps[i].width), static_cast<int>(shadowMaps[i].height));
    draw();
    finishRendering();
  }
  f->glCullFace(GL_BACK);
  renderDepthOnly = false;

  f->glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(previousFramebuffer));
  f->glViewport(previousViewport[0], previousViewport[1], previousViewport[2], previousViewport[3]);

  data = nullptr;
  f = nullptr;
}

void GraphicsContext::startRendering(const Matrix4f& projection, const Matrix4f& view, int viewportX, int viewportY, int viewportWidth, int viewportHeight, bool lighting, bool textures, bool smoothShading, bool fillPolygons)
{
  ASSERT(data);
  ASSERT(f);
  ASSERT(!shader);
  // Even if the caller wants textures to be active, we only use the corresponding shader if there are any textures
  // in the scene. Otherwise, at least the Apple implementation complains that a texture unit is used in a shader
  // without a bound texture.
  textures &= !data->textureIDs.empty();
  shader = &data->shaders[renderDepthOnly ? 8 : ((lighting ? 4 : 0) + (textures ? 2 : 0) + (smoothShading ? 1 : 0))];
  if(viewportX >= 0)
    f->glViewport(viewportX, viewportY, viewportWidth, viewportHeight);
  f->glPolygonMode(GL_FRONT_AND_BACK, (fillPolygons || renderDepthOnly) ? GL_FILL : GL_LINE);
  f->glUseProgram(shader->program);
  const Matrix4f pv = projection * view;
  f->glUniformMatrix4fv(shader->cameraPVLocation, 1, GL_FALSE, pv.data());
  if(shader->cameraPosLocation >= 0)
  {
    const Vector3f pos = -view.topLeftCorner<3, 3>().transpose() * view.topRightCorner<3, 1>();
    f->glUniform3fv(shader->cameraPosLocation, 1, pos.data());
  }
  for(std::size_t i = 0; i < shader->lightPVLocation.size(); ++i)
    if(shader->lightPVLocation[i] >= 0)
    {
      const Matrix4f lightPV = shadowMaps[i].projection * shadowMaps[i].view;
      f->glUniformMatrix4fv(shader->lightPVLocation[i], 1, GL_FALSE, lightPV.data());
      // Texture unit 0 is used for the albedo textures (see the bindings in the shaders).
      f->glBindTextureUnit(static_cast<GLuint>(i) + 1, data->shadowMapTextures[i]);
    }
  f->glBindBufferBase(GL_UNIFORM_BUFFER, 0, data->surfaceUBO);

  // Controller drawings might have changed these states in the meantime:
  data->boundVAO = 0;
  // If this shader uses textures, we bind some non-null texture initially. This prevents warnings
  // on Apple devices and signals setSurface that textures have to be bound.
  data->boundTexture = (textures && !renderDepthOnly) ? data->textureIDs.front() : 0;
  data->blendEnabled = false;
  f->glBindTextureUnit(0, data->boundTexture);
  f->glDisable(GL_BLEND);
}

void GraphicsContext::setForcedSurface(const Surface* surface)
{
  forcedSurface = surface;
  if(forcedSurface)
    setSurface(forcedSurface);
}

void GraphicsContext::draw(const Mesh* mesh, const ModelMatrix* modelMatrix, const Surface* surface)
{
  ASSERT(data);
  ASSERT(f);
  ASSERT(shader);
  const GLuint newVAO = data->vao[mesh->vertexBuffer->category];
  if(newVAO != data->boundVAO)
    f->glBindVertexArray((data->boundVAO = newVAO));
  f->glUniformMatrix4fv(shader->modelMatrixLocation, 1, GL_FALSE, modelMatrix->memory.data());
  if(!forcedSurface)
    setSurface(surface);
  if(mesh->indexBuffer)
    f->glDrawElementsBaseVertex(getPrimitiveType(mesh->topology), static_cast<GLsizei>(mesh->indexBuffer->count), GL_UNSIGNED_INT, reinterpret_cast<void*>(mesh->indexBuffer->offset), static_cast<GLint>(mesh->vertexBuffer->base));
  else
    f->glDrawArrays(getPrimitiveType(mesh->topology), static_cast<GLint>(mesh->vertexBuffer->base), static_cast<GLsizei>(mesh->vertexBuffer->count));
}

void GraphicsContext::finishRendering()
{
  ASSERT(data);
  ASSERT(f);
  ASSERT(shader);
  ASSERT(!forcedSurface);
  shader = nullptr;
}

bool GraphicsContext::startOffscreenRendering(int width, int height, bool depthOnly)
{
  ASSERT(offscreenContext && offscreenSurface);
  ASSERT(width > 0 && height > 0);

  offscreenContext->makeCurrent(offscreenSurface);

  // Considering weak graphics cards glClear is faster when the color and depth buffers are not greater then they have to be.
  // So we create an individual buffer for each size in demand.

  auto it = offscreenBuffers.find(width << 16 | height);
  if(it == offscreenBuffers.end())
  {
    QOpenGLFramebufferObject*& buffer = offscreenBuffers[width << 16 | height];

    buffer = new QOpenGLFramebufferObject(width, height, QOpenGLFramebufferObject::Depth);
    if(!buffer->isValid())
    {
      delete buffer;
      buffer = nullptr;
      return false;
    }
  }
  else if(!it->second || !it->second->bind())
    return false;

  ASSERT(!data);
  ASSERT(!f);
  ASSERT(!shader);
  data = &perContextData[offscreenContext];
  f = data->f;

  f->glClear((depthOnly ? 0 : GL_COLOR_BUFFER_BIT) | GL_DEPTH_BUFFER_BIT);
  renderDepthOnly = depthOnly;

  return true;
}

void GraphicsContext::finishOffscreenRendering(void* image, int w, int h)
{
  ASSERT(data);
  ASSERT(f);
  ASSERT(!shader);

  if(renderDepthOnly)
  {
    f->glPixelStorei(GL_PACK_ALIGNMENT, w * 4 & (8 - 1) ? 4 : 8);
    f->glReadPixels(0, 0, w, h, GL_DEPTH_COMPONENT, GL_FLOAT, image);
  }
  else
  {
    const int lineSize = w * 3;
    f->glPixelStorei(GL_PACK_ALIGNMENT, lineSize & (8 - 1) ? (lineSize & (4 - 1) ? 1 : 4) : 8);
    f->glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, image);
  }

  data = nullptr;
  f = nullptr;
}

bool GraphicsContext::startExternalRendering(bool multisample)
{
  ASSERT(!data);
  ASSERT(!f);
  ASSERT(!shader);
  if(auto it = perContextData.find(QOpenGLContext::currentContext()); it != perContextData.end())
    data = &it->second;
  else
    return false;

  f = data->f;

  if(multisample)
    f->glEnable(GL_MULTISAMPLE);
  else
    f->glDisable(GL_MULTISAMPLE);
  f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  renderDepthOnly = false;

  return true;
}

void GraphicsContext::finishExternalRendering()
{
  ASSERT(data);
  ASSERT(f);
  ASSERT(!shader);
  data = nullptr;
  f = nullptr;
}

void GraphicsContext::prepareDrawings(const DrawingList& drawings)
{
  ASSERT(data);
  ASSERT(f);
  ASSERT(!shader);
  ASSERT(data->drawingTextures.empty());

  // Release the textures of drawings that do not exist anymore.
  auto& textureCache = *data->drawingTextureCache;
  for(auto i = textureCache.begin(); i != textureCache.end();)
    if(i->second.drawing.expired())
    {
      f->glDeleteTextures(1, &i->second.id);
      i = textureCache.erase(i);
    }
    else
      ++i;

  if(drawings.numOfVertices)
  {
    f->glNamedBufferData(data->drawingVBO, static_cast<GLsizeiptr>(drawings.numOfVertices * sizeof(PreparedDrawing::Vertex)), nullptr, GL_STREAM_DRAW);
    for(const DrawingList::Item& item : drawings.items)
      if(!item.drawing->vertices.empty())
        f->glNamedBufferSubData(data->drawingVBO, static_cast<GLintptr>(item.firstVertex * sizeof(PreparedDrawing::Vertex)),
                                static_cast<GLsizeiptr>(item.drawing->vertices.size() * sizeof(PreparedDrawing::Vertex)), item.drawing->vertices.data());
  }
  if(drawings.numOfInstances)
  {
    f->glNamedBufferData(data->drawingInstanceVBO, static_cast<GLsizeiptr>(drawings.numOfInstances * sizeof(PreparedDrawing::Instance)), nullptr, GL_STREAM_DRAW);
    for(const DrawingList::Item& item : drawings.items)
      if(!item.drawing->instances.empty())
        f->glNamedBufferSubData(data->drawingInstanceVBO, static_cast<GLintptr>(item.firstInstance * sizeof(PreparedDrawing::Instance)),
                                static_cast<GLsizeiptr>(item.drawing->instances.size() * sizeof(PreparedDrawing::Instance)), item.drawing->instances.data());
  }

  // Images are uploaded when they are drawn for the first time.
  data->drawingTextures.reserve(drawings.numOfImages);
  for(const DrawingList::Item& item : drawings.items)
    for(const PreparedDrawing::Image& image : item.drawing->images)
    {
      const auto [i, inserted] = textureCache.try_emplace(image.id);
      if(inserted)
      {
        i->second.drawing = item.drawing;
        GLuint& id = i->second.id;
        f->glCreateTextures(GL_TEXTURE_2D, 1, &id);
        f->glTextureStorage2D(id, 1, GL_RGBA8, static_cast<GLsizei>(image.width), static_cast<GLsizei>(image.height));
        f->glTextureSubImage2D(id, 0, 0, 0, static_cast<GLsizei>(image.width), static_cast<GLsizei>(image.height), GL_RGBA, GL_UNSIGNED_BYTE, image.rgba.data());
        f->glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        f->glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        f->glTextureParameteri(id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        f->glTextureParameteri(id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      }
      data->drawingTextures.push_back(i->second.id);
    }
}

void GraphicsContext::drawDrawings(const DrawingList& drawings, const Matrix4f& projection, const Matrix4f& view, float alpha, bool opaquePass, bool fillPolygons)
{
  ASSERT(data);
  ASSERT(f);
  ASSERT(!shader);
  ASSERT(drawingSphereMesh && drawingCylinderMesh && drawingQuadMesh);

  const Matrix4f pv = projection * view;
  const DrawingShader& streamShader = data->drawingShaders[streamDrawingShader];
  const DrawingShader& instanceShader = data->drawingShaders[instanceDrawingShader];
  const DrawingShader& imageShader = data->drawingShaders[imageDrawingShader];

  f->glPolygonMode(GL_FRONT_AND_BACK, fillPolygons ? GL_FILL : GL_LINE);
  f->glEnable(GL_BLEND);
  f->glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);
  f->glEnable(GL_PROGRAM_POINT_SIZE);

  // Draws instances of a mesh of the context (only the positions of its vertices are used).
  const auto drawInstances = [this](const Mesh* mesh, std::size_t firstInstance, std::size_t numOfInstances)
  {
    f->glDrawElementsInstancedBaseVertexBaseInstance(getPrimitiveType(mesh->topology), static_cast<GLsizei>(mesh->indexBuffer->count), GL_UNSIGNED_INT, reinterpret_cast<void*>(mesh->indexBuffer->offset), static_cast<GLsizei>(numOfInstances), static_cast<GLint>(mesh->vertexBuffer->base), static_cast<GLuint>(firstInstance));
  };

  for(const DrawingList::Item& item : drawings.items)
  {
    const PreparedDrawing& drawing = *item.drawing;
    if(drawing.renderOptions & (SimRobotCore3::Drawing::disableTransparency | SimRobotCore3::Drawing::disableOpacity))
    {
      if(opaquePass != ((drawing.renderOptions & SimRobotCore3::Drawing::disableTransparency) != 0))
        continue;
    }
    const bool disableDepth = drawing.renderOptions & SimRobotCore3::Drawing::disableDepth;
    if(disableDepth)
      f->glDepthMask(GL_FALSE);

    const Matrix4f pvm = pv * item.transformation;

    if(!drawing.vertices.empty())
    {
      const GLint firstLineVertex = static_cast<GLint>(item.firstVertex);
      const GLint firstPoint = firstLineVertex + static_cast<GLint>(drawing.numOfLineVertices);
      const GLint firstTriangleVertex = firstPoint + static_cast<GLint>(drawing.numOfPoints);
      f->glBindVertexArray(data->drawingVAO);
      f->glUseProgram(streamShader.program);
      f->glUniformMatrix4fv(streamShader.pvmLocation, 1, GL_FALSE, pvm.data());
      f->glUniform1f(streamShader.alphaLocation, alpha);
      if(drawing.numOfLineVertices)
        f->glDrawArrays(GL_LINES, firstLineVertex, static_cast<GLsizei>(drawing.numOfLineVertices));
      if(drawing.numOfPoints)
        f->glDrawArrays(GL_POINTS, firstPoint, static_cast<GLsizei>(drawing.numOfPoints));
      if(drawing.numOfTriangleVertices)
        f->glDrawArrays(GL_TRIANGLES, firstTriangleVertex, static_cast<GLsizei>(drawing.numOfTriangleVertices));
    }

    if(!drawing.instances.empty())
    {
      f->glBindVertexArray(data->drawingInstanceVAO);
      f->glUseProgram(instanceShader.program);
      f->glUniformMatrix4fv(instanceShader.pvmLocation, 1, GL_FALSE, pvm.data());
      f->glUniform1f(instanceShader.alphaLocation, alpha);
      if(drawing.numOfSpheres)
        drawInstances(drawingSphereMesh, item.firstInstance, drawing.numOfSpheres);
      if(drawing.numOfCylinders)
        drawInstances(drawingCylinderMesh, item.firstInstance + drawing.numOfSpheres, drawing.numOfCylinders);
    }

    if(!drawing.images.empty())
    {
      f->glBindVertexArray(data->vao[drawingQuadMesh->vertexBuffer->category]);
      f->glUseProgram(imageShader.program);
      f->glUniform1f(imageShader.alphaLocation, alpha);
      f->glDisable(GL_CULL_FACE);
      for(std::size_t i = 0; i < drawing.images.size(); ++i)
      {
        const Matrix4f imagePVM = pvm * drawing.images[i].transformation;
        f->glUniformMatrix4fv(imageShader.pvmLocation, 1, GL_FALSE, imagePVM.data());
        f->glBindTextureUnit(0, data->drawingTextures[item.firstImage + i]);
        f->glDrawElementsBaseVertex(getPrimitiveType(drawingQuadMesh->topology), static_cast<GLsizei>(drawingQuadMesh->indexBuffer->count), GL_UNSIGNED_INT, reinterpret_cast<void*>(drawingQuadMesh->indexBuffer->offset), static_cast<GLint>(drawingQuadMesh->vertexBuffer->base));
      }
      f->glBindTextureUnit(0, 0);
      f->glEnable(GL_CULL_FACE);
    }

    if(disableDepth)
      f->glDepthMask(GL_TRUE);
  }

  // Restore the state that the other rendering expects.
  f->glDisable(GL_PROGRAM_POINT_SIZE);
  f->glDisable(GL_BLEND);
  f->glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  f->glBindVertexArray(0);
  f->glUseProgram(0);
  data->boundVAO = 0;
  data->boundTexture = 0;
  data->blendEnabled = false;
}

void GraphicsContext::finishDrawings()
{
  ASSERT(data);
  ASSERT(f);
  data->drawingTextures.clear();
}

void GraphicsContext::clearDepth()
{
  ASSERT(data);
  ASSERT(f);
  ASSERT(!shader);
  f->glClear(GL_DEPTH_BUFFER_BIT);
}

void GraphicsContext::setSurface(const Surface* surface)
{
  ASSERT(data);
  ASSERT(shader);
  ASSERT(f);
  if(renderDepthOnly)
    return;
  const GLuint newTexture = surface->texture ? data->textureIDs[surface->texture->index] : 0;
  // data->boundTexture is != 0 here iff the shader uses a texture. If the new surface doesn't have
  // a texture, the old one must stay bound.
  if(newTexture && data->boundTexture && newTexture != data->boundTexture)
    f->glBindTextureUnit(0, (data->boundTexture = newTexture));
  if(shader->surfaceIndexLocation >= 0)
    f->glUniform1ui(shader->surfaceIndexLocation, static_cast<GLuint>(surface->index));
  const bool newBlendState = surface->texture ? surface->texture->hasAlpha : (surface->alpha < 1.f);
  if(newBlendState && !data->blendEnabled)
  {
    f->glEnable(GL_BLEND);
    f->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    data->blendEnabled = true;
  }
  else if(!newBlendState && data->blendEnabled)
  {
    f->glDisable(GL_BLEND);
    data->blendEnabled = false;
  }
}

GLuint GraphicsContext::compileShader(const std::vector<const char*>& vertexShaderSources, const std::vector<const char*>& fragmentShaderSources)
{
  ASSERT(f);
#ifndef NDEBUG
  GLint success = 0;
#endif

  const GLuint vertexShader = f->glCreateShader(GL_VERTEX_SHADER);
  ASSERT(vertexShader > 0);
  f->glShaderSource(vertexShader, static_cast<GLsizei>(vertexShaderSources.size()), vertexShaderSources.data(), nullptr);
  f->glCompileShader(vertexShader);
#ifndef NDEBUG
  f->glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
  if(!success)
  {
    char log[512];
    f->glGetShaderInfoLog(vertexShader, sizeof(log), nullptr, log);
    TRACE("%s", log);
    ASSERT(false);
  }
#endif

  const GLuint fragmentShader = f->glCreateShader(GL_FRAGMENT_SHADER);
  ASSERT(fragmentShader > 0);
  f->glShaderSource(fragmentShader, static_cast<GLsizei>(fragmentShaderSources.size()), fragmentShaderSources.data(), nullptr);
  f->glCompileShader(fragmentShader);
#ifndef NDEBUG
  f->glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
  if(!success)
  {
    char log[512];
    f->glGetShaderInfoLog(fragmentShader, sizeof(log), nullptr, log);
    TRACE("%s", log);
    ASSERT(false);
  }
#endif

  const GLuint program = f->glCreateProgram();
  ASSERT(program > 0);
  f->glAttachShader(program, vertexShader);
  f->glAttachShader(program, fragmentShader);
  f->glLinkProgram(program);
#ifndef NDEBUG
  f->glGetProgramiv(program, GL_LINK_STATUS, &success);
  if(!success)
  {
    char log[512];
    f->glGetProgramInfoLog(program, sizeof(log), nullptr, log);
    TRACE("%s", log);
    ASSERT(false);
  }
#endif

  f->glDeleteShader(vertexShader);
  f->glDeleteShader(fragmentShader);

  return program;
}

GraphicsContext::Shader GraphicsContext::compileColorShader(bool lighting, bool textures, bool smooth)
{
  const char* versionSourceCode = "#version 450 core\n";

  std::string defines;
  defines += "#define NUM_OF_SURFACES " + std::to_string(surfaces.size()) + "\n";
  if(lighting)
    defines += "#define WITH_LIGHTING\n";
  if(textures)
    defines += "#define WITH_TEXTURES\n";
  if(smooth)
    defines += "#define NORMAL_QUALIFIER smooth\n";
  else
    defines += "#define NORMAL_QUALIFIER flat\n";

  const std::string globalAmbientLightCode = "#define GLOBAL_AMBIENT_LIGHT vec3(" + std::to_string(globalAmbientLight[0]) + ", " + std::to_string(globalAmbientLight[1]) + ", " + std::to_string(globalAmbientLight[2]) + ")\n";
  std::string lightDeclarationsCode = "#define DECLARE_LIGHTS";
  for(std::size_t i = 0; i < shadowMaps.size(); ++i)
    lightDeclarationsCode += " uniform mat4 lightPV" + std::to_string(i) + "; layout(binding = " + std::to_string(i + 1) + ") uniform sampler2DShadow shadowMap" + std::to_string(i) + ";";
  std::string lightCalculationsCode = "#define CALCULATE_LIGHTS";
  const auto toVec3 = [](const float* v)
  {
    return "vec3(" + std::to_string(v[0]) + ", " + std::to_string(v[1]) + ", " + std::to_string(v[2]) + ")";
  };
  for(std::size_t i = 0; i < lights.size(); ++i)
  {
    const LightData& light = lights[i];
    const std::string name = "light" + std::to_string(i);
    // The direction from the fragment to the light (to determine the shadow bias).
    std::string lightDirection;
    switch(light.type)
    {
      case LightData::directional:
        lightDeclarationsCode += " const DirLight " + name + " = DirLight(" + toVec3(light.color) + ", " + toVec3(light.direction) + ");";
        lightDirection = "normalize(-" + name + ".direction)";
        break;
      case LightData::spot:
        lightDeclarationsCode += " const SpotLight " + name + " = SpotLight(" + toVec3(light.color) + ", " + toVec3(light.position) + ", " + std::to_string(light.constantAttenuation) + ", " + std::to_string(light.linearAttenuation) + ", " + std::to_string(light.quadraticAttenuation) + ", " + toVec3(light.direction) + ", " + std::to_string(light.cutoff) + ");";
        lightDirection = "normalize(" + name + ".position - FragPosInWorld)";
        break;
      case LightData::point:
        lightDeclarationsCode += " const PointLight " + name + " = PointLight(" + toVec3(light.color) + ", " + toVec3(light.position) + ", " + std::to_string(light.constantAttenuation) + ", " + std::to_string(light.linearAttenuation) + ", " + std::to_string(light.quadraticAttenuation) + ");";
        lightDirection = "normalize(" + name + ".position - FragPosInWorld)";
        break;
    }
    std::string visibility = "1.0";
    if(light.shadowMap)
    {
      const std::string index = std::to_string(*light.shadowMap);
      visibility = "illumination(shadowMap" + index + ", lightPV" + index + ", " + lightDirection + ", normalizedNormal)";
    }
    switch(light.type)
    {
      case LightData::directional:
        lightCalculationsCode += " Lo += calcDirLight(" + name + ", normalizedNormal, viewDir, albedo, F0, " + visibility + ");";
        break;
      case LightData::spot:
        lightCalculationsCode += " Lo += calcSpotLight(" + name + ", FragPosInWorld, normalizedNormal, viewDir, albedo, F0, " + visibility + ");";
        break;
      case LightData::point:
        lightCalculationsCode += " Lo += calcPointLight(" + name + ", FragPosInWorld, normalizedNormal, viewDir, albedo, F0, " + visibility + ");";
        break;
    }
  }
  lightDeclarationsCode += "\n";
  lightCalculationsCode += "\n";

  Shader shader;
  shader.program = compileShader({versionSourceCode, defines.c_str(), vertexShaderSourceCode}, {versionSourceCode, defines.c_str(), globalAmbientLightCode.c_str(), lightDeclarationsCode.c_str(), lightCalculationsCode.c_str(), fragmentShaderSourceCode});

  ASSERT(f);
  shader.cameraPVLocation = f->glGetUniformLocation(shader.program, "cameraPV");
  shader.cameraPosLocation = f->glGetUniformLocation(shader.program, "cameraPos");
  shader.modelMatrixLocation = f->glGetUniformLocation(shader.program, "modelMatrix");
  shader.surfaceIndexLocation = f->glGetUniformLocation(shader.program, "surfaceIndex");
  shader.lightPVLocation.resize(shadowMaps.size());
  for(std::size_t i = 0; i < shadowMaps.size(); ++i)
    shader.lightPVLocation[i] = f->glGetUniformLocation(shader.program, ("lightPV" + std::to_string(i)).c_str());
  return shader;
}

GraphicsContext::Shader GraphicsContext::compileDepthOnlyShader()
{
  const char* versionSourceCode = "#version 450 core\n";

  Shader shader;
  shader.program = compileShader({versionSourceCode, depthOnlyVertexShaderSourceCode}, {versionSourceCode, depthOnlyFragmentShaderSourceCode});

  ASSERT(f);
  shader.cameraPVLocation = f->glGetUniformLocation(shader.program, "cameraPV");
  shader.modelMatrixLocation = f->glGetUniformLocation(shader.program, "modelMatrix");
  return shader;
}

GraphicsContext::DrawingShader GraphicsContext::compileDrawingShader(const char* vertexShaderSourceCode, const char* fragmentShaderSourceCode)
{
  const char* versionSourceCode = "#version 450 core\n";

  DrawingShader shader;
  shader.program = compileShader({versionSourceCode, vertexShaderSourceCode}, {versionSourceCode, fragmentShaderSourceCode});

  ASSERT(f);
  shader.pvmLocation = f->glGetUniformLocation(shader.program, "pvm");
  shader.alphaLocation = f->glGetUniformLocation(shader.program, "alphaScale");
  return shader;
}

QOpenGLFunctions_4_5_Core* GraphicsContext::getOpenGLFunctions() const
{
  if(auto it = perContextData.find(QOpenGLContext::currentContext()); it != perContextData.end())
    return it->second.f;
  else
    return nullptr;
}

GLenum GraphicsContext::getPrimitiveType(PrimitiveTopology topology)
{
  switch(topology)
  {
    case pointList:
      return GL_POINTS;
    case lineList:
      return GL_LINES;
    case triangleList:
      return GL_TRIANGLES;
  }
  ASSERT(false);
  return GL_TRIANGLES;
}

void GraphicsContext::setupVertexAttribute(QOpenGLFunctions_4_5_Core& functions, GLuint vao, GLuint index, GLuint binding, GLint size, GLenum type, GLboolean normalized, std::size_t offset)
{
  functions.glEnableVertexArrayAttrib(vao, index);
  functions.glVertexArrayAttribFormat(vao, index, size, type, normalized, static_cast<GLuint>(offset));
  functions.glVertexArrayAttribBinding(vao, index, binding);
}

void GraphicsContext::setupVertexAttributesPN(QOpenGLFunctions_4_5_Core& functions, GLuint vao, GLuint vbo)
{
  functions.glVertexArrayVertexBuffer(vao, 0, vbo, 0, 6 * sizeof(GLfloat));
  setupVertexAttribute(functions, vao, 0, 0, 3, GL_FLOAT, GL_FALSE, 0);
  setupVertexAttribute(functions, vao, 1, 0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat));
  // There are no texture coordinates, but shaders that use them can still be used.
  setupVertexAttribute(functions, vao, 2, 0, 2, GL_FLOAT, GL_FALSE, 0);
}

void GraphicsContext::setupVertexAttributesPNT(QOpenGLFunctions_4_5_Core& functions, GLuint vao, GLuint vbo)
{
  functions.glVertexArrayVertexBuffer(vao, 0, vbo, 0, 8 * sizeof(GLfloat));
  setupVertexAttribute(functions, vao, 0, 0, 3, GL_FLOAT, GL_FALSE, 0);
  setupVertexAttribute(functions, vao, 1, 0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat));
  setupVertexAttribute(functions, vao, 2, 0, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat));
}

void GraphicsContext::setupVertexAttributesDrawing(QOpenGLFunctions_4_5_Core& functions, GLuint vao, GLuint vbo)
{
  functions.glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(PreparedDrawing::Vertex));
  setupVertexAttribute(functions, vao, 0, 0, 3, GL_FLOAT, GL_FALSE, offsetof(PreparedDrawing::Vertex, position));
  setupVertexAttribute(functions, vao, 1, 0, 4, GL_UNSIGNED_BYTE, GL_TRUE, offsetof(PreparedDrawing::Vertex, color));
  setupVertexAttribute(functions, vao, 2, 0, 1, GL_FLOAT, GL_FALSE, offsetof(PreparedDrawing::Vertex, size));
}

void GraphicsContext::setupVertexAttributesDrawingInstances(QOpenGLFunctions_4_5_Core& functions, GLuint vao, GLuint vbo)
{
  functions.glVertexArrayVertexBuffer(vao, 1, vbo, 0, sizeof(PreparedDrawing::Instance));
  functions.glVertexArrayBindingDivisor(vao, 1, 1);
  // A matrix occupies one location per column.
  for(GLuint i = 0; i < 4; ++i)
    setupVertexAttribute(functions, vao, 3 + i, 1, 4, GL_FLOAT, GL_FALSE, offsetof(PreparedDrawing::Instance, transformation) + i * 4 * sizeof(GLfloat));
  setupVertexAttribute(functions, vao, 7, 1, 2, GL_FLOAT, GL_FALSE, offsetof(PreparedDrawing::Instance, radii));
  setupVertexAttribute(functions, vao, 8, 1, 4, GL_UNSIGNED_BYTE, GL_TRUE, offsetof(PreparedDrawing::Instance, color));
}

#endif
