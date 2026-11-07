/**
 * @file Graphics/OpenGL/GraphicsContext.cpp
 *
 * This file implements a class that handles graphics using OpenGL 3.3 Core.
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
#include <QOpenGLFunctions_3_3_Core>
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
uniform sampler2D albedoTexture;
#endif
layout (std140) uniform Surfaces
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

// The shaders of controller drawings (which are not lit): vertices with colors and sizes, a mesh with a uniform color, and a textured mesh.
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

static const char* drawingFlatVertexShaderSourceCode = R"glsl(
layout(location = 0) in vec3 inPos;

uniform mat4 pvm;
uniform vec4 color;
uniform float alphaScale;

out vec4 Color;

void main()
{
  Color = vec4(color.rgb, color.a * alphaScale);
  gl_Position = pvm * vec4(inPos, 1.0);
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
uniform sampler2D tex;
uniform float alphaScale;
out vec4 FragColor;

void main()
{
  FragColor = vec4(texture(tex, TexCoords).rgb, alphaScale);
}
)glsl";

static const char* depthOnlyFragmentShaderSourceCode = R"glsl(
void main()
{
  // TODO: Later, this shader could compute gl_FragDepth including the postprocessing from the DepthImageSensor.
}
)glsl";

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

void GraphicsContext::compile()
{
  layoutResources();

  ASSERT(!offscreenSurface && !offscreenContext);

  offscreenSurface = new QOffscreenSurface;
  offscreenSurface->create();

  offscreenContext = new QOpenGLContext;
  offscreenContext->setShareContext(QOpenGLContext::globalShareContext());
  VERIFY(offscreenContext->create());
  offscreenContext->makeCurrent(offscreenSurface);

  createGraphics();
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
    data.f = new QOpenGLFunctions_3_3_Core;
    data.f->initializeOpenGLFunctions();
  }
  f = data.f;

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
    f->glGenBuffers(1, &data.vbo);
    f->glGenBuffers(1, &data.ebo);
    f->glGenBuffers(1, &data.surfaceUBO);
    // Data can't be uploaded here because to bind the EBO, a VAO must be bound.
  }

  // All vertex attributes use the same VBO.
  f->glBindBuffer(GL_ARRAY_BUFFER, data.vbo);

  // VAOs are never shared between contexts, so they must be created here.
  data.vao.resize(vertexBuffers.size());
  f->glGenVertexArrays(static_cast<GLsizei>(data.vao.size()), data.vao.data());
  for(std::size_t vaoIndex = 0; vaoIndex < data.vao.size(); ++vaoIndex)
  {
    f->glBindVertexArray(data.vao[vaoIndex]);
    f->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.ebo);
    vertexAttributeSetups[vaoIndex](*f);
  }

  // Upload buffer data, now that also the EBO is bound.
  if(!shareData)
  {
    f->glBufferData(GL_ARRAY_BUFFER, vertexBufferTotalSize, nullptr, GL_STATIC_DRAW);
    for(const auto& category : vertexBuffers)
      for(const auto* buffer : category)
        f->glBufferSubData(GL_ARRAY_BUFFER, buffer->offset, buffer->size(), buffer->data);

    f->glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBufferTotalSize, nullptr, GL_STATIC_DRAW);
    for(const auto* buffer : indexBuffers)
      f->glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, buffer->offset, buffer->size(), buffer->indices.data());

    f->glBindBuffer(GL_UNIFORM_BUFFER, data.surfaceUBO);
    f->glBufferData(GL_UNIFORM_BUFFER, surfaces.size() * surfaceMemorySize, nullptr, GL_STATIC_DRAW);
    for(std::size_t i = 0; i < surfaces.size(); ++i)
    {
      static constexpr std::size_t verbatimPart = offsetof(Surface, ambient) - offsetof(Surface, albedo) + sizeof(Surface::ambient);
      unsigned char buf[surfaceMemorySize];
      std::memcpy(buf, &surfaces[i]->albedo, verbatimPart);
      *reinterpret_cast<unsigned int*>(buf + verbatimPart) = surfaces[i]->texture != nullptr;
      f->glBufferSubData(GL_UNIFORM_BUFFER, i * surfaceMemorySize, surfaceMemorySize, buf);
    }
    f->glBindBuffer(GL_UNIFORM_BUFFER, 0);
  }

  f->glBindBuffer(GL_ARRAY_BUFFER, 0);
  f->glBindVertexArray(0);

  // The VAO and VBO of controller drawings. Their contents are uploaded for every frame.
  f->glGenBuffers(1, &data.drawingVBO);
  f->glBindBuffer(GL_ARRAY_BUFFER, data.drawingVBO);
  f->glGenVertexArrays(1, &data.drawingVAO);
  f->glBindVertexArray(data.drawingVAO);
  f->glEnableVertexAttribArray(0);
  f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(DrawingList::Vertex), reinterpret_cast<void*>(offsetof(DrawingList::Vertex, position)));
  f->glEnableVertexAttribArray(1);
  f->glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(DrawingList::Vertex), reinterpret_cast<void*>(offsetof(DrawingList::Vertex, color)));
  f->glEnableVertexAttribArray(2);
  f->glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(DrawingList::Vertex), reinterpret_cast<void*>(offsetof(DrawingList::Vertex, size)));
  f->glBindBuffer(GL_ARRAY_BUFFER, 0);
  f->glBindVertexArray(0);

  // Shaders and textures are shared as well.
  if(shareData)
  {
    data.textureIDs = shareData->textureIDs;
    data.shadowMapTextures = shareData->shadowMapTextures;
    data.shaders = shareData->shaders;
    data.drawingShaders = shareData->drawingShaders;
  }
  else
  {
    // Upload textures.
    data.textureIDs.resize(textures.size());
    f->glGenTextures(static_cast<GLsizei>(textures.size()), data.textureIDs.data());
    for(const auto& pair : textures)
    {
      const Texture* texture = pair.second;
      f->glBindTexture(GL_TEXTURE_2D, data.textureIDs[texture->index]);
      f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      f->glTexImage2D(GL_TEXTURE_2D, 0, texture->hasAlpha ? GL_RGBA : GL_RGB, texture->width, texture->height, 0, GL_BGRA, GL_UNSIGNED_BYTE, texture->data.data());
      f->glGenerateMipmap(GL_TEXTURE_2D);
    }

    // Create the shadow maps. Everything in the lights' frustums is lit as long as they have not been rendered.
    data.shadowMapTextures.resize(shadowMaps.size());
    f->glGenTextures(static_cast<GLsizei>(shadowMaps.size()), data.shadowMapTextures.data());
    for(std::size_t i = 0; i < shadowMaps.size(); ++i)
    {
      f->glBindTexture(GL_TEXTURE_2D, data.shadowMapTextures[i]);
      f->glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, shadowMaps[i].width, shadowMaps[i].height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
      f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
      f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
      f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
      f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
      static const float borderColor[] = {1.f, 1.f, 1.f, 1.f};
      f->glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    }

    // Compile shaders.
    for(unsigned int i = 0; i < 8; ++i)
      data.shaders[i] = compileColorShader(i & 4, i & 2, i & 1);
    data.shaders[8] = compileDepthOnlyShader();

    const char* versionSourceCode = "#version 330 core\n";
    const auto compileDrawingShader = [&](const char* vertexShaderSourceCode, const char* fragmentShaderSourceCode)
    {
      DrawingShader drawingShader;
      drawingShader.program = compileShader({versionSourceCode, vertexShaderSourceCode}, {versionSourceCode, fragmentShaderSourceCode});
      drawingShader.pvmLocation = f->glGetUniformLocation(drawingShader.program, "pvm");
      drawingShader.colorLocation = f->glGetUniformLocation(drawingShader.program, "color");
      drawingShader.alphaLocation = f->glGetUniformLocation(drawingShader.program, "alphaScale");
      drawingShader.textureLocation = f->glGetUniformLocation(drawingShader.program, "tex");
      return drawingShader;
    };
    data.drawingShaders[0] = compileDrawingShader(drawingStreamVertexShaderSourceCode, drawingColorFragmentShaderSourceCode);
    data.drawingShaders[1] = compileDrawingShader(drawingFlatVertexShaderSourceCode, drawingColorFragmentShaderSourceCode);
    data.drawingShaders[2] = compileDrawingShader(drawingImageVertexShaderSourceCode, drawingImageFragmentShaderSourceCode);
  }

  // Framebuffer objects are not shared.
  GLint previousFramebuffer;
  f->glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFramebuffer);
  data.shadowMapFBOs.resize(shadowMaps.size());
  f->glGenFramebuffers(static_cast<GLsizei>(shadowMaps.size()), data.shadowMapFBOs.data());
  for(std::size_t i = 0; i < shadowMaps.size(); ++i)
  {
    f->glBindFramebuffer(GL_FRAMEBUFFER, data.shadowMapFBOs[i]);
    f->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, data.shadowMapTextures[i], 0);
    f->glDrawBuffer(GL_NONE);
    f->glReadBuffer(GL_NONE);
    ASSERT(f->glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    if(!shareData)
      f->glClear(GL_DEPTH_BUFFER_BIT);
  }
  f->glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(previousFramebuffer));

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
  for(std::size_t i = 0; i < shader->shadowMapLocation.size(); ++i)
  {
    if(shader->lightPVLocation[i] >= 0)
    {
      const Matrix4f lightPV = shadowMaps[i].projection * shadowMaps[i].view;
      f->glUniformMatrix4fv(shader->lightPVLocation[i], 1, GL_FALSE, lightPV.data());
    }
    if(shader->shadowMapLocation[i] >= 0)
    {
      // Texture unit 0 is used for the albedo textures.
      f->glUniform1i(shader->shadowMapLocation[i], static_cast<GLint>(i) + 1);
      f->glActiveTexture(GL_TEXTURE1 + static_cast<GLenum>(i));
      f->glBindTexture(GL_TEXTURE_2D, data->shadowMapTextures[i]);
    }
  }
  f->glActiveTexture(GL_TEXTURE0);
  f->glBindBufferBase(GL_UNIFORM_BUFFER, 0, data->surfaceUBO);

  // Controller drawings might have changed these states in the meantime:
  data->boundVAO = 0;
  // If this shader uses textures, we bind some non-null texture initially. This prevents warnings
  // on Apple devices and signals setSurface that textures have to be bound.
  data->boundTexture = (textures && !renderDepthOnly) ? data->textureIDs.front() : 0;
  data->blendEnabled = false;
  f->glBindTexture(GL_TEXTURE_2D, data->boundTexture);
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

bool GraphicsContext::startExternalRendering()
{
  ASSERT(!data);
  ASSERT(!f);
  ASSERT(!shader);
  if(auto it = perContextData.find(QOpenGLContext::currentContext()); it != perContextData.end())
    data = &it->second;
  else
    return false;

  f = data->f;

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

  // The vertices are stored one after the other: lines, points, triangles.
  if(!drawings.lines.empty() || !drawings.points.empty() || !drawings.triangles.empty())
  {
    f->glBindBuffer(GL_ARRAY_BUFFER, data->drawingVBO);
    const std::size_t numOfVertices = drawings.lines.size() + drawings.points.size() + drawings.triangles.size();
    f->glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(numOfVertices * sizeof(DrawingList::Vertex)), nullptr, GL_STREAM_DRAW);
    GLintptr offset = 0;
    for(const auto* vertices : {&drawings.lines, &drawings.points, &drawings.triangles})
    {
      const GLsizeiptr size = static_cast<GLsizeiptr>(vertices->size() * sizeof(DrawingList::Vertex));
      if(size)
        f->glBufferSubData(GL_ARRAY_BUFFER, offset, size, vertices->data());
      offset += size;
    }
    f->glBindBuffer(GL_ARRAY_BUFFER, data->vbo);
  }

  if(!drawings.images.empty())
  {
    data->drawingTextures.resize(drawings.images.size());
    f->glGenTextures(static_cast<GLsizei>(data->drawingTextures.size()), data->drawingTextures.data());
    f->glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    for(std::size_t i = 0; i < drawings.images.size(); ++i)
    {
      const DrawingList::Image& image = drawings.images[i];
      f->glBindTexture(GL_TEXTURE_2D, data->drawingTextures[i]);
      f->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, static_cast<GLsizei>(image.width), static_cast<GLsizei>(image.height), 0, GL_RGB, GL_UNSIGNED_BYTE, image.rgb.data());
      f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    f->glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    f->glBindTexture(GL_TEXTURE_2D, 0);
  }
}

void GraphicsContext::drawDrawings(const DrawingList& drawings, const Matrix4f& projection, const Matrix4f& view, float alpha, bool opaquePass, bool fillPolygons)
{
  ASSERT(data);
  ASSERT(f);
  ASSERT(!shader);
  ASSERT(drawingSphereMesh && drawingQuadMesh);

  const Matrix4f pv = projection * view;
  const DrawingShader& streamShader = data->drawingShaders[0];
  const DrawingShader& flatShader = data->drawingShaders[1];
  const DrawingShader& imageShader = data->drawingShaders[2];

  const GLint firstPoint = static_cast<GLint>(drawings.lines.size());
  const GLint firstTriangle = static_cast<GLint>(drawings.lines.size() + drawings.points.size());

  f->glPolygonMode(GL_FRONT_AND_BACK, fillPolygons ? GL_FILL : GL_LINE);
  f->glEnable(GL_BLEND);
  f->glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ZERO, GL_ONE);
  f->glEnable(GL_PROGRAM_POINT_SIZE);

  // Binds a mesh of the context (only its position and texture coordinates are used).
  const auto bindMesh = [this](const Mesh* mesh)
  {
    f->glBindVertexArray(data->vao[mesh->vertexBuffer->category]);
  };
  const auto drawMesh = [this](const Mesh* mesh)
  {
    f->glDrawElementsBaseVertex(getPrimitiveType(mesh->topology), static_cast<GLsizei>(mesh->indexBuffer->count), GL_UNSIGNED_INT, reinterpret_cast<void*>(mesh->indexBuffer->offset), static_cast<GLint>(mesh->vertexBuffer->base));
  };

  for(const DrawingList::Item& item : drawings.items)
  {
    if(item.renderOptions & (SimRobotCore3::Drawing::disableTransparency | SimRobotCore3::Drawing::disableOpacity))
    {
      if(opaquePass != ((item.renderOptions & SimRobotCore3::Drawing::disableTransparency) != 0))
        continue;
    }
    const bool disableDepth = item.renderOptions & SimRobotCore3::Drawing::disableDepth;
    if(disableDepth)
      f->glDepthMask(GL_FALSE);

    const Matrix4f pvm = pv * item.transformation;

    if(item.numOfPoints || item.numOfLines || item.numOfTriangles)
    {
      f->glBindVertexArray(data->drawingVAO);
      f->glBindBuffer(GL_ARRAY_BUFFER, data->drawingVBO);
      f->glUseProgram(streamShader.program);
      f->glUniformMatrix4fv(streamShader.pvmLocation, 1, GL_FALSE, pvm.data());
      f->glUniform1f(streamShader.alphaLocation, alpha);
      if(item.numOfLines)
        f->glDrawArrays(GL_LINES, static_cast<GLint>(item.firstLine), static_cast<GLsizei>(item.numOfLines));
      if(item.numOfPoints)
        f->glDrawArrays(GL_POINTS, firstPoint + static_cast<GLint>(item.firstPoint), static_cast<GLsizei>(item.numOfPoints));
      if(item.numOfTriangles)
        f->glDrawArrays(GL_TRIANGLES, firstTriangle + static_cast<GLint>(item.firstTriangle), static_cast<GLsizei>(item.numOfTriangles));
      f->glBindBuffer(GL_ARRAY_BUFFER, data->vbo);
    }

    if(item.numOfInstances)
    {
      bindMesh(drawingSphereMesh);
      f->glUseProgram(flatShader.program);
      f->glUniform1f(flatShader.alphaLocation, alpha);
      for(std::size_t i = item.firstInstance; i < item.firstInstance + item.numOfInstances; ++i)
      {
        const DrawingList::Instance& instance = drawings.instances[i];
        const Matrix4f instancePVM = pvm * instance.transformation;
        f->glUniformMatrix4fv(flatShader.pvmLocation, 1, GL_FALSE, instancePVM.data());
        f->glUniform4f(flatShader.colorLocation, instance.color[0] / 255.f, instance.color[1] / 255.f, instance.color[2] / 255.f, instance.color[3] / 255.f);
        drawMesh(drawingSphereMesh);
      }
    }

    if(item.numOfImages)
    {
      bindMesh(drawingQuadMesh);
      f->glUseProgram(imageShader.program);
      f->glUniform1f(imageShader.alphaLocation, alpha);
      f->glUniform1i(imageShader.textureLocation, 0);
      f->glActiveTexture(GL_TEXTURE0);
      f->glDisable(GL_CULL_FACE);
      for(std::size_t i = item.firstImage; i < item.firstImage + item.numOfImages; ++i)
      {
        const Matrix4f imagePVM = pvm * drawings.images[i].transformation;
        f->glUniformMatrix4fv(imageShader.pvmLocation, 1, GL_FALSE, imagePVM.data());
        f->glBindTexture(GL_TEXTURE_2D, data->drawingTextures[i]);
        drawMesh(drawingQuadMesh);
      }
      f->glBindTexture(GL_TEXTURE_2D, 0);
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
  if(!data->drawingTextures.empty())
  {
    f->glDeleteTextures(static_cast<GLsizei>(data->drawingTextures.size()), data->drawingTextures.data());
    data->drawingTextures.clear();
  }
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
    f->glBindTexture(GL_TEXTURE_2D, (data->boundTexture = newTexture));
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
  const char* versionSourceCode = "#version 330 core\n";

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
    lightDeclarationsCode += " uniform mat4 lightPV" + std::to_string(i) + "; uniform sampler2DShadow shadowMap" + std::to_string(i) + ";";
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
  f->glUniformBlockBinding(shader.program, f->glGetUniformBlockIndex(shader.program, "Surfaces"), 0);
  shader.cameraPVLocation = f->glGetUniformLocation(shader.program, "cameraPV");
  shader.cameraPosLocation = f->glGetUniformLocation(shader.program, "cameraPos");
  shader.modelMatrixLocation = f->glGetUniformLocation(shader.program, "modelMatrix");
  shader.surfaceIndexLocation = f->glGetUniformLocation(shader.program, "surfaceIndex");
  shader.lightPVLocation.resize(shadowMaps.size());
  shader.shadowMapLocation.resize(shadowMaps.size());
  for(std::size_t i = 0; i < shadowMaps.size(); ++i)
  {
    shader.lightPVLocation[i] = f->glGetUniformLocation(shader.program, ("lightPV" + std::to_string(i)).c_str());
    shader.shadowMapLocation[i] = f->glGetUniformLocation(shader.program, ("shadowMap" + std::to_string(i)).c_str());
  }
  return shader;
}

GraphicsContext::Shader GraphicsContext::compileDepthOnlyShader()
{
  const char* versionSourceCode = "#version 330 core\n";

  Shader shader;
  shader.program = compileShader({versionSourceCode, depthOnlyVertexShaderSourceCode}, {versionSourceCode, depthOnlyFragmentShaderSourceCode});

  ASSERT(f);
  shader.cameraPVLocation = f->glGetUniformLocation(shader.program, "cameraPV");
  shader.modelMatrixLocation = f->glGetUniformLocation(shader.program, "modelMatrix");
  return shader;
}

QOpenGLFunctions_3_3_Core* GraphicsContext::getOpenGLFunctions() const
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

void GraphicsContext::setupVertexAttributesPN(QOpenGLFunctions_3_3_Core& functions)
{
  functions.glEnableVertexAttribArray(0);
  functions.glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), reinterpret_cast<void*>(0));
  functions.glEnableVertexAttribArray(1);
  functions.glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), reinterpret_cast<void*>(3 * sizeof(GLfloat)));
  functions.glEnableVertexAttribArray(2);
  functions.glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), reinterpret_cast<void*>(0));
}

void GraphicsContext::setupVertexAttributesPNT(QOpenGLFunctions_3_3_Core& functions)
{
  functions.glEnableVertexAttribArray(0);
  functions.glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), reinterpret_cast<void*>(0));
  functions.glEnableVertexAttribArray(1);
  functions.glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), reinterpret_cast<void*>(3 * sizeof(GLfloat)));
  functions.glEnableVertexAttribArray(2);
  functions.glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), reinterpret_cast<void*>(6 * sizeof(GLfloat)));
}

#endif
