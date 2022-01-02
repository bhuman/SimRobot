/**
 * @file Graphics/GraphicsContext.cpp
 *
 * This file implements a class that handles graphics.
 *
 * @author Arne Hasselbring
 */

#include "GraphicsContext.h"
#include "Graphics/Light.h"
#include <QImage>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>

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
uniform Surface surface;
uniform sampler2D diffuseTexture;

out vec4 FragColor;

void calcDirLight(in DirLight light, in vec3 normal, in vec3 viewDir, inout vec4 diffuse, inout vec4 ambient, inout vec4 specular)
{
  vec3 lightDir = normalize(-light.direction);
  float diff = max(dot(normal, lightDir), 0.0);
  vec3 reflectDir = reflect(-lightDir, normal);
  float spec = pow(max(dot(viewDir, reflectDir), 0.0), surface.shininess);
  diffuse += light.diffuseColor * diff;
  ambient += light.ambientColor;
  specular += light.specularColor * spec;
}

void calcPointLight(in PointLight light, in vec3 pos, in vec3 normal, in vec3 viewDir, inout vec4 diffuse, inout vec4 ambient, inout vec4 specular)
{
  vec3 lightDir = normalize(light.position - pos);
  float diff = max(dot(normal, lightDir), 0.0);
  vec3 reflectDir = reflect(-lightDir, normal);
  float spec = pow(max(dot(viewDir, reflectDir), 0.0), surface.shininess);
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
  float spec = pow(max(dot(viewDir, reflectDir), 0.0), surface.shininess);
  float distance = length(light.position - pos);
  float attenuation = 1.0 / (light.constantAttenuation + light.linearAttenuation * distance + light.quadraticAttenuation * distance * distance);
  float theta = dot(lightDir, normalize(-light.direction));
  float intensity = clamp((theta - light.cutoff) / (1 - light.cutoff), 0.0, 1.0);
  diffuse += light.diffuseColor * diff * attenuation * intensity;
  ambient += light.ambientColor * attenuation * intensity;
  specular += light.specularColor * spec * attenuation * intensity;
}

void main()
{
  vec4 color;
#ifdef WITH_LIGHTING
  vec3 viewDir = normalize(cameraPos - FragPos);
  vec4 diffuse = vec4(0.0);
  vec4 ambient = GLOBAL_AMBIENT_LIGHT;
  vec4 specular = vec4(0.0);
  LIGHTING
  color = surface.emissionColor + ambient * surface.ambientColor + diffuse * surface.diffuseColor + specular * surface.specularColor;
  color = clamp(color, 0.0, 1.0);
#else
  color = surface.diffuseColor;
#endif
#ifdef WITH_TEXTURES
  if (surface.hasTexture)
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

GraphicsContext::~GraphicsContext()
{
  ASSERT(!data);
  ASSERT(!shader);
  ASSERT(!forcedSurface);

  if(offscreenContext && offscreenSurface)
  {
    ASSERT(perContextData.size() == 1);
    // ASSERT(perContextData.begin() == offscreenContext); // TODO
    offscreenContext->makeCurrent(offscreenSurface);
    destroyGraphics();
  }
  ASSERT(perContextData.empty());
  offscreenBuffers.clear();
  delete offscreenContext;
  delete offscreenSurface;

  for(const auto& texture : textures)
    delete texture.second;
  for(const auto* modelMatrix : modelMatrices)
    delete modelMatrix;
  for(const auto* surface : surfaces)
    delete surface;
  for(const auto* vertexBuffer : vertexBuffers)
    delete vertexBuffer;
  for(const auto* indexBuffer : indexBuffers)
    delete indexBuffer;
  for(const auto* mesh : meshes)
    delete mesh;
}

bool GraphicsContext::compile()
{
  // Determine buffer memory layout of vertex buffer.
  GLint base = 0;
  GLintptr offset = 0;
  for(std::uint32_t vaoIndex = 0; vaoIndex < 2; ++vaoIndex) // TODO: for each VAO type
  {
    const std::size_t stride = (vaoIndex == 1 ? 8 : 6) * sizeof(float); // TODO: VAOType::stride
    // Align on multiples of the current stride to adjust the base index.
    offset += stride - 1;
    base = static_cast<GLint>(offset / stride);
    offset = base * stride;
    for(auto* buffer : vertexBuffers)
    {
      if(buffer->vaoIndex != vaoIndex)
        continue;
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

  return true;
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

  // VBOs and EBOs are shared between contexts.
  if(shareData)
  {
    data.vbo = shareData->vbo;
    data.ebo = shareData->ebo;
  }
  else
  {
    glGenBuffers(1, &data.vbo);
    glGenBuffers(1, &data.ebo);
    // Data can't be uploaded here because to bind the buffers, a VAO must be bound.
  }

  // VAOs are never shared between contexts, so they must be created here.
  glGenVertexArrays(2, data.vao);
  for(unsigned int pixelType = 0; pixelType < 2; ++pixelType)
  {
    glBindVertexArray(data.vao[pixelType]);
    glBindBuffer(GL_ARRAY_BUFFER, data.vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, data.ebo);

    const bool hasTextureCoordinates = pixelType == 1;
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, (hasTextureCoordinates ? 8 : 6) * sizeof(GLfloat), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, (hasTextureCoordinates ? 8 : 6) * sizeof(GLfloat), reinterpret_cast<void*>(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, (hasTextureCoordinates ? 8 : 6) * sizeof(GLfloat), reinterpret_cast<void*>(hasTextureCoordinates ? 6 * sizeof(GLfloat) : 0));
  }

  // Upload buffer data, now that a VAO is bound (doesn't matter which one).
  if(!shareData)
  {
    glBufferData(GL_ARRAY_BUFFER, vertexBufferTotalSize, nullptr, GL_STATIC_DRAW);
    for(const auto* buffer : vertexBuffers)
      glBufferSubData(GL_ARRAY_BUFFER, buffer->offset, buffer->size(), buffer->data);

    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBufferTotalSize, nullptr, GL_STATIC_DRAW);
    for(const auto* buffer : indexBuffers)
      glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, buffer->offset, buffer->size(), buffer->indices.data());
  }

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

    // Create and compile shaders.
    auto doShader = [this](bool lighting, bool textures, bool smooth)
    {
      GLint success = 0;
      const char* versionSourceCode = "#version 330 core\n";

      std::string defines;
      if(lighting)
        defines += "#define WITH_LIGHTING\n";
      if(textures)
        defines += "#define WITH_TEXTURES\n";
      if(smooth)
        defines += "#define NORMAL_QUALIFIER smooth\n";
      else
        defines += "#define NORMAL_QUALIFIER flat\n";

      const GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
      if(vShader <= 0) ASSERT(false);
      {} // TODO: error
      const char* totalVertexShaderSourceCode[] = {versionSourceCode, defines.c_str(), vertexShaderSourceCode};
      glShaderSource(vShader, 3, totalVertexShaderSourceCode, nullptr);
      glCompileShader(vShader);
      glGetShaderiv(vShader, GL_COMPILE_STATUS, &success);
      if(!success)
      {
        char log[512];
        glGetShaderInfoLog(vShader, 512, nullptr, log);
        fprintf(stderr, "%s\n", log);
      }

      const GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);
      if(fShader <= 0) ASSERT(false);
      {} // TODO: error
      const std::string globalAmbientLightCode = "#define GLOBAL_AMBIENT_LIGHT " + globalAmbientLight + "\n";
      std::string lightingCode = "#define LIGHTING";
      for(const auto& light : lights)
        lightingCode += " " + light;
      lightingCode += "\n";
      const char* totalFragmentShaderSourceCode[] = {versionSourceCode, defines.c_str(), globalAmbientLightCode.c_str(), lightingCode.c_str(), fragmentShaderSourceCode};
      glShaderSource(fShader, 5, totalFragmentShaderSourceCode, nullptr);
      glCompileShader(fShader);
      glGetShaderiv(fShader, GL_COMPILE_STATUS, &success);
      if(!success)
      {
        char log[512];
        glGetShaderInfoLog(fShader, 512, nullptr, log);
        fprintf(stderr, "%s\n", log);
      }

      Shader shader;
      shader.program = glCreateProgram();
      if(shader.program <= 0) ASSERT(false);
      {} // TODO: error
      glAttachShader(shader.program, vShader);
      glAttachShader(shader.program, fShader);
      glLinkProgram(shader.program);
      glGetProgramiv(shader.program, GL_LINK_STATUS, &success);
      if(!success)
      {
        char log[512];
        glGetProgramInfoLog(shader.program, 512, nullptr, log);
        fprintf(stderr, "%s\n", log);
      }

      glDeleteShader(vShader);
      glDeleteShader(fShader);

      shader.cameraPVLocation = glGetUniformLocation(shader.program, "cameraPV");
      shader.cameraPosLocation = glGetUniformLocation(shader.program, "cameraPos");
      shader.modelMatrixLocation = glGetUniformLocation(shader.program, "modelMatrix");
      shader.surfaceHasTextureLocation = glGetUniformLocation(shader.program, "surface.hasTexture");
      shader.surfaceDiffuseColorLocation = glGetUniformLocation(shader.program, "surface.diffuseColor");
      shader.surfaceAmbientColorLocation = glGetUniformLocation(shader.program, "surface.ambientColor");
      shader.surfaceSpecularColorLocation = glGetUniformLocation(shader.program, "surface.specularColor");
      shader.surfaceEmissionColorLocation = glGetUniformLocation(shader.program, "surface.emissionColor");
      shader.surfaceShininessLocation = glGetUniformLocation(shader.program, "surface.shininess");
      return shader;
    };

    auto doDepthOnlyShader = [this]()
    {
      GLint success = 0;
      const char* versionSourceCode = "#version 330 core\n";

      const GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
      if(vShader <= 0) ASSERT(false);
      {} // TODO: error
      const char* totalVertexShaderSourceCode[] = {versionSourceCode, depthOnlyVertexShaderSourceCode};
      glShaderSource(vShader, 2, totalVertexShaderSourceCode, nullptr);
      glCompileShader(vShader);
      glGetShaderiv(vShader, GL_COMPILE_STATUS, &success);
      if(!success)
      {
        char log[512];
        glGetShaderInfoLog(vShader, 512, nullptr, log);
        fprintf(stderr, "%s\n", log);
      }

      const GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);
      if(fShader <= 0) ASSERT(false);
      {} // TODO: error
      const char* totalFragmentShaderSourceCode[] = {versionSourceCode, depthOnlyFragmentShaderSourceCode};
      glShaderSource(fShader, 2, totalFragmentShaderSourceCode, nullptr);
      glCompileShader(fShader);
      glGetShaderiv(fShader, GL_COMPILE_STATUS, &success);
      if(!success)
      {
        char log[512];
        glGetShaderInfoLog(fShader, 512, nullptr, log);
        fprintf(stderr, "%s\n", log);
      }

      Shader shader;
      shader.program = glCreateProgram();
      if(shader.program <= 0) ASSERT(false);
      {} // TODO: error
      glAttachShader(shader.program, vShader);
      glAttachShader(shader.program, fShader);
      glLinkProgram(shader.program);
      glGetProgramiv(shader.program, GL_LINK_STATUS, &success);
      if(!success)
      {
        char log[512];
        glGetProgramInfoLog(shader.program, 512, nullptr, log);
        fprintf(stderr, "%s\n", log);
      }

      glDeleteShader(vShader);
      glDeleteShader(fShader);

      shader.cameraPVLocation = glGetUniformLocation(shader.program, "cameraPV");
      shader.modelMatrixLocation = glGetUniformLocation(shader.program, "modelMatrix");
      return shader;
    };

    for(unsigned int i = 0; i < 8; ++i)
      data.shaders[i] = doShader(i & 4, i & 2, i & 1);
    data.shaders[8] = doDepthOnlyShader();
  }
}

void GraphicsContext::destroyGraphics()
{
  const auto* context = QOpenGLContext::currentContext();

  if(perContextData.find(context) == perContextData.end())
    return;

  auto& data = perContextData[context];
  glDeleteVertexArrays(2, data.vao);

  // TODO: if --data.referenceCount == 0)
  // {
  //   glDeleteBuffers(1, &data.vbo);
  //   glDeleteBuffers(1, &data.ebo);
  //   glDeleteTextures(data.textureIDs.size(), data.textureIDs.data());
  //   for(const auto& shader : data.shaders)
  //     glDeleteProgram(shader.program);
  // }

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

void GraphicsContext::addLight(const ::Light* light)
{
  if(const ::DirLight* dirLight = dynamic_cast<const ::DirLight*>(light); dirLight)
    lights.push_back("calcDirLight(DirLight(vec4(" + std::to_string(dirLight->diffuseColor[0]) + ", " + std::to_string(dirLight->diffuseColor[1]) + ", " + std::to_string(dirLight->diffuseColor[2]) + ", " + std::to_string(dirLight->diffuseColor[3]) + "), vec4(" + std::to_string(dirLight->ambientColor[0]) + ", " + std::to_string(dirLight->ambientColor[1]) + ", " + std::to_string(dirLight->ambientColor[2]) + ", " + std::to_string(dirLight->ambientColor[3]) + "), vec4(" + std::to_string(dirLight->specularColor[0]) + ", " + std::to_string(dirLight->specularColor[1]) + ", " + std::to_string(dirLight->specularColor[2]) + ", " + std::to_string(dirLight->specularColor[3]) + "), vec3(" + std::to_string(dirLight->direction[0]) + ", " + std::to_string(dirLight->direction[1]) + ", " + std::to_string(dirLight->direction[2]) + ")), Normal, viewDir, diffuse, ambient, specular);");
  else if(const ::SpotLight* spotLight = dynamic_cast<const ::SpotLight*>(light); spotLight)
    lights.push_back("calcSpotLight(SpotLight(vec4(" + std::to_string(spotLight->diffuseColor[0]) + ", " + std::to_string(spotLight->diffuseColor[1]) + ", " + std::to_string(spotLight->diffuseColor[2]) + ", " + std::to_string(spotLight->diffuseColor[3]) + "), vec4(" + std::to_string(spotLight->ambientColor[0]) + ", " + std::to_string(spotLight->ambientColor[1]) + ", " + std::to_string(spotLight->ambientColor[2]) + ", " + std::to_string(spotLight->ambientColor[3]) + "), vec4(" + std::to_string(spotLight->specularColor[0]) + ", " + std::to_string(spotLight->specularColor[1]) + ", " + std::to_string(spotLight->specularColor[2]) + ", " + std::to_string(spotLight->specularColor[3]) + "), vec3(" + std::to_string(spotLight->position[0]) + ", " + std::to_string(spotLight->position[1]) + ", " + std::to_string(spotLight->position[2]) + "), " + std::to_string(spotLight->constantAttenuation) + ", " + std::to_string(spotLight->linearAttenuation) + ", " + std::to_string(spotLight->quadraticAttenuation) + ", vec3(" + std::to_string(spotLight->direction[0]) + ", " + std::to_string(spotLight->direction[1]) + ", " + std::to_string(spotLight->direction[2]) + "), " + std::to_string(spotLight->cutoff) + "), FragPos, Normal, viewDir, diffuse, ambient, specular);");
  else if(const ::PointLight* pointLight = dynamic_cast<const ::PointLight*>(light); pointLight)
    lights.push_back("calcPointLight(PointLight(vec4(" + std::to_string(pointLight->diffuseColor[0]) + ", " + std::to_string(pointLight->diffuseColor[1]) + ", " + std::to_string(pointLight->diffuseColor[2]) + ", " + std::to_string(pointLight->diffuseColor[3]) + "), vec4(" + std::to_string(pointLight->ambientColor[0]) + ", " + std::to_string(pointLight->ambientColor[1]) + ", " + std::to_string(pointLight->ambientColor[2]) + ", " + std::to_string(pointLight->ambientColor[3]) + "), vec4(" + std::to_string(pointLight->specularColor[0]) + ", " + std::to_string(pointLight->specularColor[1]) + ", " + std::to_string(pointLight->specularColor[2]) + ", " + std::to_string(pointLight->specularColor[3]) + "), vec3(" + std::to_string(pointLight->position[0]) + ", " + std::to_string(pointLight->position[1]) + ", " + std::to_string(pointLight->position[2]) + "), " + std::to_string(pointLight->constantAttenuation) + ", " + std::to_string(pointLight->linearAttenuation) + ", " + std::to_string(pointLight->quadraticAttenuation) + "), FragPos, Normal, viewDir, diffuse, ambient, specular);");
}

GraphicsContext::ModelMatrix* GraphicsContext::requestModelMatrix()
{
  modelMatrices.push_back(new ModelMatrix(modelMatrixStackStack.top().getC()));
  return modelMatrices.back();
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

void GraphicsContext::pushModelMatrix(const float* transformation)
{
  modelMatrixStackStack.top().push(transformation); // TODO: set constant flag
}

void GraphicsContext::pushModelMatrixByReference(const float* transformation)
{
  modelMatrixStackStack.top().push(transformation);
}

void GraphicsContext::popModelMatrix()
{
  modelMatrixStackStack.top().pop();
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
  {
    // TODO: proper constant handling
    Eigen::Map<Matrix4f> product(modelMatrix->memory);
    product = Matrix4f::Identity();
    for(const float* factor : modelMatrix->product)
    {
      const Eigen::Map<const Matrix4f> factorAsEigen(factor);
      product *= factorAsEigen;
    }
  }
}

void GraphicsContext::startRendering(const float* cameraProjection, const float* view, bool lighting, bool textures, bool smoothShading, bool fillPolygons)
{
  const auto* context = QOpenGLContext::currentContext();
  ASSERT(!data);
  ASSERT(!shader);
  data = &perContextData[context];
  shader = &data->shaders[(lighting ? 4 : 0) + (textures ? 2 : 0) + (smoothShading ? 1 : 0)];
  glPolygonMode(GL_FRONT_AND_BACK, fillPolygons ? GL_FILL : GL_LINE); // TODO: only GL_FRONT?
  glUseProgram(shader->program);
  float pv[16];
  Eigen::Map<Matrix4f> pvEigen(pv);
  pvEigen = Eigen::Map<const Matrix4f>(cameraProjection) * Eigen::Map<const Matrix4f>(view);
  glUniformMatrix4fv(shader->cameraPVLocation, 1, GL_FALSE, pv);
  if(shader->cameraPosLocation >= 0)
  {
    float pos[3];
    Eigen::Map<Vector3f> posEigen(pos);
    posEigen = Eigen::Map<const Matrix4f>(view).inverse().col(3).head<3>(); // TODO: this can be done faster, given that view is homogeneous
    glUniform3fv(shader->cameraPosLocation, 1, pos);
  }
}

void GraphicsContext::startDepthOnlyRendering(const float* cameraProjection, const float* view)
{
  const auto* context = QOpenGLContext::currentContext();
  ASSERT(!data);
  ASSERT(!shader);
  data = &perContextData[context];
  shader = &data->shaders[8];
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
  glUseProgram(shader->program);
  float pv[16];
  Eigen::Map<Matrix4f> pvEigen(pv);
  pvEigen = Eigen::Map<const Matrix4f>(cameraProjection) * Eigen::Map<const Matrix4f>(view);
  glUniformMatrix4fv(shader->cameraPVLocation, 1, GL_FALSE, pv);
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
  data = nullptr;
  shader = nullptr;
}

void GraphicsContext::setSurface(const Surface* surface)
{
  ASSERT(data);
  ASSERT(shader);
  if(surface->texture)
    glBindTexture(GL_TEXTURE_2D, data->textureIDs[surface->texture->index]);
  else
    glBindTexture(GL_TEXTURE_2D, 0);
  if(shader->surfaceHasTextureLocation >= 0)
    glUniform1i(shader->surfaceHasTextureLocation, surface->texture ? 1 : 0);
  if(shader->surfaceDiffuseColorLocation >= 0)
    glUniform4fv(shader->surfaceDiffuseColorLocation, 1, surface->diffuseColor);
  if(shader->surfaceAmbientColorLocation >= 0)
    glUniform4fv(shader->surfaceAmbientColorLocation, 1, surface->ambientColor);
  if(shader->surfaceSpecularColorLocation >= 0)
    glUniform4fv(shader->surfaceSpecularColorLocation, 1, surface->specularColor);
  if(shader->surfaceEmissionColorLocation >= 0)
    glUniform4fv(shader->surfaceEmissionColorLocation, 1, surface->emissionColor);
  if(shader->surfaceShininessLocation >= 0)
    glUniform1f(shader->surfaceShininessLocation, surface->shininess);
  if(surface->texture ? surface->texture->hasAlpha : (surface->diffuseColor[3] < 1.f))
  {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }
  else
    glDisable(GL_BLEND);
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
    OffscreenBuffer& buffer = offscreenBuffers[width << 16 | height << 1 | (sampleBuffers ? 1 : 0)];

    buffer.framebuffer = new QOpenGLFramebufferObject(width, height, QOpenGLFramebufferObject::Depth);
    if(!buffer.framebuffer->isValid())
    {
      delete buffer.framebuffer;
      buffer.framebuffer = nullptr;
      return false;
    }

    return true;
  }
  else
  {
    OffscreenBuffer& buffer = it->second;
    return buffer.framebuffer && buffer.framebuffer->bind();
  }
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

GraphicsContext::Texture::Texture(const std::string& file)
{
  /*
  if(file.length() >= 4)
  {
    std::string suffix = file.substr(file.length() - 4);
    if(strcasecmp(suffix.c_str(), ".tga") == 0)
      return loadTGA(file);
  }
  */

  QImage image;
  if(image.load(file.c_str()))
  {
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
}

/*
bool GraphicsContext::Texture::loadTGA(const std::string& name)
{
  GLubyte TGAheader[12] = {0, 0, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0}; // Uncompressed TGA header
  GLubyte TGAcompare[12];                                       // Used to compare TGA header
  GLubyte header[6];                                            // First 6 useful bytes of the header
  GLuint  bytesPerPixel;
  GLuint  imageSize;
  GLuint  bpp;

  FILE* file = fopen(name.c_str(), "rb");               // Open the TGA file

  // Load the file and perform checks
  if(file == nullptr ||                                                      // Does file exist?
     fread(TGAcompare, 1, sizeof(TGAcompare), file) != sizeof(TGAcompare) || // Are there 12 bytes to read?
     memcmp(TGAheader, TGAcompare, sizeof(TGAheader)) != 0 ||                // Is it the right format?
     fread(header, 1, sizeof(header), file) != sizeof(header))               // If so then read the next 6 header bytes
  {
    if(file == nullptr) // If the file didn't exist then return
      return false;
    else
    {
      fclose(file); // If something broke then close the file and return
      return false;
    }
  }

  // Determine the TGA width and height (highbyte*256+lowbyte)
  width  = header[1] * 256 + header[0];
  height = header[3] * 256 + header[2];

  // Check to make sure the targa is valid
  if(width <= 0 || height <= 0)
  {
    fclose(file);
    return false;
  }
  // Only 24 or 32 bit images are supported
  if(header[4] != 24 && header[4] != 32)
  {
    fclose(file);
    return false;
  }

  bpp = header[4];  // Grab the bits per pixel
  bytesPerPixel = bpp / 8; // Divide by 8 to get the bytes per pixel
  imageSize = width * height * bytesPerPixel; // Calculate the memory required for the data

  // Allocate the memory for the image data
  imageData = new GLubyte[imageSize];
  if(!imageData)
  {
    fclose(file);
    return false;
  }

  // Load the image data
  if(fread(imageData, 1, imageSize, file) != imageSize)  // Does the image size match the memory reserved?
  {
    delete [] imageData;  // If so, then release the image data
    fclose(file); // Close the file
    return false;
  }

  // We are done with the file so close it
  fclose(file);

  byteOrder = bpp == 24 ? GL_BGR : GL_BGRA;
  hasAlpha = byteOrder == GL_BGRA;
  return true;
}
*/

GraphicsContext::OffscreenBuffer::~OffscreenBuffer()
{
  delete framebuffer;
}
