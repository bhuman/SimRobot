/**
 * @file Graphics/GraphicsContextBase.cpp
 *
 * This file implements a class that provides the functionality of a graphics context that does not depend
 * on the graphics API.
 *
 * @author Arne Hasselbring
 */

#include "GraphicsContextBase.h"
#include "Graphics/Light.h"
#include "Graphics/LightSpace.h"
#include "Platform/Assert.h"
#include "Simulation/Simulation.h"
#include <QImage>
#include <cstring>

GraphicsContextBase::~GraphicsContextBase()
{
  for(const auto& texture : textures)
    delete texture.second;
  for(const auto& modelMatrixSet : modelMatrixSets)
  {
    for(const auto* modelMatrix : modelMatrixSet.variableModelMatrices)
      delete modelMatrix;
    for(const auto* modelMatrix : modelMatrixSet.constantModelMatrices)
      delete modelMatrix;
  }
  for(const auto* surface : surfaces)
    delete surface;
  for(const auto& category : vertexBuffers)
    for(const auto* vertexBuffer : category)
      delete vertexBuffer;
  for(const auto* indexBuffer : indexBuffers)
    delete indexBuffer;
  for(const auto* mesh : meshes)
    delete mesh;
}

void GraphicsContextBase::layoutResources()
{
  // Determine buffer memory layout of vertex buffer.
  std::uint32_t base = 0;
  std::uint64_t offset = 0;
  for(std::size_t category = 0; category < numOfVertexCategories; ++category)
  {
    // Align on multiples of the current stride to adjust the base index.
    const std::uint32_t stride = vertexStrides[category];
    offset += stride - 1;
    base = static_cast<std::uint32_t>(offset / stride);
    offset = base * static_cast<std::uint64_t>(stride);
    for(auto* buffer : vertexBuffers[category])
    {
      buffer->base = base;
      buffer->offset = offset;
      base += buffer->count;
      offset += buffer->size();
    }
  }
  vertexBufferTotalSize = offset;

  // Determine buffer memory layout of index buffer.
  offset = 0;
  for(auto* buffer : indexBuffers)
  {
    buffer->offset = offset;
    buffer->count = static_cast<std::uint32_t>(buffer->indices.size());
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

void GraphicsContextBase::setClearColor(const float* color)
{
  std::memcpy(clearColor, color, sizeof(clearColor));
}

GraphicsContextBase::IndexBuffer* GraphicsContextBase::requestIndexBuffer()
{
  return indexBuffers.emplace_back(new IndexBuffer);
}

GraphicsContextBase::Mesh* GraphicsContextBase::requestMesh(const VertexBufferBase* vertexBuffer, const IndexBuffer* indexBuffer, PrimitiveTopology primitiveTopology)
{
  Mesh* mesh = new Mesh;
  mesh->vertexBuffer = vertexBuffer;
  mesh->indexBuffer = indexBuffer;
  mesh->topology = primitiveTopology;
  meshes.push_back(mesh);
  return mesh;
}

GraphicsContextBase::Texture* GraphicsContextBase::requestTexture(const std::string& file)
{
  auto iter = textures.find(file);
  if(iter != textures.end())
  {
    Texture* texture = iter->second;
    return texture->data.empty() ? nullptr : texture;
  }
  Texture*& texture = textures[file];
  texture = new Texture;
  QImage image;
  if(!image.load(file.c_str()))
    return nullptr;
  if(image.format() != QImage::Format_ARGB32 && image.format() != QImage::Format_RGB32)
    image.convertTo(image.hasAlphaChannel() ? QImage::Format_ARGB32 : QImage::Format_RGB32); // Qt also loads grayscale, indexed, 16 bit, ... images
  texture->width = static_cast<unsigned int>(image.width());
  texture->height = static_cast<unsigned int>(image.height());
  texture->hasAlpha = image.hasAlphaChannel();
  texture->data.resize(static_cast<std::size_t>(image.sizeInBytes()));
  std::uint8_t* p = texture->data.data();
  for(int y = image.height(); y-- > 0;)
  {
    std::memcpy(p, image.scanLine(y), static_cast<std::size_t>(image.bytesPerLine()));
    p += image.bytesPerLine();
  }
  return texture;
}

GraphicsContextBase::Surface* GraphicsContextBase::requestSurface(const float* albedo, float alpha, float metallic, float roughness, float ambient, const Texture* texture)
{
  auto* surface = new Surface;
  std::memcpy(surface->albedo, albedo, sizeof(surface->albedo));
  surface->alpha = alpha;
  surface->metallic = metallic;
  surface->roughness = roughness;
  surface->ambient = ambient;
  surface->texture = texture;
  surfaces.push_back(surface);
  return surface;
}

void GraphicsContextBase::setGlobalAmbientLight(const float* color)
{
  std::memcpy(globalAmbientLight, color, sizeof(globalAmbientLight));
}

void GraphicsContextBase::addLight(const Light* light)
{
  LightData data;
  std::memcpy(data.color, light->color, sizeof(data.color));
  if(const auto* dirLight = dynamic_cast<const DirLight*>(light); dirLight)
  {
    data.type = LightData::directional;
    std::memcpy(data.direction, dirLight->direction, sizeof(data.direction));
  }
  else if(const auto* spotLight = dynamic_cast<const SpotLight*>(light); spotLight)
  {
    data.type = LightData::spot;
    std::memcpy(data.position, spotLight->position, sizeof(data.position));
    data.constantAttenuation = spotLight->constantAttenuation;
    data.linearAttenuation = spotLight->linearAttenuation;
    data.quadraticAttenuation = spotLight->quadraticAttenuation;
    std::memcpy(data.direction, spotLight->direction, sizeof(data.direction));
    data.cutoff = spotLight->cutoff;
  }
  else if(const auto* pointLight = dynamic_cast<const PointLight*>(light); pointLight)
  {
    data.type = LightData::point;
    std::memcpy(data.position, pointLight->position, sizeof(data.position));
    data.constantAttenuation = pointLight->constantAttenuation;
    data.linearAttenuation = pointLight->linearAttenuation;
    data.quadraticAttenuation = pointLight->quadraticAttenuation;
  }
  else
  {
    ASSERT(false);
    return;
  }

  if(light->shadowMap)
  {
    const std::optional<LightSpace> lightSpace = shadowMaps.size() < maxShadowMaps ? LightSpace::create(*light) : std::nullopt;
    if(lightSpace)
    {
      data.shadowMap = shadowMaps.size();
      shadowMaps.push_back({lightSpace->view, lightSpace->projection, light->shadowMap->width, light->shadowMap->height});
    }
    else
      TRACE("Light %zu does not cast shadows (too many shadow maps, or the light is inside of its bounding box, or the bounding box is behind it).", lights.size());
  }
  lights.push_back(data);
}

GraphicsContextBase::ModelMatrix* GraphicsContextBase::requestModelMatrix(ModelMatrix::Usage usage)
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
  modelMatrix->constantPart = product.size() > startIndex ? *product[startIndex] : Pose3f();
  for(std::size_t i = startIndex + 1; i < product.size(); ++i)
    modelMatrix->constantPart *= *product[i];
  modelMatrix->memory.row(3) = Eigen::RowVector4f(0.f, 0.f, 0.f, 1.f);
  if(modelMatrix->variablePart)
    modelMatrixSets[usage].variableModelMatrices.push_back(modelMatrix);
  else
  {
    modelMatrix->memory.topLeftCorner<3, 3>() = modelMatrix->constantPart.rotation;
    modelMatrix->memory.topRightCorner<3, 1>() = modelMatrix->constantPart.translation;
    modelMatrixSets[usage].constantModelMatrices.push_back(modelMatrix);
  }
  return modelMatrix;
}

void GraphicsContextBase::updateModelMatrices(ModelMatrix::Usage usage, bool forceUpdate)
{
  if(modelMatrixSets[usage].lastUpdate == Simulation::simulation->simulationStep && !forceUpdate)
    return;
  modelMatrixSets[usage].lastUpdate = Simulation::simulation->simulationStep;

  for(ModelMatrix* modelMatrix : modelMatrixSets[usage].variableModelMatrices)
    modelMatrix->updateMemory();
}

void GraphicsContextBase::ModelMatrix::updateMemory()
{
  if(!variablePart)
    return;
  const Pose3f result = *variablePart * constantPart;
  memory.topLeftCorner<3, 3>() = result.rotation;
  memory.topRightCorner<3, 1>() = result.translation;
}
