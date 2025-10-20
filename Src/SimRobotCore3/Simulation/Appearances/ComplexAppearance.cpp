/**
 * @file Simulation/Appearances/ComplexAppearance.cpp
 * Implementation of class ComplexAppearance
 * @author Colin Graf
 */

#include "ComplexAppearance.h"
#include "Platform/Assert.h"
#include "Simulation/Simulation.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <unordered_map>

void ComplexAppearance::PrimitiveGroup::addParent(Element& element)
{
  ComplexAppearance* complexAppearance = dynamic_cast<ComplexAppearance*>(&element);
  complexAppearance->primitiveGroups.push_back(this);
}

void ComplexAppearance::Vertices::addParent(Element& element)
{
  ComplexAppearance* complexAppearance = dynamic_cast<ComplexAppearance*>(&element);
  ASSERT(!complexAppearance->vertices);
  complexAppearance->vertices = this;
}

void ComplexAppearance::Normals::addParent(Element& element)
{
  ComplexAppearance* complexAppearance = dynamic_cast<ComplexAppearance*>(&element);
  ASSERT(!complexAppearance->normals);
  complexAppearance->normals = this;
}

void ComplexAppearance::TexCoords::addParent(Element& element)
{
  ComplexAppearance* complexAppearance = dynamic_cast<ComplexAppearance*>(&element);
  ASSERT(!complexAppearance->texCoords);
  complexAppearance->texCoords = this;
}

ComplexAppearance::Descriptor::Descriptor(const ComplexAppearance& appearance) :
  vertices(appearance.vertices),
  normals(appearance.normals),
  texCoords(appearance.texCoords),
  primitiveGroups(&appearance.primitiveGroups)
{}

bool ComplexAppearance::Descriptor::operator==(const Descriptor& other) const
{
  return vertices == other.vertices &&
         normals == other.normals &&
         texCoords == other.texCoords &&
         std::equal(primitiveGroups->begin(), primitiveGroups->end(), other.primitiveGroups->begin(), other.primitiveGroups->end());
}

std::size_t ComplexAppearance::Hasher::operator()(const Descriptor& descriptor) const
{
  // Don't hash primitive groups. The few collisions don't really hurt initialization performance.
  return ((std::hash<ComplexAppearance::Vertices*>()(descriptor.vertices) ^ (std::hash<ComplexAppearance::Normals*>()(descriptor.normals) << 1)) >> 1) ^ (std::hash<ComplexAppearance::TexCoords*>()(descriptor.texCoords) << 1);
}

GraphicsContext::Mesh* ComplexAppearance::createMesh(GraphicsContext& graphicsContext)
{
  ASSERT(vertices);
  ASSERT(!primitiveGroups.empty());

  const Descriptor descriptor(*this);
  if(const auto cachedMesh = Simulation::simulation->complexAppearanceMeshCache.find(descriptor); cachedMesh != Simulation::simulation->complexAppearanceMeshCache.end())
    return cachedMesh->second;

  GraphicsContext::Mesh* mesh = (texCoords && surface->texture) ?
                                createMeshImpl<GraphicsContext::VertexPNT, true>(graphicsContext) :
                                createMeshImpl<GraphicsContext::VertexPN, false>(graphicsContext);

  Simulation::simulation->complexAppearanceMeshCache[descriptor] = mesh;

  return mesh;
}

template<typename VertexType, bool withTextureCoordinates>
GraphicsContext::Mesh* ComplexAppearance::createMeshImpl(GraphicsContext& graphicsContext)
{
  const std::size_t verticesSize = vertices->vertices.size();
  ASSERT(!withTextureCoordinates || texCoords->coords.size() == verticesSize);

  GraphicsContext::VertexBuffer<VertexType>* vertexBuffer = graphicsContext.requestVertexBuffer<VertexType>();
  vertexBuffer->vertices.reserve(verticesSize);

  std::unordered_map<std::uint64_t, unsigned int> indexMap;

  auto getVertex = [this, vertexBuffer, &indexMap](std::list<unsigned int>::const_iterator& iter) -> unsigned int
  {
    const unsigned int vertexIndex = *(iter++);
    const unsigned int normalIndex = normals ? *(iter++) : vertexIndex;
    if(vertexIndex >= vertices->vertices.size() || (normals && normalIndex >= normals->normals.size()))
      return 0; // Same as above: This does not make sense, but is better than crashing.
    const std::uint64_t combinedIndex = normals ? (vertexIndex | (static_cast<std::uint64_t>(normalIndex) << 32)) : static_cast<std::uint64_t>(vertexIndex);
    // Has this vertex already been added to the buffer?
    unsigned int& index = indexMap[combinedIndex];
    if(index)
      return index - 1;
    // Append a new vertex.
    const Vector3f& vertex = vertices->vertices[vertexIndex];
    const Vector3f& normal = normals ? normals->normals[normalIndex] : Vector3f::Zero(); // If no normals are defined, they will be calculated later (as the average of the normals of all faces that this vertex is part of).
    if constexpr(withTextureCoordinates)
    {
      const Vector2f& texCoord = texCoords->coords[vertexIndex];
      vertexBuffer->vertices.emplace_back(vertex, normal, texCoord);
    }
    else
      vertexBuffer->vertices.emplace_back(vertex, normal);
    // Write the new index back to the map.
    index = static_cast<unsigned int>(vertexBuffer->vertices.size());
    return index - 1;
  };

  GraphicsContext::IndexBuffer* indexBuffer = graphicsContext.requestIndexBuffer();
  auto& indices = indexBuffer->indices;
  for(const PrimitiveGroup* primitiveGroup : primitiveGroups)
  {
    ASSERT(primitiveGroup->mode == triangles || primitiveGroup->mode == quads);
    ASSERT(primitiveGroup->vertices.size() % (normals ? (primitiveGroup->mode == triangles ? 6 : 8) : (primitiveGroup->mode == triangles ? 3 : 4)) == 0);
    for(auto iter = primitiveGroup->vertices.begin(), end = primitiveGroup->vertices.end(); iter != end;)
    {
      const auto i1 = getVertex(iter);
      const auto i2 = getVertex(iter);
      const auto i3 = getVertex(iter);

      indices.push_back(i1);
      indices.push_back(i2);
      indices.push_back(i3);

      Vector3f n;
      if(!normals)
      {
        const auto& p1 = vertexBuffer->vertices[i1].position;
        const auto& p2 = vertexBuffer->vertices[i2].position;
        const auto& p3 = vertexBuffer->vertices[i3].position;

        const Vector3f u = p2 - p1;
        const Vector3f v = p3 - p1;
        n = u.cross(v).normalized();

        vertexBuffer->vertices[i1].normal += n;
        vertexBuffer->vertices[i2].normal += n;
        vertexBuffer->vertices[i3].normal += n;
      }

      if(primitiveGroup->mode == quads)
      {
        const auto i4 = getVertex(iter);
        indices.push_back(i3);
        indices.push_back(i4);
        indices.push_back(i1);
        if(!normals)
          vertexBuffer->vertices[i4].normal += n;
      }
    }
  }

  if(!normals)
    for(auto& vertex : vertexBuffer->vertices)
      vertex.normal.normalize();
  vertexBuffer->vertices.shrink_to_fit();
  vertexBuffer->finish();

  return graphicsContext.requestMesh(vertexBuffer, indexBuffer, GraphicsContext::triangleList);
}
