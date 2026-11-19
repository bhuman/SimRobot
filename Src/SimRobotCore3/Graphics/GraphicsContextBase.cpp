/**
 * @file Graphics/GraphicsContextBase.cpp
 *
 * This file implements a class that provides the functionality of a graphics context that does not depend
 * on the graphics API.
 *
 * @author Arne Hasselbring
 */

#include "GraphicsContextBase.h"
#include "Graphics/Environment.h"
#include "Graphics/Ground.h"
#include "Graphics/Light.h"
#include "Graphics/Backdrop.h"
#include "Graphics/LightSpace.h"
#include "Graphics/Primitives.h"
#include "Platform/Assert.h"
#include "Simulation/Simulation.h"
#include <QFont>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QImage>
#include <QPainter>
#include <QString>
#include <meshoptimizer.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <limits>
#include <map>
#include <numeric>
#include <string_view>
#include <tuple>

GraphicsContextBase::GraphicsContextBase()
{
  if(const char* lodPixelErrorString = std::getenv("SIMROBOT_LOD_PIXEL_ERROR"))
    lodPixelError = static_cast<float>(std::atof(lodPixelErrorString));
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
  // The bounds of the meshes (a sphere around the box of the vertices that are used by the mesh).
  for(Mesh* mesh : meshes)
  {
    const char* const vertices = static_cast<const char*>(mesh->vertexBuffer->data);
    const std::size_t stride = vertexStrides[mesh->vertexBuffer->category];
    const std::size_t count = mesh->indexBuffer ? mesh->indexBuffer->indices.size() : mesh->vertexBuffer->count;
    const auto position = [&](std::size_t i)
    {
      Vector3f result; // The position is the first member of all vertex types.
      std::memcpy(result.data(), vertices + (mesh->indexBuffer ? mesh->indexBuffer->indices[i] : i) * stride, sizeof(result));
      return result;
    };
    Vector3f min = Vector3f::Constant(std::numeric_limits<float>::max());
    Vector3f max = Vector3f::Constant(std::numeric_limits<float>::lowest());
    for(std::size_t i = 0; i < count; ++i)
    {
      const Vector3f p = position(i);
      min = min.cwiseMin(p);
      max = max.cwiseMax(p);
    }
    mesh->boundsCenter = (min + max) * 0.5f;
    mesh->boundsHalfExtent = (max - min) * 0.5f;
    float squaredRadius = 0.f;
    for(std::size_t i = 0; i < count; ++i)
      squaredRadius = std::max(squaredRadius, (position(i) - mesh->boundsCenter).squaredNorm());
    mesh->boundsRadius = std::sqrt(squaredRadius);
  }

  // Meshes with many triangles get simplified versions that use a subset of their vertices (so the vertex buffers stay as they are).
  // Meshes with equal contents get the same versions.
  if(lodPixelError > 0.f)
  {
    // The maximum errors of the simplified versions (in meters, which are the units of model space).
    static constexpr float lodErrors[maxLods] = {0.001f, 0.003f, 0.009f, 0.027f};
    static constexpr std::size_t minLodTriangles = 256; // Smaller meshes are not worth the trouble.
    std::unordered_map<std::size_t, std::vector<std::pair<const Mesh*, std::vector<Mesh::Lod>>>> cache;
    for(Mesh* mesh : meshes)
    {
      if(mesh->topology != triangleList || !mesh->indexBuffer || mesh->indexBuffer->indices.size() < minLodTriangles * 3)
        continue;
      const std::size_t stride = vertexStrides[mesh->vertexBuffer->category];
      const std::string_view vertexBytes(static_cast<const char*>(mesh->vertexBuffer->data), mesh->vertexBuffer->size());
      const std::string_view indexBytes(reinterpret_cast<const char*>(mesh->indexBuffer->indices.data()), mesh->indexBuffer->indices.size() * sizeof(std::uint32_t));
      auto& candidates = cache[std::hash<std::string_view>()(vertexBytes) ^ (std::hash<std::string_view>()(indexBytes) << 1)];
      const auto original = std::find_if(candidates.begin(), candidates.end(), [&](const auto& candidate)
      {
        const Mesh& other = *candidate.first;
        return other.vertexBuffer->category == mesh->vertexBuffer->category && other.vertexBuffer->size() == mesh->vertexBuffer->size() &&
               !std::memcmp(other.vertexBuffer->data, mesh->vertexBuffer->data, mesh->vertexBuffer->size()) && other.indexBuffer->indices == mesh->indexBuffer->indices;
      });
      if(original != candidates.end())
      {
        mesh->lods = original->second;
        continue;
      }

      // Normals and (if there are any) texture coordinates follow the position in all vertex types. They keep shading and textures intact.
      const float* const positions = static_cast<const float*>(mesh->vertexBuffer->data);
      const float* const attributes = positions + 3;
      const std::size_t numOfAttributes = stride / sizeof(float) - 3;
      static constexpr float attributeWeights[5] = {1.f, 1.f, 1.f, 0.5f, 0.5f};
      ASSERT(numOfAttributes <= 5);
      const std::vector<std::uint32_t>& indices = mesh->indexBuffer->indices;
      std::size_t previousSize = indices.size();
      std::vector<Mesh::Lod> lods;
      for(const float targetError : lodErrors)
      {
        std::vector<std::uint32_t> simplified(indices.size());
        float error = 0.f;
        simplified.resize(meshopt_simplifyWithAttributes(simplified.data(), indices.data(), indices.size(), positions, mesh->vertexBuffer->count, stride,
                                                         attributes, stride, attributeWeights, numOfAttributes, nullptr, 0, targetError, meshopt_SimplifyErrorAbsolute, &error));
        // Versions that are not much smaller are not worth it.
        if(simplified.size() < 3 * 4 || simplified.size() * 5 > previousSize * 4)
          continue;
        meshopt_optimizeVertexCache(simplified.data(), simplified.data(), simplified.size(), mesh->vertexBuffer->count);
        IndexBuffer* indexBuffer = requestIndexBuffer();
        indexBuffer->indices = std::move(simplified);
        previousSize = indexBuffer->indices.size();
        lods.push_back({indexBuffer, error});
      }
      mesh->lods = lods;
      candidates.emplace_back(mesh, std::move(lods));
    }
  }

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
  const auto isShared = [&](const VertexBufferBase* buffer)
  {
    return std::any_of(sharedVertexBuffers.begin(), sharedVertexBuffers.end(), [&](const auto& pair) {return pair.first == buffer;});
  };

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
      if(isShared(buffer))
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
    const auto original = std::find_if(candidates.begin(), candidates.end(), [&](const IndexBuffer* candidate) {return candidate->indices == buffer->indices;});
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
  std::uint32_t matrixIndex = 0;
  for(ModelMatrixSet& set : modelMatrixSets)
  {
    for(ModelMatrix* modelMatrix : set.constantModelMatrices)
      modelMatrix->index = matrixIndex++;
    for(ModelMatrix* modelMatrix : set.variableModelMatrices)
      modelMatrix->index = matrixIndex++;
  }
  modelMatrixArray.resize(matrixIndex);
  for(ModelMatrixSet& set : modelMatrixSets)
  {
    for(const ModelMatrix* modelMatrix : set.constantModelMatrices)
      modelMatrixArray[modelMatrix->index] = modelMatrix->memory;
    for(ModelMatrix* modelMatrix : set.variableModelMatrices)
    {
      modelMatrix->updateMemory();
      modelMatrixArray[modelMatrix->index] = modelMatrix->memory;
    }
  }
  ++modelMatrixGeneration;
}

void GraphicsContextBase::setEnvironment(const Environment& environment)
{
  std::memcpy(clearColor, environment.color, sizeof(clearColor));
  std::memcpy(globalAmbientLight, environment.ambient, sizeof(globalAmbientLight));
  std::memcpy(globalAmbientLightGround, environment.ambientGround, sizeof(globalAmbientLightGround));
  exposure = environment.exposure;
  filmicToneMapping = environment.filmicToneMapping;
  for(Light* light : environment.lights)
    addLight(light);
  if(environment.backdrop)
  {
    ASSERT(!hasSky);
    environmentMap = EnvironmentMap::create(*environment.backdrop);
    roomSize = environment.backdrop->size;
  }
  if(environment.ground)
  {
    const Ground& source = *environment.ground;
    ground.present = true;
    for(int i = 0; i < 3; ++i)
      ground.albedo[i] = std::pow(source.color[i], 2.2f);
    ground.height = source.height;
    ground.tileSize = source.tileSize;
    ground.jointWidth = source.jointWidth;
    // The line of trees is part of the sky, a backdrop has walls instead.
    ground.treeDistance = environment.backdrop ? 0.f : source.treeDistance;
    ground.treeHeight = source.treeHeight;
  }
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

GraphicsContextBase::Surface* GraphicsContextBase::requestSurface(const float* albedo, float alpha, float metallic, float roughness, float ambient, const Texture* texture, float textureVariation,
                                                                    const float* sheen, float sheenRoughness, const Texture* normalMap, float normalScale, const Texture* roughnessMap)
{
  auto* surface = new Surface;
  std::memcpy(surface->albedo, albedo, sizeof(surface->albedo));
  surface->alpha = alpha;
  surface->metallic = metallic;
  surface->roughness = roughness;
  surface->ambient = ambient;
  surface->texture = texture;
  surface->textureVariation = textureVariation;
  if(sheen)
    std::memcpy(surface->sheen, sheen, sizeof(surface->sheen));
  surface->sheenRoughness = sheenRoughness;
  surface->normalMap = normalMap;
  surface->normalScale = normalScale;
  surface->roughnessMap = roughnessMap;
  surfaces.push_back(surface);
  return surface;
}

GraphicsContextBase::ReadbackBuffer* GraphicsContextBase::requestReadbackBuffer(ImageFormat format, unsigned int width, unsigned int height, const Matrix4f* projection, const SensorModel* sensorModel)
{
  auto* readbackBuffer = new ReadbackBuffer;
  if(sensorModel)
    readbackBuffer->sensorModel = *sensorModel;
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

void GraphicsContextBase::setSky(const SkyState& state)
{
  sky = state;
  if(skyLight)
  {
    LightData& data = lights[*skyLight];
    std::memcpy(data.color, sky.lightColor.data(), sizeof(data.color));
    std::memcpy(data.direction, sky.lightDirection.data(), sizeof(data.direction));
    if(data.shadowMap && sky.lightView && sky.lightProjection)
    {
      shadowMaps[*data.shadowMap].view = *sky.lightView;
      shadowMaps[*data.shadowMap].projection = *sky.lightProjection;
    }
  }
  ++skyGeneration;
}

std::array<Vector4f, 8> GraphicsContextBase::getSkyLightValues() const
{
  std::array<Vector4f, 8> values;
  values[0] << sky.lightColor, sky.exposure;
  values[1] << sky.lightDirection, 0.f;
  values[2] << sky.ambientSky, 0.f;
  values[3] << sky.ambientGround, 0.f;
  Matrix4f lightPV = Matrix4f::Identity();
  if(skyLight && lights[*skyLight].shadowMap)
    lightPV = shadowMaps[*lights[*skyLight].shadowMap].projection * shadowMaps[*lights[*skyLight].shadowMap].view;
  for(int i = 0; i < 4; ++i)
    values[4 + i] = lightPV.col(i);
  return values;
}

std::array<Vector4f, GraphicsContextBase::numOfSkyUniforms> GraphicsContextBase::getSkyUniforms(const Matrix4f& projection, const Matrix4f& view) const
{
  constexpr float pi = 3.14159265358979323846f;
  std::array<Vector4f, numOfSkyUniforms> values;
  values.fill(Vector4f::Zero());
  const Vector3f cameraPosition = -view.topLeftCorner<3, 3>().transpose() * view.topRightCorner<3, 1>();
  if(hasSky)
  {
    values[0] << sky.sunDirection, sky.day;
    values[1] << sky.moonDirection, sky.moonRadius;
    values[2] << sky.sunColor, sky.twilight;
    values[3] << sky.night, sky.aurora, sky.time, sky.exposure;
    values[4] << sky.auroraDirection, 0.f;
    for(int i = 0; i < 3; ++i)
      values[5 + i] << sky.stars.row(i).transpose(), 0.f;
    values[15] << ground.treeDistance, ground.treeHeight, sky.clouds, 0.f;
    values[16] << sky.lightColor / pi, sky.leaves;
    values[17] << -sky.lightDirection.normalized(), sky.autumn;
    values[18] << sky.ambientSky, 0.f;
  }
  else
  {
    // A backdrop does not have a sun, so its values are those of the room: half of its size in x and y, its height and the
    // height of the point where the cube map was computed.
    values[0] << 0.5f * roomSize.x(), 0.5f * roomSize.y(), roomSize.z(), std::min(0.5f, 0.5f * roomSize.z());
  }
  for(int i = 0; i < 3; ++i)
    // The rotation from view space to the scene is the transpose of the one of the view matrix.
    values[8 + i] << view.topLeftCorner<3, 3>().col(i), 0.f;
  values[11] << projection(0, 0), projection(1, 1), 0.f, 0.f;

  // The ground: the light that a horizontal surface gets (as for the diffuse part of the shaders of lit surfaces, without shadows).
  if(ground.present)
  {
    Vector3f light = Vector3f::Zero();
    if(hasSky)
      light = sky.lightColor * std::max(-sky.lightDirection.normalized().z(), 0.f) / pi + sky.ambientSky;
    else if(environmentMap)
    {
      const std::array<float, 9> basis = EnvironmentMap::basis(Vector3f::UnitZ());
      for(std::size_t i = 0; i < basis.size(); ++i)
        light += environmentMap->irradiance[i] * basis[i];
      for(const LightData& data : lights)
        if(data.type == LightData::directional)
          light += Vector3f(data.color[0], data.color[1], data.color[2]) * std::max(-Vector3f(data.direction[0], data.direction[1], data.direction[2]).normalized().z(), 0.f) / pi;
    }
    values[12] << ground.albedo, ground.height;
    values[13] << light, ground.tileSize;
    values[14] << cameraPosition, 1.f;
    // The width of the joints is given relative to the size of the tiles.
    values[15].w() = ground.tileSize > 0.f ? ground.jointWidth / ground.tileSize : 0.f;
  }
  return values;
}

void GraphicsContextBase::addLight(Light* light)
{
  LightData data;
  data.dynamic = light->isDynamic();
  if(data.dynamic)
  {
    ASSERT(!skyLight);
    hasSky = true;
    skyLight = lights.size();
  }
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
      shadowMaps.push_back({lightSpace->view, lightSpace->projection, light->shadowMap->width, light->shadowMap->height, light});
    }
    else
      TRACE("Light %zu does not cast shadows (too many shadow maps, or the light is inside of its bounding box, or the bounding box is behind it).", lights.size());
  }
  lights.push_back(data);
}

namespace
{
  constexpr int textCellWidth = 24; /**< The width of the cell of a character of a text in pixels (0.6 times its height, as in SimRobotCore3::Drawing::Text). */
  constexpr int textCellHeight = 40; /**< The height of the cell of a character of a text in pixels. */

  /** Renders a text with a monospace font: every character is centered in a cell, so the layout does not depend on the font. */
  QImage renderText(const std::string& text, const SimRobotCore3::Drawing::Color& color, bool outline)
  {
    QFont font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    for(int size = textCellHeight - 5; size > 4; --size)
    {
      font.setPixelSize(size);
      const QFontMetrics metrics(font);
      if(metrics.height() <= textCellHeight - 2 && metrics.maxWidth() <= textCellWidth - 2)
        break;
    }
    QImage image(std::max<int>(static_cast<int>(text.size()), 1) * textCellWidth, textCellHeight, QImage::Format_RGBA8888);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::TextAntialiasing);
    painter.setFont(font);
    for(std::size_t i = 0; i < text.size(); ++i)
    {
      const char character = text[i] >= ' ' && text[i] < 127 ? text[i] : ' ';
      const QString string(QChar(static_cast<ushort>(character)));
      const QRect cell(static_cast<int>(i) * textCellWidth, 0, textCellWidth, textCellHeight);
      if(outline)
      {
        painter.setPen(QColor(0, 0, 0, 200));
        for(int dx = -1; dx <= 1; ++dx)
          for(int dy = -1; dy <= 1; ++dy)
            if(dx || dy)
              painter.drawText(cell.translated(dx, dy), Qt::AlignCenter, string);
      }
      painter.setPen(QColor(color.r, color.g, color.b, color.a));
      painter.drawText(cell, Qt::AlignCenter, string);
    }
    return image;
  }
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

  // Texts are images that are turned towards the viewer.
  for(const SimRobotCore3::Drawing::Text& text : drawing.texts)
  {
    if(text.text.empty() || text.height <= 0.f)
      continue;
    const QImage rendered = renderText(text.text, text.color, text.outline);
    Image& preparedImage = images.emplace_back();
    preparedImage.transformation = (Matrix4f() << Matrix3f::Identity(), Eigen::Map<const Vector3f>(text.point), Eigen::RowVector3f::Zero(), 1.f).finished();
    preparedImage.width = static_cast<unsigned int>(rendered.width());
    preparedImage.height = static_cast<unsigned int>(rendered.height());
    preparedImage.rgba.resize(static_cast<std::size_t>(rendered.width()) * rendered.height() * 4);
    for(int y = 0; y < rendered.height(); ++y)
      std::memcpy(preparedImage.rgba.data() + static_cast<std::size_t>(y) * rendered.width() * 4, rendered.constScanLine(y), static_cast<std::size_t>(rendered.width()) * 4);
    preparedImage.billboard = true;
    preparedImage.size[0] = static_cast<float>(text.text.size()) * 0.6f * text.height;
    preparedImage.size[1] = text.height;
    // The center of the text relative to the anchor (to the right and up).
    preparedImage.offset[0] = text.horizontal == SimRobotCore3::Drawing::Text::left ? 0.5f * preparedImage.size[0] : text.horizontal == SimRobotCore3::Drawing::Text::right ? -0.5f * preparedImage.size[0] : 0.f;
    preparedImage.offset[1] = text.vertical == SimRobotCore3::Drawing::Text::top ? -0.5f * preparedImage.size[1] : text.vertical == SimRobotCore3::Drawing::Text::bottom ? 0.5f * preparedImage.size[1] : 0.f;
    preparedImage.id = nextImageId++;
  }
}

Matrix4f GraphicsContextBase::imageToWorld(const DrawingList::Item& item, const PreparedDrawing::Image& image, const Matrix4f& view)
{
  if(!image.billboard)
    return item.transformation * image.transformation;

  // The unit quad is turned so that its normal points away from the viewer and its top is up in the view. The size is scaled like the drawing.
  const float scale = item.transformation.col(0).head<3>().norm();
  const Vector3f anchor = (item.transformation * image.transformation).topRightCorner<3, 1>();
  const Vector3f right = view.block<1, 3>(0, 0).transpose();
  const Vector3f up = view.block<1, 3>(1, 0).transpose();
  const Vector3f back = view.block<1, 3>(2, 0).transpose();
  Matrix4f result = Matrix4f::Identity();
  result.col(0).head<3>() = -back; // the image is seen along its x axis
  result.col(1).head<3>() = -right * (0.5f * image.size[0] * scale); // the first pixel of each row is at +y, which is on the left for the viewer
  result.col(2).head<3>() = up * (0.5f * image.size[1] * scale);
  result.col(3).head<3>() = anchor + (right * image.offset[0] + up * image.offset[1]) * scale;
  return result;
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
  {
    modelMatrix->updateMemory();
    if(modelMatrix->index < modelMatrixArray.size())
      modelMatrixArray[modelMatrix->index] = modelMatrix->memory;
  }
  if(!modelMatrixSets[usage].variableModelMatrices.empty())
    ++modelMatrixGeneration;
}

void GraphicsContextBase::draw(const Mesh* mesh, const ModelMatrix* modelMatrix, const Surface* surface)
{
  ASSERT(recordingList);
  recordingList->items.push_back({mesh, modelMatrix, surface});
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
    recordingList = nullptr;
    buildDrawList(*drawList);
  }
  return drawList.get();
}

void GraphicsContextBase::startCulling(const Matrix4f& projection, const Matrix4f& view, int viewportHeight)
{
  cullingActive = true;
  frustumCulling = !renderingShadowMap;
  viewFrustum.set(projection, view, viewportHeight);
}

void GraphicsContextBase::cullInstances(const DrawList::Pass& pass, bool allowLods, VisibleInstances& visible) const
{
  visible.instances.clear();
  visible.ranges.clear();
  allowLods &= lodPixelError > 0.f && viewFrustum.pixelsPerMeterAtOne > 0.f;
  const float maxPixelError = renderingShadowMap ? shadowLodTexelError : lodPixelError;
  bool all = true;
  auto& lodInstances = visible.instancesOfLod;
  for(std::uint32_t b = 0; b < pass.batches.size(); ++b)
  {
    const DrawList::Batch& batch = pass.batches[b];
    const Mesh& mesh = *batch.mesh;
    const bool useLods = allowLods && !batch.blend && !mesh.lods.empty();
    for(auto& instances : lodInstances)
      instances.clear();
    for(std::uint32_t i = batch.firstInstance; i < batch.firstInstance + batch.numOfInstances; ++i)
    {
      const DrawList::Instance& instance = pass.instances[i];
      const Matrix4f& modelMatrix = modelMatrixArray[instance.modelMatrix];
      // Model matrices are rigid, so the radius does not change.
      const Vector3f center = modelMatrix.topLeftCorner<3, 3>() * mesh.boundsCenter + modelMatrix.topRightCorner<3, 1>();
      if(frustumCulling && !viewFrustum.intersects(center, mesh.boundsRadius))
      {
        all = false;
        continue;
      }
      std::size_t lod = 0;
      if(useLods)
      {
        // The error of the level must not be seen in the nearest part of the mesh.
        const float maxError = maxPixelError * std::max(viewFrustum.nearestDistance(center, mesh.boundsRadius), 0.01f) / viewFrustum.pixelsPerMeterAtOne;
        while(lod < mesh.lods.size() && mesh.lods[lod].error <= maxError)
          ++lod;
        all &= lod == 0;
      }
      lodInstances[lod].push_back(instance);
    }
    for(std::uint32_t lod = 0; lod <= maxLods; ++lod)
      if(!lodInstances[lod].empty())
      {
        visible.ranges.push_back({b, lod, static_cast<std::uint32_t>(visible.instances.size()), static_cast<std::uint32_t>(lodInstances[lod].size())});
        visible.instances.insert(visible.instances.end(), lodInstances[lod].begin(), lodInstances[lod].end());
      }
  }
  visible.all = all;
}

bool GraphicsContextBase::fitShadowMaps(const std::function<void()>& draw)
{
  const auto needsFit = [](const ShadowMap& shadowMap) {return shadowMap.light->shadowMap->autoFit && !shadowMap.light->shadowMap->fitted;};
  if(std::none_of(shadowMaps.begin(), shadowMaps.end(), needsFit))
    return false;

  // The bounds of everything that is drawn (this does not render anything).
  fitMinimum = fitFlatMinimum = Vector3f::Constant(std::numeric_limits<float>::max());
  fitMaximum = fitFlatMaximum = Vector3f::Constant(std::numeric_limits<float>::lowest());
  fittingShadowMaps = true;
  draw();
  fittingShadowMaps = false;
  if((fitMinimum.array() > fitMaximum.array()).any())
  {
    // There are only flat surfaces.
    fitMinimum = fitFlatMinimum;
    fitMaximum = fitFlatMaximum;
  }

  // Everything that moves a bit or is a bit higher than at the start should still cast shadows.
  constexpr float margin = 0.75f; // The shadows of objects of the minimum height fall about that far from them.
  constexpr float minHeight = 2.5f;
  Vector3f center = Vector3f::Zero(), extent = Vector3f::Ones();
  if((fitMinimum.array() <= fitMaximum.array()).all())
  {
    fitMinimum.array() -= margin;
    fitMaximum.array() += margin;
    fitMaximum.z() = std::max(fitMaximum.z(), fitMinimum.z() + minHeight);
    center = (fitMinimum + fitMaximum) * 0.5f;
    extent = fitMaximum - fitMinimum;
  }
  for(ShadowMap& shadowMap : shadowMaps)
    if(needsFit(shadowMap))
    {
      TRACE("Shadow map fitted to the scene: center (%.2f, %.2f, %.2f) m, extent (%.2f, %.2f, %.2f) m.", center.x(), center.y(), center.z(), extent.x(), extent.y(), extent.z());
      Light::ShadowMap& parameters = *shadowMap.light->shadowMap;
      parameters.aabbCenter = center;
      parameters.aabbExtent = extent;
      parameters.fitted = true;
      if(const std::optional<LightSpace> lightSpace = LightSpace::create(*shadowMap.light))
      {
        shadowMap.view = lightSpace->view;
        shadowMap.projection = lightSpace->projection;
      }
    }
  ++skyGeneration; // The matrices of the shadow maps are part of what the shaders read for the sky light.
  return true;
}

void GraphicsContextBase::accumulateBounds(const DrawList::Pass& pass)
{
  constexpr float flatHeight = 0.01f; // The half height of the bounds of surfaces that are considered to be flat.
  for(const DrawList::Batch& batch : pass.batches)
  {
    const Mesh& mesh = *batch.mesh;
    for(std::uint32_t i = batch.firstInstance; i < batch.firstInstance + batch.numOfInstances; ++i)
    {
      const Matrix4f& modelMatrix = modelMatrixArray[pass.instances[i].modelMatrix];
      const Vector3f center = modelMatrix.topLeftCorner<3, 3>() * mesh.boundsCenter + modelMatrix.topRightCorner<3, 1>();
      const Vector3f halfExtent = modelMatrix.topLeftCorner<3, 3>().cwiseAbs() * mesh.boundsHalfExtent;
      // Horizontal surfaces (floors, lines) only receive shadows, so they do not make the box larger if there is something else.
      Vector3f& minimum = halfExtent.z() < flatHeight ? fitFlatMinimum : fitMinimum;
      Vector3f& maximum = halfExtent.z() < flatHeight ? fitFlatMaximum : fitMaximum;
      minimum = minimum.cwiseMin(center - halfExtent);
      maximum = maximum.cwiseMax(center + halfExtent);
    }
  }
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
  const auto category = [&](std::size_t i) {return items[i].mesh->vertexBuffer->category;};
  const auto texture = [&](std::size_t i) {return items[i].surface->texture;};
  // Instances with the same textures get the same number (which is 0 if the graphics API selects them per instance or there are none).
  std::map<std::tuple<std::size_t, std::size_t, std::size_t>, std::size_t> textureNumbers;
  const auto textureNumber = [&](std::size_t i) -> std::size_t
  {
    const Surface& surface = *items[i].surface;
    if(!batchByTexture || (!surface.texture && !surface.normalMap && !surface.roughnessMap))
      return 0;
    const auto key = std::make_tuple(surface.texture ? surface.texture->index + 1 : 0, surface.normalMap ? surface.normalMap->index + 1 : 0, surface.roughnessMap ? surface.roughnessMap->index + 1 : 0);
    return textureNumbers.try_emplace(key, textureNumbers.size() + 1).first->second;
  };

  // Makes the instances in the given order batches of the same mesh (and texture, if required) that follow each other.
  const auto addBatches = [&](DrawList::Pass& pass, const std::vector<std::size_t>& order, bool withSurfaces, bool blend, bool alphaTest = false)
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
        batch.surface = withSurfaces && batchByTexture ? items[i].surface : nullptr;
        batch.blend = blend;
        batch.alphaTest = alphaTest;
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

  // Depths: All instances are drawn. The surfaces do not matter, except for those with textures with alpha (the fragments of
  // the instances that have them are discarded where the texture is transparent), which are drawn after the others.
  const auto isCutout = [&](std::size_t i) {return texture(i) && texture(i)->hasAlpha && category(i) == VertexPNT::category;};
  std::vector<std::size_t> solid, cutouts;
  for(std::size_t i = 0; i < items.size(); ++i)
    (isCutout(i) ? cutouts : solid).push_back(i);
  std::stable_sort(solid.begin(), solid.end(), [&](std::size_t a, std::size_t b)
  {
    return std::make_tuple(category(a), meshNumber[a]) < std::make_tuple(category(b), meshNumber[b]);
  });
  std::stable_sort(cutouts.begin(), cutouts.end(), [&](std::size_t a, std::size_t b)
  {
    return std::make_tuple(category(a), textureNumber(a), meshNumber[a]) < std::make_tuple(category(b), textureNumber(b), meshNumber[b]);
  });
  drawList.depthOnly = DrawList::Pass();
  addBatches(drawList.depthOnly, solid, false, false);
  addBatches(drawList.depthOnly, cutouts, true, false, true);
}

void GraphicsContextBase::ModelMatrix::updateMemory()
{
  if(!variablePart)
    return;
  const Pose3f result = *variablePart * constantPart;
  memory.topLeftCorner<3, 3>() = result.rotation;
  memory.topRightCorner<3, 1>() = result.translation;
}
