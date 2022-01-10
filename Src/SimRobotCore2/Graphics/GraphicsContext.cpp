/**
 * @file Graphics/GraphicsContext.cpp
 *
 * This file implements a class that handles graphics.
 *
 * @author Arne Hasselbring
 */

#include "GraphicsContext.h"
#include "Graphics/Light.h"
#include "Platform/Assert.h"
#include <QImage>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <cstddef>

// The following shader source code is based on https://learnopengl.com/Lighting/Multiple-lights.

static const char* vertexShaderSourceCode = R"glsl(
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoords;

out vec3 FragPos;
NORMAL_QUALIFIER out vec3 Normal;
out vec2 TexCoords;

uniform mat4 cameraPV;
uniform mat4 modelMatrix;

void main()
{
  FragPos = vec3(modelMatrix * vec4(inPosition, 1.0));
  Normal = mat3(modelMatrix) * inNormal;
  TexCoords = inTexCoords;
  gl_Position = cameraPV * vec4(FragPos, 1.0);
}
)glsl";

static const char* depthOnlyVertexShaderSourceCode = R"glsl(
layout(location = 0) in vec3 inPosition;

uniform mat4 cameraPV;
uniform mat4 modelMatrix;

void main()
{
  gl_Position = cameraPV * modelMatrix * vec4(inPosition, 1.0);
}
)glsl";

static const char* fragmentShaderSourceCode = R"glsl(
struct DirLight
{
  vec4 diffuseColor;
  vec4 ambientColor;
  vec4 specularColor;
  vec3 direction;
};

struct PointLight
{
  vec4 diffuseColor;
  vec4 ambientColor;
  vec4 specularColor;
  vec3 position;
  float constantAttenuation;
  float linearAttenuation;
  float quadraticAttenuation;
};

struct SpotLight
{
  vec4 diffuseColor;
  vec4 ambientColor;
  vec4 specularColor;
  vec3 position;
  float constantAttenuation;
  float linearAttenuation;
  float quadraticAttenuation;
  vec3 direction;
  float cutoff;
};

struct Surface
{
  vec4 diffuseColor;
  vec4 ambientColor;
  vec4 specularColor;
  vec4 emissionColor;
  float shininess;
  bool hasTexture;
};

in vec3 FragPos;
NORMAL_QUALIFIER in vec3 Normal;
in vec2 TexCoords;

uniform vec3 cameraPos;
uniform uint surfaceIndex;
uniform sampler2D diffuseTexture;
layout (std140) uniform Surfaces
{
  Surface surfaces[NUM_OF_SURFACES];
};

out vec4 FragColor;

void calcDirLight(in DirLight light, in vec3 normal, in vec3 viewDir, inout vec4 diffuse, inout vec4 ambient, inout vec4 specular)
{
  vec3 lightDir = normalize(-light.direction);
  float diff = max(dot(normal, lightDir), 0.0);
  vec3 reflectDir = reflect(-lightDir, normal);
  float spec = pow(max(dot(viewDir, reflectDir), 0.0), surfaces[surfaceIndex].shininess);
  diffuse += light.diffuseColor * diff;
  ambient += light.ambientColor;
  specular += light.specularColor * spec;
}

void calcPointLight(in PointLight light, in vec3 pos, in vec3 normal, in vec3 viewDir, inout vec4 diffuse, inout vec4 ambient, inout vec4 specular)
{
  vec3 lightDir = normalize(light.position - pos);
  float diff = max(dot(normal, lightDir), 0.0);
  vec3 reflectDir = reflect(-lightDir, normal);
  float spec = pow(max(dot(viewDir, reflectDir), 0.0), surfaces[surfaceIndex].shininess);
  float distance = length(light.position - pos);
  float attenuation = 1.0 / (light.constantAttenuation + light.linearAttenuation * distance + light.quadraticAttenuation * distance * distance);
  diffuse += light.diffuseColor * diff * attenuation;
  ambient += light.ambientColor * attenuation;
  specular += light.specularColor * spec * attenuation;
}

void calcSpotLight(in SpotLight light, in vec3 pos, in vec3 normal, in vec3 viewDir, inout vec4 diffuse, inout vec4 ambient, inout vec4 specular)
{
  vec3 lightDir = normalize(light.position - pos);
  float diff = max(dot(normal, lightDir), 0.0);
  vec3 reflectDir = reflect(-lightDir, normal);
  float spec = pow(max(dot(viewDir, reflectDir), 0.0), surfaces[surfaceIndex].shininess);
  float distance = length(light.position - pos);
  float attenuation = 1.0 / (light.constantAttenuation + light.linearAttenuation * distance + light.quadraticAttenuation * distance * distance);
  float theta = dot(lightDir, normalize(-light.direction));
  float intensity = clamp((theta - light.cutoff) / (1 - light.cutoff), 0.0, 1.0);
  diffuse += light.diffuseColor * diff * attenuation * intensity;
  ambient += light.ambientColor * attenuation * intensity;
  specular += light.specularColor * spec * attenuation * intensity;
}

#ifdef WITH_LIGHTING
DECLARE_LIGHTS
#endif

void main()
{
  vec4 color;
#ifdef WITH_LIGHTING
  vec3 viewDir = normalize(cameraPos - FragPos);
  vec4 diffuse = vec4(0.0);
  vec4 ambient = GLOBAL_AMBIENT_LIGHT;
  vec4 specular = vec4(0.0);
  CALCULATE_LIGHTS
  color = surfaces[surfaceIndex].emissionColor + ambient * surfaces[surfaceIndex].ambientColor + diffuse * surfaces[surfaceIndex].diffuseColor + specular * surfaces[surfaceIndex].specularColor;
  color = clamp(color, 0.0, 1.0);
#else
  color = surfaces[surfaceIndex].diffuseColor;
#endif
#ifdef WITH_TEXTURES
  if (surfaces[surfaceIndex].hasTexture)
  {
    color = color * texture(diffuseTexture, TexCoords);
  }
#endif
  if (color.a < 0.01)
  {
    discard;
  }
  FragColor = color;
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
  vertexBuffers.resize(2);
  vertexBuffers[VertexPN::index].setupVertexAttributes = VertexPN::setupVertexAttributes;
  vertexBuffers[VertexPN::index].stride = VertexPN::size;
  vertexBuffers[VertexPNT::index].setupVertexAttributes = VertexPNT::setupVertexAttributes;
  vertexBuffers[VertexPNT::index].stride = VertexPNT::size;
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

  for(const auto& texture : textures)
    delete texture.second;
  for(const auto* modelMatrix : modelMatrices)
    delete modelMatrix;
  for(const auto* surface : surfaces)
    delete surface;
  for(const auto& pair : vertexBuffers)
    for(const auto* vertexBuffer : pair.buffers)
      delete vertexBuffer;
  for(const auto* indexBuffer : indexBuffers)
    delete indexBuffer;
  for(const auto* mesh : meshes)
    delete mesh;
}

void GraphicsContext::compile()
{
  // Determine buffer memory layout of vertex buffer.
  GLint base = 0;
  GLintptr offset = 0;
  for(const auto& category : vertexBuffers)
  {
    // Align on multiples of the current stride to adjust the base index.
    offset += category.stride - 1;
    base = static_cast<GLint>(offset / category.stride);
    offset = base * category.stride;
    for(auto* buffer : category.buffers)
    {
      buffer->base = base;
      buffer->offset = offset;
      base += buffer->count;
      offset += buffer->size();
    }
  }
  vertexBufferTotalSize = offset;

  // Determine buffer memory layout of element buffer.
  offset = 0;
  for(auto* buffer : indexBuffers)
  {
    buffer->offset = offset;
    buffer->count = static_cast<std::uint32_t>(buffer->indices.size());
    buffer->type = GL_UNSIGNED_INT; // TODO: later, it might be nice to use shorts/bytes if the indices fit within that range
    offset += buffer->size();
  }
  indexBufferTotalSize = offset;

  // Determine texture indices.
  std::size_t index = 0;
  for(auto& texture : textures)
    texture.second->index = index++;

  // Determine surface indices.
  index = 0;
  for(auto* surface : surfaces)
    surface->index = index++;
}

void GraphicsContext::createGraphics()
{
  if(!initializeOpenGLFunctions())
    return;

  const auto* context = QOpenGLContext::currentContext();

  // Check if the context is already initialized.
  if(perContextData.find(context) != perContextData.end())
    return;

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

  // Enable depth test.
  glClearDepth(1.0f);
  glDepthFunc(GL_LEQUAL);
  glEnable(GL_DEPTH_TEST);

  // Avoid rendering the backside of surfaces.
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glFrontFace(GL_CCW);

  // Set clear color.
  glClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);

  // Buffer objects are shared between contexts.
  if(shareData)
  {
    data.vbo = shareData->vbo;
    data.ebo = shareData->ebo;
    data.ubo = shareData->ubo;
  }
  else
  {
    glGenBuffers(1, &data.vbo);
    glGenBuffers(1, &data.ebo);
    glGenBuffers(1, &data.ubo);
    // Data can't be uploaded here because to bind the EBO, a VAO must be bound.
  }

  // All vertex attributes use the same VBO.
  glBindBuffer(GL_ARRAY_BUFFER, data.vbo);

  // VAOs are never shared between contexts, so they must be created here.
  data.vao.resize(vertexBuffers.size());
  glGenVertexArrays(static_cast<GLsizei>(data.vao.size()), data.vao.data());
  for(std::size_t vaoIndex = 0; vaoIndex < data.vao.size(); ++vaoIndex)
  {
    glBindVertexArray(data.vao[vaoIndex]);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.ebo);
    vertexBuffers[vaoIndex].setupVertexAttributes(*this);
  }

  // Upload buffer data, now that also the EBO is bound.
  if(!shareData)
  {
    glBufferData(GL_ARRAY_BUFFER, vertexBufferTotalSize, nullptr, GL_STATIC_DRAW);
    for(const auto& pair : vertexBuffers)
      for(const auto* buffer : pair.buffers)
        glBufferSubData(GL_ARRAY_BUFFER, buffer->offset, buffer->size(), buffer->data);

    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBufferTotalSize, nullptr, GL_STATIC_DRAW);
    for(const auto* buffer : indexBuffers)
      glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, buffer->offset, buffer->size(), buffer->indices.data());

    glBindBuffer(GL_UNIFORM_BUFFER, data.ubo);
    glBufferData(GL_UNIFORM_BUFFER, surfaces.size() * Surface::memorySize, nullptr, GL_STATIC_DRAW);
    for(std::size_t i = 0; i < surfaces.size(); ++i)
    {
      static constexpr std::size_t verbatimPart = offsetof(Surface, shininess) - offsetof(Surface, diffuseColor) + sizeof(Surface::shininess);
      unsigned char buf[Surface::memorySize];
      std::memcpy(buf, &surfaces[i]->diffuseColor, verbatimPart);
      *reinterpret_cast<unsigned int*>(buf + verbatimPart) = surfaces[i]->texture != nullptr;
      glBufferSubData(GL_UNIFORM_BUFFER, i * Surface::memorySize, Surface::memorySize, buf);
    }
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
  }

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);

  // Shaders and textures are shared as well.
  if(shareData)
  {
    data.textureIDs = shareData->textureIDs;
    data.shaders = shareData->shaders;
  }
  else
  {
    // Upload textures.
    data.textureIDs.resize(textures.size());
    glGenTextures(static_cast<GLsizei>(textures.size()), data.textureIDs.data());
    for(const auto& pair : textures)
    {
      const Texture* texture = pair.second;
      glBindTexture(GL_TEXTURE_2D, data.textureIDs[texture->index]);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexImage2D(GL_TEXTURE_2D, 0, texture->hasAlpha ? GL_RGBA : GL_RGB, texture->width, texture->height, 0, texture->byteOrder, GL_UNSIGNED_BYTE, texture->data);
      glGenerateMipmap(GL_TEXTURE_2D);
    }

    // Compile shaders.
    for(unsigned int i = 0; i < 8; ++i)
      data.shaders[i] = compileColorShader(i & 4, i & 2, i & 1);
    data.shaders[8] = compileDepthOnlyShader();
  }
}

void GraphicsContext::destroyGraphics()
{
  const auto* context = QOpenGLContext::currentContext();

  if(perContextData.find(context) == perContextData.end())
    return;

  auto& data = perContextData[context];
  glDeleteVertexArrays(static_cast<GLsizei>(data.vao.size()), data.vao.data());
  if(--referenceCounters[data.referenceCounterIndex] == 0)
  {
    glDeleteBuffers(1, &data.vbo);
    glDeleteBuffers(1, &data.ebo);
    glDeleteBuffers(1, &data.ubo);
    glDeleteTextures(static_cast<GLsizei>(data.textureIDs.size()), data.textureIDs.data());
    for(const auto& shader : data.shaders)
      glDeleteProgram(shader.program);
  }

  perContextData.erase(context);
}

void GraphicsContext::setClearColor(const float* color)
{
  std::memcpy(clearColor, color, sizeof(clearColor));
}

GraphicsContext::IndexBuffer* GraphicsContext::requestIndexBuffer()
{
  return indexBuffers.emplace_back(new IndexBuffer);
}

GraphicsContext::Mesh* GraphicsContext::requestMesh(const VertexBufferBase* vertexBuffer, const IndexBuffer* indexBuffer, PrimitiveTopology primitiveTopology)
{
  Mesh* mesh = new Mesh;
  mesh->vertexBuffer = vertexBuffer;
  mesh->indexBuffer = indexBuffer;
  switch(primitiveTopology)
  {
    case pointList:
      mesh->mode = GL_POINTS;
      break;
    case lineList:
      mesh->mode = GL_LINES;
      break;
    case triangleList:
      mesh->mode = GL_TRIANGLES;
      break;
  }
  meshes.push_back(mesh);
  return mesh;
}

GraphicsContext::Texture* GraphicsContext::requestTexture(const std::string& file)
{
  auto iter = textures.find(file);
  if(iter != textures.end())
  {
    Texture* texture = iter->second;
    return texture->data ? texture : nullptr;
  }
  Texture*& texture = textures[file];
  texture = new Texture(file);
  return texture->data ? texture : nullptr;
}

GraphicsContext::Surface* GraphicsContext::requestSurface(const float* diffuseColor, const float* ambientColor, const float* specularColor, const float* emissionColor, float shininess, const Texture* texture)
{
  auto* surface = new Surface;
  static const float defaultColor[4] = {0.f, 0.f, 0.f, 1.f};
  std::memcpy(surface->diffuseColor, diffuseColor, sizeof(surface->diffuseColor));
  std::memcpy(surface->ambientColor, ambientColor, sizeof(surface->ambientColor));
  std::memcpy(surface->specularColor, specularColor ? specularColor : defaultColor, sizeof(surface->specularColor));
  std::memcpy(surface->emissionColor, emissionColor ? emissionColor : defaultColor, sizeof(surface->emissionColor));
  surface->shininess = shininess;
  surface->texture = texture;
  surfaces.push_back(surface);
  return surface;
}

void GraphicsContext::setGlobalAmbientLight(const float* color)
{
  globalAmbientLight = "vec4(" + std::to_string(color[0]) + ", " + std::to_string(color[1]) + ", " + std::to_string(color[2]) + ", " + std::to_string(color[3]) + ")";
}

void GraphicsContext::addLight(const Light* light)
{
  ASSERT(lightDeclarations.size() == lightCalculations.size());
  if(const DirLight* dirLight = dynamic_cast<const DirLight*>(light); dirLight)
  {
    lightDeclarations.push_back("const DirLight light" + std::to_string(lightDeclarations.size()) + " = DirLight(vec4(" + std::to_string(dirLight->diffuseColor[0]) + ", " + std::to_string(dirLight->diffuseColor[1]) + ", " + std::to_string(dirLight->diffuseColor[2]) + ", " + std::to_string(dirLight->diffuseColor[3]) + "), vec4(" + std::to_string(dirLight->ambientColor[0]) + ", " + std::to_string(dirLight->ambientColor[1]) + ", " + std::to_string(dirLight->ambientColor[2]) + ", " + std::to_string(dirLight->ambientColor[3]) + "), vec4(" + std::to_string(dirLight->specularColor[0]) + ", " + std::to_string(dirLight->specularColor[1]) + ", " + std::to_string(dirLight->specularColor[2]) + ", " + std::to_string(dirLight->specularColor[3]) + "), vec3(" + std::to_string(dirLight->direction[0]) + ", " + std::to_string(dirLight->direction[1]) + ", " + std::to_string(dirLight->direction[2]) + "));");
    lightCalculations.push_back("calcDirLight(light" + std::to_string(lightCalculations.size()) + ", Normal, viewDir, diffuse, ambient, specular);");
  }
  else if(const SpotLight* spotLight = dynamic_cast<const SpotLight*>(light); spotLight)
  {
    lightDeclarations.push_back("const SpotLight light" + std::to_string(lightDeclarations.size()) + " = SpotLight(vec4(" + std::to_string(spotLight->diffuseColor[0]) + ", " + std::to_string(spotLight->diffuseColor[1]) + ", " + std::to_string(spotLight->diffuseColor[2]) + ", " + std::to_string(spotLight->diffuseColor[3]) + "), vec4(" + std::to_string(spotLight->ambientColor[0]) + ", " + std::to_string(spotLight->ambientColor[1]) + ", " + std::to_string(spotLight->ambientColor[2]) + ", " + std::to_string(spotLight->ambientColor[3]) + "), vec4(" + std::to_string(spotLight->specularColor[0]) + ", " + std::to_string(spotLight->specularColor[1]) + ", " + std::to_string(spotLight->specularColor[2]) + ", " + std::to_string(spotLight->specularColor[3]) + "), vec3(" + std::to_string(spotLight->position[0]) + ", " + std::to_string(spotLight->position[1]) + ", " + std::to_string(spotLight->position[2]) + "), " + std::to_string(spotLight->constantAttenuation) + ", " + std::to_string(spotLight->linearAttenuation) + ", " + std::to_string(spotLight->quadraticAttenuation) + ", vec3(" + std::to_string(spotLight->direction[0]) + ", " + std::to_string(spotLight->direction[1]) + ", " + std::to_string(spotLight->direction[2]) + "), " + std::to_string(spotLight->cutoff) + ");");
    lightCalculations.push_back("calcSpotLight(light" + std::to_string(lightCalculations.size()) + ", FragPos, Normal, viewDir, diffuse, ambient, specular);");
  }
  else if(const PointLight* pointLight = dynamic_cast<const PointLight*>(light); pointLight)
  {
    lightDeclarations.push_back("const PointLight light" + std::to_string(lightDeclarations.size()) + " = PointLight(vec4(" + std::to_string(pointLight->diffuseColor[0]) + ", " + std::to_string(pointLight->diffuseColor[1]) + ", " + std::to_string(pointLight->diffuseColor[2]) + ", " + std::to_string(pointLight->diffuseColor[3]) + "), vec4(" + std::to_string(pointLight->ambientColor[0]) + ", " + std::to_string(pointLight->ambientColor[1]) + ", " + std::to_string(pointLight->ambientColor[2]) + ", " + std::to_string(pointLight->ambientColor[3]) + "), vec4(" + std::to_string(pointLight->specularColor[0]) + ", " + std::to_string(pointLight->specularColor[1]) + ", " + std::to_string(pointLight->specularColor[2]) + ", " + std::to_string(pointLight->specularColor[3]) + "), vec3(" + std::to_string(pointLight->position[0]) + ", " + std::to_string(pointLight->position[1]) + ", " + std::to_string(pointLight->position[2]) + "), " + std::to_string(pointLight->constantAttenuation) + ", " + std::to_string(pointLight->linearAttenuation) + ", " + std::to_string(pointLight->quadraticAttenuation) + ");");
    lightCalculations.push_back("calcPointLight(light" + std::to_string(lightCalculations.size()) + ", FragPos, Normal, viewDir, diffuse, ambient, specular);");
  }
}

GraphicsContext::ModelMatrix* GraphicsContext::requestModelMatrix()
{
  ModelMatrix* modelMatrix = new ModelMatrix;
  const ModelMatrixStack& modelMatrixStack = modelMatrixStackStack.top();
  const auto& product = modelMatrixStack.getC();
  std::size_t startIndex = 0;
  if(modelMatrixStack.bottomIsVariable)
  {
    ASSERT(!product.empty());
    modelMatrix->variablePart = product[0];
    startIndex = 1;
  }
  modelMatrix->constantPart = product.size() > startIndex ? Matrix4f(Eigen::Map<const Matrix4f>(product[startIndex])) : Matrix4f(Matrix4f::Identity());
  for(std::size_t i = startIndex + 1; i < product.size(); ++i)
    modelMatrix->constantPart *= Eigen::Map<const Matrix4f>(product[i]);
  Eigen::Map<Matrix4f>(modelMatrix->memory) = modelMatrix->constantPart;
  modelMatrices.push_back(modelMatrix);
  return modelMatrix;
}

void GraphicsContext::pushModelMatrixStack()
{
  modelMatrixStackStack.emplace();
}

void GraphicsContext::popModelMatrixStack()
{
  ASSERT(modelMatrixStackStack.top().empty());
  modelMatrixStackStack.pop();
}

void GraphicsContext::pushModelMatrix(const Matrix4f& transformation)
{
  modelMatrixStackStack.top().push(transformation.data());
}

void GraphicsContext::pushModelMatrixByReference(const Matrix4f& transformation)
{
  // A variable transformation must be the first on the stack.
  ASSERT(modelMatrixStackStack.top().empty());
  modelMatrixStackStack.top().push(transformation.data());
  modelMatrixStackStack.top().bottomIsVariable = true;
}

void GraphicsContext::popModelMatrix()
{
  modelMatrixStackStack.top().pop();
  if(modelMatrixStackStack.top().empty())
    modelMatrixStackStack.top().bottomIsVariable = false;
}

bool GraphicsContext::emptyModelMatrixStack() const
{
  return modelMatrixStackStack.top().empty();
}

void GraphicsContext::updateModelMatrices(bool forceUpdate)
{
  if(lastModelMatrixTimestamp == Simulation::simulation->simulationStep && !forceUpdate)
    return;
  lastModelMatrixTimestamp = Simulation::simulation->simulationStep;

  for(ModelMatrix* modelMatrix : modelMatrices)
    if(modelMatrix->variablePart)
      Eigen::Map<Matrix4f>(modelMatrix->memory) = Eigen::Map<const Matrix4f>(modelMatrix->variablePart) * modelMatrix->constantPart;
}

void GraphicsContext::startColorRendering(const Matrix4f& projection, const Matrix4f& view, int viewportX, int viewportY, int viewportWidth, int viewportHeight, bool clear, bool lighting, bool textures, bool smoothShading, bool fillPolygons)
{
  const auto* context = QOpenGLContext::currentContext();
  ASSERT(!data);
  ASSERT(!shader);
  data = &perContextData[context];
  shader = &data->shaders[(lighting ? 4 : 0) + (textures ? 2 : 0) + (smoothShading ? 1 : 0)];
  if(clear)
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  if(viewportX >= 0)
    glViewport(viewportX, viewportY, viewportWidth, viewportHeight);
  glPolygonMode(GL_FRONT_AND_BACK, fillPolygons ? GL_FILL : GL_LINE);
  glUseProgram(shader->program);
  const Matrix4f pv = projection * view;
  glUniformMatrix4fv(shader->cameraPVLocation, 1, GL_FALSE, pv.data());
  if(shader->cameraPosLocation >= 0)
  {
    const Vector3f pos = -view.topLeftCorner<3, 3>().transpose() * view.topRightCorner<3, 1>();
    glUniform3fv(shader->cameraPosLocation, 1, pos.data());
  }
  glBindBufferBase(GL_UNIFORM_BUFFER, 0, data->ubo);
}

void GraphicsContext::startDepthOnlyRendering(const Matrix4f& projection, const Matrix4f& view, int viewportX, int viewportY, int viewportWidth, int viewportHeight, bool clear)
{
  const auto* context = QOpenGLContext::currentContext();
  ASSERT(!data);
  ASSERT(!shader);
  data = &perContextData[context];
  shader = &data->shaders[8];
  if(clear)
    glClear(GL_DEPTH_BUFFER_BIT);
  if(viewportX >= 0)
    glViewport(viewportX, viewportY, viewportWidth, viewportHeight);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glUseProgram(shader->program);
  const Matrix4f pv = projection * view;
  glUniformMatrix4fv(shader->cameraPVLocation, 1, GL_FALSE, pv.data());
  glBindBufferBase(GL_UNIFORM_BUFFER, 0, data->ubo);
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
  ASSERT(shader);
  glBindVertexArray(data->vao[mesh->vertexBuffer->vaoIndex]);
  glUniformMatrix4fv(shader->modelMatrixLocation, 1, GL_FALSE, modelMatrix->memory);
  if(!forcedSurface)
    setSurface(surface);
  if(mesh->indexBuffer)
    glDrawElementsBaseVertex(mesh->mode, mesh->indexBuffer->count, mesh->indexBuffer->type, reinterpret_cast<void*>(mesh->indexBuffer->offset), mesh->vertexBuffer->base);
  else
    glDrawArrays(mesh->mode, mesh->vertexBuffer->base, mesh->vertexBuffer->count);
}

void GraphicsContext::finishRendering()
{
  ASSERT(data);
  ASSERT(shader);
  ASSERT(!forcedSurface);
  data = nullptr;
  shader = nullptr;
}

void GraphicsContext::initOffscreenRenderer()
{
  ASSERT(!offscreenSurface && !offscreenContext);

  offscreenSurface = new QOffscreenSurface;
  offscreenSurface->create();

  offscreenContext = new QOpenGLContext;
  offscreenContext->setShareContext(QOpenGLContext::globalShareContext());
  VERIFY(offscreenContext->create());
  offscreenContext->makeCurrent(offscreenSurface);

  createGraphics();
}

bool GraphicsContext::makeCurrent(int width, int height, bool sampleBuffers)
{
  ASSERT(offscreenContext && offscreenSurface);
  offscreenContext->makeCurrent(offscreenSurface);

  // Considering weak graphics cards glClear is faster when the color and depth buffers are not greater then they have to be.
  // So we create an individual buffer for each size in demand.

  auto it = offscreenBuffers.find(width << 16 | height << 1 | (sampleBuffers ? 1 : 0));
  if(it == offscreenBuffers.end())
  {
    QOpenGLFramebufferObject*& buffer = offscreenBuffers[width << 16 | height << 1 | (sampleBuffers ? 1 : 0)];

    buffer = new QOpenGLFramebufferObject(width, height, QOpenGLFramebufferObject::Depth);
    if(!buffer->isValid())
    {
      delete buffer;
      buffer = nullptr;
      return false;
    }

    return true;
  }
  else
    return it->second && it->second->bind();
}

void GraphicsContext::finishImageRendering(void* image, int w, int h)
{
  const int lineSize = w * 3;
  glPixelStorei(GL_PACK_ALIGNMENT, lineSize & (8 - 1) ? (lineSize & (4 - 1) ? 1 : 4) : 8);
  glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, image);
}

void GraphicsContext::finishDepthRendering(void* image, int w, int h)
{
  glPixelStorei(GL_PACK_ALIGNMENT, w * 4 & (8 - 1) ? 4 : 8);
  glReadPixels(0, 0, w, h, GL_DEPTH_COMPONENT, GL_FLOAT, image);
}

void GraphicsContext::setSurface(const Surface* surface)
{
  ASSERT(data);
  ASSERT(shader);
  if(surface->texture)
    glBindTexture(GL_TEXTURE_2D, data->textureIDs[surface->texture->index]);
  else
    glBindTexture(GL_TEXTURE_2D, 0);
  if(shader->surfaceIndexLocation >= 0)
    glUniform1ui(shader->surfaceIndexLocation, static_cast<GLuint>(surface->index));
  if(surface->texture ? surface->texture->hasAlpha : (surface->diffuseColor[3] < 1.f))
  {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }
  else
    glDisable(GL_BLEND);
}

GLuint GraphicsContext::compileShader(const std::vector<const char*>& vertexShaderSources, const std::vector<const char*>& fragmentShaderSources)
{
#ifndef NDEBUG
  GLint success = 0;
#endif

  const GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
  ASSERT(vertexShader > 0);
  glShaderSource(vertexShader, static_cast<GLsizei>(vertexShaderSources.size()), vertexShaderSources.data(), nullptr);
  glCompileShader(vertexShader);
#ifndef NDEBUG
  glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
  if(!success)
  {
    char log[512];
    glGetShaderInfoLog(vertexShader, sizeof(log), nullptr, log);
    TRACE("%s", log);
    ASSERT(false);
  }
#endif

  const GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
  ASSERT(fragmentShader > 0);
  glShaderSource(fragmentShader, static_cast<GLsizei>(fragmentShaderSources.size()), fragmentShaderSources.data(), nullptr);
  glCompileShader(fragmentShader);
#ifndef NDEBUG
  glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
  if(!success)
  {
    char log[512];
    glGetShaderInfoLog(fragmentShader, sizeof(log), nullptr, log);
    TRACE("%s", log);
    ASSERT(false);
  }
#endif

  const GLuint program = glCreateProgram();
  ASSERT(program > 0);
  glAttachShader(program, vertexShader);
  glAttachShader(program, fragmentShader);
  glLinkProgram(program);
#ifndef NDEBUG
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if(!success)
  {
    char log[512];
    glGetProgramInfoLog(program, sizeof(log), nullptr, log);
    TRACE("%s", log);
    ASSERT(false);
  }
#endif

  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

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

  const std::string globalAmbientLightCode = "#define GLOBAL_AMBIENT_LIGHT " + globalAmbientLight + "\n";
  std::string lightDeclarationsCode = "#define DECLARE_LIGHTS";
  for(const auto& light : lightDeclarations)
    lightDeclarationsCode += " " + light;
  lightDeclarationsCode += "\n";
  std::string lightCalculationsCode = "#define CALCULATE_LIGHTS";
  for(const auto& light : lightCalculations)
    lightCalculationsCode += " " + light;
  lightCalculationsCode += "\n";

  Shader shader;
  shader.program = compileShader({versionSourceCode, defines.c_str(), vertexShaderSourceCode}, {versionSourceCode, defines.c_str(), globalAmbientLightCode.c_str(), lightDeclarationsCode.c_str(), lightCalculationsCode.c_str(), fragmentShaderSourceCode});

  glUniformBlockBinding(shader.program, glGetUniformBlockIndex(shader.program, "Surfaces"), 0);
  shader.cameraPVLocation = glGetUniformLocation(shader.program, "cameraPV");
  shader.cameraPosLocation = glGetUniformLocation(shader.program, "cameraPos");
  shader.modelMatrixLocation = glGetUniformLocation(shader.program, "modelMatrix");
  shader.surfaceIndexLocation = glGetUniformLocation(shader.program, "surfaceIndex");
  return shader;
}

GraphicsContext::Shader GraphicsContext::compileDepthOnlyShader()
{
  const char* versionSourceCode = "#version 330 core\n";

  Shader shader;
  shader.program = compileShader({versionSourceCode, depthOnlyVertexShaderSourceCode}, {versionSourceCode, depthOnlyFragmentShaderSourceCode});

  shader.cameraPVLocation = glGetUniformLocation(shader.program, "cameraPV");
  shader.modelMatrixLocation = glGetUniformLocation(shader.program, "modelMatrix");
  return shader;
}

void GraphicsContext::VertexPN::setupVertexAttributes(QOpenGLFunctions_3_3_Core& functions)
{
  functions.glEnableVertexAttribArray(0);
  functions.glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), reinterpret_cast<void*>(0));
  functions.glEnableVertexAttribArray(1);
  functions.glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), reinterpret_cast<void*>(3 * sizeof(GLfloat)));
  functions.glEnableVertexAttribArray(2);
  functions.glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), reinterpret_cast<void*>(0));
}

void GraphicsContext::VertexPNT::setupVertexAttributes(QOpenGLFunctions_3_3_Core& functions)
{
  functions.glEnableVertexAttribArray(0);
  functions.glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), reinterpret_cast<void*>(0));
  functions.glEnableVertexAttribArray(1);
  functions.glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), reinterpret_cast<void*>(3 * sizeof(GLfloat)));
  functions.glEnableVertexAttribArray(2);
  functions.glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), reinterpret_cast<void*>(6 * sizeof(GLfloat)));
}

GraphicsContext::Texture::Texture(const std::string& file)
{
  QImage image;
  if(!image.load(file.c_str()))
    return;
  if(image.format() != QImage::Format_ARGB32 &&
     image.format() != QImage::Format_RGB32 &&
     image.format() != QImage::Format_RGB888)
    return;
  width = image.width();
  height = image.height();
  byteOrder = image.format() == QImage::Format_RGB888 ? GL_BGR : GL_BGRA;
  hasAlpha = image.hasAlphaChannel();
  data = new GLubyte[image.sizeInBytes()];
  if(!data)
    return;

  GLubyte* p = reinterpret_cast<GLubyte*>(data);
  for(int y = height; y-- > 0;)
  {
    std::memcpy(p, image.scanLine(y), image.bytesPerLine());
    p += image.bytesPerLine();
  }
}
