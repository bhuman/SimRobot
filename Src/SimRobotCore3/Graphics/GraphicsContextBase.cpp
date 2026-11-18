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
#include "Graphics/Primitives.h"
#include "Platform/Assert.h"
#include "Simulation/Simulation.h"
#include <QImage>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <functional>
#include <limits>
#include <map>
#include <numeric>
#include <string_view>
#include <tuple>
#include <unordered_set>

GraphicsContextBase::GraphicsContextBase(bool batchByTexture) :
  batchByTexture(batchByTexture)
{
  drawingSphereMesh = Primitives::createSphere(*this, 1.f, 16, 16, false);
  drawingCylinderMesh = Primitives::createCylinder(*this, 1.f, 1.f, 16);
  drawingQuadMesh = Primitives::createQuad(*this);
}

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
  for(const auto* readbackBuffer : readbackBuffers)
    delete readbackBuffer;
}

void GraphicsContextBase::layoutResources()
{
  // Buffers with the same contents (e.g. the meshes of the parts that several robots have in common) share their memory.
  std::vector<std::pair<VertexBufferBase*, const VertexBufferBase*>> sharedVertexBuffers;
  std::unordered_map<std::size_t, std::vector<VertexBufferBase*>> uniqueVertexBuffers;
  for(auto& category : vertexBuffers)
    for(auto* buffer : category)
    {
      const std::string_view bytes(static_cast<const char*>(buffer->data), buffer->size());
      auto& candidates = uniqueVertexBuffers[std::hash<std::string_view>()(bytes) ^ buffer->category];
      const auto original = std::find_if(candidates.begin(), candidates.end(), [&](const VertexBufferBase* candidate)
      {
        return candidate->category == buffer->category && candidate->size() == buffer->size() && !std::memcmp(candidate->data, buffer->data, buffer->size());
      });
      if(original != candidates.end())
        sharedVertexBuffers.emplace_back(buffer, *original);
      else
        candidates.push_back(buffer);
    }
  std::unordered_set<const VertexBufferBase*> sharedVertexBufferSet;
  for(const auto& [buffer, original] : sharedVertexBuffers)
    sharedVertexBufferSet.insert(buffer);

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
      if(sharedVertexBufferSet.contains(buffer))
        continue;
      buffer->base = base;
      buffer->offset = offset;
      base += buffer->count;
      offset += buffer->size();
    }
  }
  vertexBufferTotalSize = offset;
  for(const auto& [buffer, original] : sharedVertexBuffers)
  {
    buffer->base = original->base;
    buffer->offset = original->offset;
  }

  // Determine buffer memory layout of index buffer.
  offset = 0;
  std::vector<std::pair<IndexBuffer*, const IndexBuffer*>> sharedIndexBuffers;
  std::unordered_map<std::size_t, std::vector<IndexBuffer*>> uniqueIndexBuffers;
  for(auto* buffer : indexBuffers)
  {
    buffer->count = static_cast<std::uint32_t>(buffer->indices.size());
    const std::string_view bytes(reinterpret_cast<const char*>(buffer->indices.data()), buffer->indices.size() * sizeof(std::uint32_t));
    auto& candidates = uniqueIndexBuffers[std::hash<std::string_view>()(bytes)];
    const auto original = std::find_if(candidates.begin(), candidates.end(), [&](const IndexBuffer* candidate){return candidate->indices == buffer->indices;});
    if(original != candidates.end())
    {
      sharedIndexBuffers.emplace_back(buffer, *original);
      continue;
    }
    candidates.push_back(buffer);
    buffer->offset = offset;
    offset += buffer->size();
  }
  indexBufferTotalSize = offset;
  for(const auto& [buffer, original] : sharedIndexBuffers)
    buffer->offset = original->offset;

  // Determine texture indices.
  std::size_t index = 0;
  for(auto& texture : textures)
    texture.second->index = index++;

  // Determine surface indices.
  index = 0;
  for(auto* surface : surfaces)
    surface->index = index++;

  // Determine the indices of the model matrices: the constant ones first, then the variable ones, of each usage class.
  // The matrices live in the array from now on (it must not change its size anymore).
  ASSERT(modelMatrixArray.empty());
  std::uint32_t matrixIndex = 0;
  for(ModelMatrixSet& set : modelMatrixSets)
  {
    for(ModelMatrix* modelMatrix : set.constantModelMatrices)
      modelMatrix->index = matrixIndex++;
    for(ModelMatrix* modelMatrix : set.variableModelMatrices)
      modelMatrix->index = matrixIndex++;
  }
  modelMatrixArray.resize(std::max<std::uint32_t>(matrixIndex, 1)); // Never empty, so that the array has a valid address to upload.
  for(ModelMatrixSet& set : modelMatrixSets)
    for(auto* matrices : {&set.constantModelMatrices, &set.variableModelMatrices})
      for(ModelMatrix* modelMatrix : *matrices)
      {
        modelMatrix->memory = &modelMatrixArray[modelMatrix->index];
        modelMatrix->memory->row(3) = Eigen::RowVector4f(0.f, 0.f, 0.f, 1.f);
        if(modelMatrix->variablePart)
          modelMatrix->updateMemory();
        else
        {
          modelMatrix->memory->topLeftCorner<3, 3>() = modelMatrix->constantPart.rotation;
          modelMatrix->memory->topRightCorner<3, 1>() = modelMatrix->constantPart.translation;
        }
      }
  ++modelMatrixGeneration;
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

  // The bounds of the mesh (a sphere around the box of the vertices that are used by the mesh).
  const std::size_t count = indexBuffer ? indexBuffer->indices.size() : vertexBuffer->count;
  const auto position = [&](std::size_t i) -> const Vector3f& {return vertexBuffer->getPosition(indexBuffer ? indexBuffer->indices[i] : i);};
  Vector3f min = Vector3f::Constant(std::numeric_limits<float>::max());
  Vector3f max = Vector3f::Constant(std::numeric_limits<float>::lowest());
  for(std::size_t i = 0; i < count; ++i)
  {
    min = min.cwiseMin(position(i));
    max = max.cwiseMax(position(i));
  }
  mesh->boundsCenter = (min + max) * 0.5f;
  float squaredRadius = 0.f;
  for(std::size_t i = 0; i < count; ++i)
    squaredRadius = std::max(squaredRadius, (position(i) - mesh->boundsCenter).squaredNorm());
  mesh->boundsRadius = std::sqrt(squaredRadius);

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

GraphicsContextBase::ReadbackBuffer* GraphicsContextBase::requestReadbackBuffer(ImageFormat format, unsigned int width, unsigned int height, const Matrix4f* projection)
{
  auto* readbackBuffer = new ReadbackBuffer;
  readbackBuffer->format = format;
  readbackBuffer->width = width;
  readbackBuffer->height = height;
  const std::size_t pixels = static_cast<std::size_t>(width) * height;
  // The OpenGL kernels write 32 bit words (three of them per four pixels for RGB).
  readbackBuffer->size = format == rgbImage ? (pixels + 3) / 4 * 12 : pixels * (format == yuyvImage ? 2 : sizeof(float));
  readbackBuffer->index = readbackBuffers.size();
  if(format == depthImage)
  {
    // The depth d in [0, 1] corresponds to the distance p23 / 2 / (d + p22 / 2 - 1 / 2) (OpenGL conventions).
    ASSERT(projection);
    readbackBuffer->depthNumerator = (*projection)(2, 3) * 0.5f;
    readbackBuffer->depthOffset = (*projection)(2, 2) * 0.5f - 0.5f;
  }
  readbackBuffers.push_back(readbackBuffer);
  return readbackBuffer;
}

void GraphicsContextBase::readOffscreenImage(const ReadbackBuffer& buffer, int y)
{
  OffscreenReadback& readback = offscreenReadbacks.emplace_back();
  readback.buffer = &buffer;
  readback.y = y;
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

GraphicsContextBase::PreparedDrawing::PreparedDrawing(SimRobotCore3::Drawing&& drawing) :
  transformation(Eigen::Map<const Matrix4f>(drawing.transformation)), renderOptions(drawing.renderOptions)
{
  static std::uint64_t nextImageId = 0;

  const auto makeVertex = [](const float* position, const SimRobotCore3::Drawing::Color& color, float size)
  {
    Vertex vertex;
    std::memcpy(vertex.position, position, sizeof(vertex.position));
    vertex.color[0] = color.r;
    vertex.color[1] = color.g;
    vertex.color[2] = color.b;
    vertex.color[3] = color.a;
    vertex.size = size;
    return vertex;
  };

  for(const SimRobotCore3::Drawing::Line& line : drawing.lines)
  {
    vertices.push_back(makeVertex(line.start, line.color, 1.f));
    vertices.push_back(makeVertex(line.end, line.color, 1.f));
  }
  numOfLineVertices = vertices.size();

  for(const SimRobotCore3::Drawing::Dot& dot : drawing.dots)
    vertices.push_back(makeVertex(dot.point, dot.color, dot.size));
  numOfPoints = vertices.size() - numOfLineVertices;

  for(const SimRobotCore3::Drawing::Quad& quad : drawing.quads)
  {
    // The corners are the boundary of the quad: 0-1-2-3 is split along the diagonal from 1 to 3.
    static constexpr int order[] = {0, 1, 3, 3, 1, 2};
    for(int corner : order)
      vertices.push_back(makeVertex(quad.points[corner], quad.color, 1.f));
  }
  numOfTriangleVertices = vertices.size() - numOfLineVertices - numOfPoints;

  const auto addInstance = [this](const Matrix3f& rotation, const Vector3f& point, const Vector3f& scale, float baseRadius, float topRadius, const SimRobotCore3::Drawing::Color& color)
  {
    Instance& instance = instances.emplace_back();
    instance.transformation = (Matrix4f() << rotation * scale.asDiagonal(), point, Eigen::RowVector3f::Zero(), 1.f).finished();
    instance.radii[0] = baseRadius;
    instance.radii[1] = topRadius;
    instance.color[0] = color.r;
    instance.color[1] = color.g;
    instance.color[2] = color.b;
    instance.color[3] = color.a;
  };
  for(const SimRobotCore3::Drawing::Sphere& sphere : drawing.spheres)
    addInstance(Matrix3f::Identity(), Eigen::Map<const Vector3f>(sphere.point), Vector3f::Constant(sphere.radius), 1.f, 1.f, sphere.color);
  for(const SimRobotCore3::Drawing::Ellipsoid& ellipsoid : drawing.ellipsoids)
    addInstance(Eigen::Map<const Matrix3f>(ellipsoid.rotation), Eigen::Map<const Vector3f>(ellipsoid.point), Eigen::Map<const Vector3f>(ellipsoid.radii), 1.f, 1.f, ellipsoid.color);
  numOfSpheres = instances.size();

  // The radii of a cylinder are applied by the vertex shader, because they differ at its ends.
  for(const SimRobotCore3::Drawing::Cylinder& cylinder : drawing.cylinders)
    addInstance(Eigen::Map<const Matrix3f>(cylinder.rotation), Eigen::Map<const Vector3f>(cylinder.point), Vector3f(1.f, 1.f, cylinder.height), cylinder.baseRadius, cylinder.topRadius, cylinder.color);
  numOfCylinders = instances.size() - numOfSpheres;

  for(SimRobotCore3::Drawing::Image& image : drawing.images)
  {
    if(image.pixelWidth == 0 || image.pixelHeight == 0 || (!image.fill && image.rgba.size() < static_cast<std::size_t>(image.pixelWidth) * image.pixelHeight * 4))
      continue;
    Image& preparedImage = images.emplace_back();
    preparedImage.transformation = (Matrix4f() << Eigen::Map<const Matrix3f>(image.rotation) * Vector3f(1.f, image.width * 0.5f, image.height * 0.5f).asDiagonal(),
                                    Eigen::Map<const Vector3f>(image.point), Eigen::RowVector3f::Zero(), 1.f).finished();
    preparedImage.width = image.pixelWidth;
    preparedImage.height = image.pixelHeight;
    if(image.fill)
    {
      preparedImage.rgba.resize(static_cast<std::size_t>(image.pixelWidth) * image.pixelHeight * 4);
      image.fill(preparedImage.rgba.data());
    }
    else
      preparedImage.rgba = std::move(image.rgba);
    preparedImage.id = nextImageId++;
  }
}

void GraphicsContextBase::DrawingList::add(const std::shared_ptr<const PreparedDrawing>& drawing, const float* objectTransformation)
{
  if(drawing->empty())
    return;
  Item& item = items.emplace_back();
  item.drawing = drawing;
  item.transformation = Eigen::Map<const Matrix4f>(objectTransformation) * drawing->transformation;
  item.firstVertex = numOfVertices;
  item.firstInstance = numOfInstances;
  item.firstImage = numOfImages;
  numOfVertices += drawing->vertices.size();
  numOfInstances += drawing->instances.size();
  numOfImages += drawing->images.size();
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
  if(modelMatrix->variablePart)
    modelMatrixSets[usage].variableModelMatrices.push_back(modelMatrix);
  else
    modelMatrixSets[usage].constantModelMatrices.push_back(modelMatrix);
  return modelMatrix;
}

const GraphicsContextBase::DrawList* GraphicsContextBase::getDrawList(const DrawListKey& key, const std::function<void()>& record)
{
  std::unique_ptr<DrawList>& drawList = drawLists[key];
  if(!drawList)
  {
    ASSERT(!recordingList);
    drawList = std::make_unique<DrawList>();
    recordingList = drawList.get();
    record();
    ASSERT(!forcedSurface);
    recordingList = nullptr;
    buildDrawList(*drawList);
  }
  return drawList.get();
}

void GraphicsContextBase::setForcedSurface(const Surface* surface)
{
  ASSERT(recordingList);
  forcedSurface = surface;
}

void GraphicsContextBase::draw(const Mesh* mesh, const ModelMatrix* modelMatrix, const Surface* surface)
{
  ASSERT(recordingList);
  recordingList->items.push_back({mesh, modelMatrix, forcedSurface ? forcedSurface : surface});
}

void GraphicsContextBase::updateModelMatrices(ModelMatrix::Usage usage, bool forceUpdate)
{
  if(modelMatrixSets[usage].lastUpdate == Simulation::simulation->simulationStep && !forceUpdate)
    return;
  modelMatrixSets[usage].lastUpdate = Simulation::simulation->simulationStep;

  for(ModelMatrix* modelMatrix : modelMatrixSets[usage].variableModelMatrices)
    modelMatrix->updateMemory();
  if(!modelMatrixSets[usage].variableModelMatrices.empty())
    ++modelMatrixGeneration;
}

void GraphicsContextBase::setupCulling(const Matrix4f& projectionView)
{
  // The shadow maps must contain everything that can cast a shadow into the view, so nothing is excluded from them.
  if(renderingShadowMap)
    viewFrustum.reset();
  else
    viewFrustum.emplace().set(projectionView);
}

bool GraphicsContextBase::cullInstances(const DrawList::Pass& pass, VisibleInstances& visible) const
{
  ASSERT(viewFrustum);
  visible.instances.clear();
  visible.ranges.clear();
  for(const DrawList::Batch& batch : pass.batches)
  {
    const Mesh& mesh = *batch.mesh;
    const std::uint32_t first = static_cast<std::uint32_t>(visible.instances.size());
    for(std::uint32_t i = batch.firstInstance; i < batch.firstInstance + batch.numOfInstances; ++i)
    {
      const DrawList::Instance& instance = pass.instances[i];
      const Matrix4f& modelMatrix = modelMatrixArray[instance.modelMatrix];
      // Model matrices are rigid, so the radius does not change.
      if(viewFrustum->intersects(modelMatrix.topLeftCorner<3, 3>() * mesh.boundsCenter + modelMatrix.topRightCorner<3, 1>(), mesh.boundsRadius))
        visible.instances.push_back(instance);
    }
    visible.ranges.emplace_back(first, static_cast<std::uint32_t>(visible.instances.size()) - first);
  }
  return visible.instances.size() != pass.instances.size();
}

void GraphicsContextBase::buildDrawList(DrawList& drawList) const
{
  const std::vector<DrawList::Item>& items = drawList.items;

  // Meshes with the same geometry (they might use different objects with the same contents) get the same number.
  std::map<std::tuple<std::size_t, std::uint32_t, std::uint32_t, std::uint64_t, std::uint32_t, int>, std::size_t> meshNumbers;
  std::vector<std::size_t> meshNumber(items.size());
  for(std::size_t i = 0; i < items.size(); ++i)
  {
    const Mesh& mesh = *items[i].mesh;
    const auto key = std::make_tuple(mesh.vertexBuffer->category, mesh.vertexBuffer->base, mesh.vertexBuffer->count,
                                     mesh.indexBuffer ? mesh.indexBuffer->offset : ~std::uint64_t(0), mesh.indexBuffer ? mesh.indexBuffer->count : 0u, static_cast<int>(mesh.topology));
    meshNumber[i] = meshNumbers.try_emplace(key, meshNumbers.size()).first->second;
  }
  const auto category = [&](std::size_t i){return items[i].mesh->vertexBuffer->category;};
  const auto texture = [&](std::size_t i){return items[i].surface->texture;};
  const auto textureNumber = [&](std::size_t i){return batchByTexture && texture(i) ? texture(i)->index + 1 : std::size_t(0);};

  // Makes the instances in the given order batches of the same mesh (and texture, if required) that follow each other.
  const auto addBatches = [&](DrawList::Pass& pass, const std::vector<std::size_t>& order, bool withSurfaces, bool blend)
  {
    const std::size_t* previous = nullptr;
    for(const std::size_t& i : order)
    {
      if(!previous || category(*previous) != category(i) || meshNumber[*previous] != meshNumber[i] || (withSurfaces && textureNumber(*previous) != textureNumber(i)))
      {
        DrawList::Batch& batch = pass.batches.emplace_back();
        batch.mesh = items[i].mesh;
        batch.firstInstance = static_cast<std::uint32_t>(pass.instances.size());
        batch.numOfInstances = 0;
        batch.texture = withSurfaces && batchByTexture ? texture(i) : nullptr;
        batch.blend = blend;
      }
      ++pass.batches.back().numOfInstances;
      pass.instances.push_back({items[i].modelMatrix->index, withSurfaces ? static_cast<std::uint32_t>(items[i].surface->index) : 0u});
      previous = &i;
    }
  };

  // Colors: Opaque instances are sorted (this keeps the order of equal ones), transparent ones must stay in their order.
  const auto isTransparent = [&](std::size_t i)
  {
    const Surface& surface = *items[i].surface;
    return surface.texture ? surface.texture->hasAlpha : surface.alpha < 1.f;
  };
  std::vector<std::size_t> opaque, transparent;
  for(std::size_t i = 0; i < items.size(); ++i)
    (isTransparent(i) ? transparent : opaque).push_back(i);
  std::stable_sort(opaque.begin(), opaque.end(), [&](std::size_t a, std::size_t b)
  {
    return std::make_tuple(category(a), textureNumber(a), meshNumber[a]) < std::make_tuple(category(b), textureNumber(b), meshNumber[b]);
  });
  drawList.color = DrawList::Pass();
  addBatches(drawList.color, opaque, true, false);
  addBatches(drawList.color, transparent, true, true);

  // Depths: All instances are drawn, and the surfaces do not matter.
  std::vector<std::size_t> all(items.size());
  std::iota(all.begin(), all.end(), std::size_t(0));
  std::stable_sort(all.begin(), all.end(), [&](std::size_t a, std::size_t b)
  {
    return std::make_tuple(category(a), meshNumber[a]) < std::make_tuple(category(b), meshNumber[b]);
  });
  drawList.depthOnly = DrawList::Pass();
  addBatches(drawList.depthOnly, all, false, false);
}

void GraphicsContextBase::ModelMatrix::updateMemory()
{
  if(!variablePart)
    return;
  const Pose3f result = *variablePart * constantPart;
  memory->topLeftCorner<3, 3>() = result.rotation;
  memory->topRightCorner<3, 1>() = result.translation;
}
