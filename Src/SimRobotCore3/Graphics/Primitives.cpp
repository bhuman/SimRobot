/**
 * @file Graphics/Primitives.cpp
 *
 * This file implements functions to create meshes for standard geometry objects.
 *
 * @author Arne Hasselbring
 */

#include "Primitives.h"
#include "Tools/Math/Constants.h"

GraphicsContext::Mesh* Primitives::createLine(GraphicsContext& graphicsContext, const Vector3f& start, const Vector3f& end)
{
  GraphicsContext::VertexBuffer<GraphicsContext::VertexPN>* vertexBuffer = graphicsContext.requestVertexBuffer<GraphicsContext::VertexPN>();
  vertexBuffer->vertices.emplace_back(start, Vector3f(0.f, 0.f, 1.f));
  vertexBuffer->vertices.emplace_back(end, Vector3f(0.f, 0.f, 1.f));
  vertexBuffer->finish();

  return graphicsContext.requestMesh(vertexBuffer, nullptr, GraphicsContext::lineList);
}

GraphicsContext::Mesh* Primitives::createSphere(GraphicsContext& graphicsContext, float radius, unsigned int slices, unsigned int stacks, bool withTextureCoordinates)
{
  if(slices < 3 || stacks < 2 || radius < 0.f)
    return nullptr;

  GraphicsContext::VertexBuffer<GraphicsContext::VertexPN>* vertexBuffer = withTextureCoordinates ? nullptr : graphicsContext.requestVertexBuffer<GraphicsContext::VertexPN>();
  GraphicsContext::VertexBuffer<GraphicsContext::VertexPNT>* vertexBufferT = withTextureCoordinates ? graphicsContext.requestVertexBuffer<GraphicsContext::VertexPNT>() : nullptr;
  if(withTextureCoordinates)
  {
    vertexBufferT->vertices.reserve(slices + (stacks - 1) * (slices + 1) + slices);
    for(unsigned int i = 0; i < slices; ++i)
      vertexBufferT->vertices.emplace_back(Vector3f(0.f, 0.f, -radius), Vector3f(0.f, 0.f, -1.f), Vector2f(static_cast<float>(i) / slices, 0.f));
  }
  else
  {
    vertexBuffer->vertices.reserve(1 + (stacks - 1) * slices + 1);
    vertexBuffer->vertices.emplace_back(Vector3f(0.f, 0.f, -radius), Vector3f(0.f, 0.f, -1.f));
  }
  for(unsigned int i = 1; i < stacks; ++i)
  {
    const float z = -radius * std::cos(static_cast<float>(i) * pi / stacks);
    const float r = radius * std::sin(static_cast<float>(i) * pi / stacks);
    const float v = static_cast<float>(i) / stacks;
    for(unsigned int j = 0; j < slices + (withTextureCoordinates ? 1 : 0); ++j)
    {
      const float x = r * std::cos(static_cast<float>(j) * 2.f * pi / slices);
      const float y = r * std::sin(static_cast<float>(j) * 2.f * pi / slices);
      const Vector3f position(x, y, z);
      const Vector3f normal = position.normalized();
      if(withTextureCoordinates)
        vertexBufferT->vertices.emplace_back(position, normal, Vector2f(static_cast<float>(j) / slices, v));
      else
        vertexBuffer->vertices.emplace_back(position, normal);
    }
  }
  if(withTextureCoordinates)
  {
    for(unsigned int i = 0; i < slices; ++i)
      vertexBufferT->vertices.emplace_back(Vector3f(0.f, 0.f, radius), Vector3f(0.f, 0.f, 1.f), Vector2f(static_cast<float>(i) / slices, 1.f));
    vertexBufferT->finish();
  }
  else
  {
    vertexBuffer->vertices.emplace_back(Vector3f(0.f, 0.f, radius), Vector3f(0.f, 0.f, 1.f));
    vertexBuffer->finish();
  }

  GraphicsContext::IndexBuffer* indexBuffer = graphicsContext.requestIndexBuffer();
  auto& indices = indexBuffer->indices;
  if(withTextureCoordinates)
  {
    indices.reserve(3 * (slices + 2 * (stacks - 2) * slices + slices));
    for(unsigned int i = 0; i < slices; ++i)
    {
      indices.push_back(i);
      indices.push_back(i + 1 + slices);
      indices.push_back(i + slices);
    }
    for(unsigned int i = 0; i < stacks - 2; ++i)
    {
      for(unsigned int j = 0; j < slices; ++j)
      {
        indices.push_back(j + i * (slices + 1) + slices);
        indices.push_back(j + 1 + i * (slices + 1) + slices);
        indices.push_back(j + 1 + (i + 1) * (slices + 1) + slices);
        indices.push_back(j + 1 + (i + 1) * (slices + 1) + slices);
        indices.push_back(j + (i + 1) * (slices + 1) + slices);
        indices.push_back(j + i * (slices + 1) + slices);
      }
    }
    for(unsigned int i = 0; i < slices; ++i)
    {
      indices.push_back((stacks - 1) * (slices + 1) + i + slices);
      indices.push_back(i + (stacks - 2) * (slices + 1) + slices);
      indices.push_back(i + 1 + (stacks - 2) * (slices + 1) + slices);
    }
  }
  else
  {
    indices.reserve(3 * (slices + 2 * (stacks - 2) * slices + slices));
    for(unsigned int i = 0; i < slices; ++i)
    {
      indices.push_back(0);
      indices.push_back(((i + 1) % slices) + 1);
      indices.push_back(i + 1);
    }
    for(unsigned int i = 0; i < stacks - 2; ++i)
    {
      for(unsigned int j = 0; j < slices; ++j)
      {
        indices.push_back(j + i * slices + 1);
        indices.push_back(((j + 1) % slices) + i * slices + 1);
        indices.push_back(((j + 1) % slices) + (i + 1) * slices + 1);
        indices.push_back(((j + 1) % slices) + (i + 1) * slices + 1);
        indices.push_back(j + (i + 1) * slices + 1);
        indices.push_back(j + i * slices + 1);
      }
    }
    for(unsigned int i = 0; i < slices; ++i)
    {
      indices.push_back((stacks - 1) * slices + 1);
      indices.push_back(i + (stacks - 2) * slices + 1);
      indices.push_back(((i + 1) % slices) + (stacks - 2) * slices + 1);
    }
  }

  return graphicsContext.requestMesh(withTextureCoordinates ? static_cast<GraphicsContext::VertexBufferBase*>(vertexBufferT) : vertexBuffer, indexBuffer, GraphicsContext::triangleList);
}

GraphicsContext::Mesh* Primitives::createCylinder(GraphicsContext& graphicsContext, float radius, float height, unsigned int slices)
{
  if(slices < 3 || radius < 0.f || height < 0.f)
    return nullptr;

  GraphicsContext::VertexBuffer<GraphicsContext::VertexPN>* vertexBuffer = graphicsContext.requestVertexBuffer<GraphicsContext::VertexPN>();
  auto& vertices = vertexBuffer->vertices;
  vertices.reserve(4 * slices + 2);
  vertices.emplace_back(Vector3f(0.f, 0.f, -height * 0.5f), Vector3f(0.f, 0.f, -1.f));
  for(unsigned int i = 0; i < slices; ++i)
    vertices.emplace_back(Vector3f(radius * std::cos(i * 2.f * pi / slices), radius * std::sin(i * 2.f * pi / slices), -height * 0.5f), Vector3f(0.f, 0.f, -1.f));
  for(unsigned int i = 0; i < slices; ++i)
  {
    const float c = std::cos(static_cast<float>(i) * 2.f * pi / static_cast<float>(slices));
    const float s = std::sin(static_cast<float>(i) * 2.f * pi / static_cast<float>(slices));
    vertices.emplace_back(Vector3f(radius * c, radius * s, -height * 0.5f), Vector3f(c, s, 0.f));
  }
  for(unsigned int i = 0; i < slices; ++i)
  {
    const float c = std::cos(static_cast<float>(i) * 2.f * pi / static_cast<float>(slices));
    const float s = std::sin(static_cast<float>(i) * 2.f * pi / static_cast<float>(slices));
    vertices.emplace_back(Vector3f(radius * c, radius * s, height * 0.5f), Vector3f(c, s, 0.f));
  }
  for(unsigned int i = 0; i < slices; ++i)
    vertices.emplace_back(Vector3f(radius * std::cos(i * 2.f * pi / slices), radius * std::sin(i * 2.f * pi / slices), height * 0.5f), Vector3f(0.f, 0.f, 1.f));
  vertices.emplace_back(Vector3f(0.f, 0.f, height * 0.5f), Vector3f(0.f, 0.f, 1.f));
  vertexBuffer->finish();

  GraphicsContext::IndexBuffer* indexBuffer = graphicsContext.requestIndexBuffer();
  auto& indices = indexBuffer->indices;
  indices.reserve(12 * slices);
  for(unsigned int i = 0; i < slices; ++i)
  {
    indices.push_back(0);
    indices.push_back(((i + 1) % slices) + 1);
    indices.push_back(i + 1);
  }
  for(unsigned int i = 0; i < slices; ++i)
  {
    indices.push_back(i + slices + 1);
    indices.push_back(((i + 1) % slices) + slices + 1);
    indices.push_back(((i + 1) % slices) + 2 * slices + 1);
    indices.push_back(((i + 1) % slices) + 2 * slices + 1);
    indices.push_back(i + 2 * slices + 1);
    indices.push_back(i + slices + 1);
  }
  for(unsigned int i = 0; i < slices; ++i)
  {
    indices.push_back(((i + 1) % slices) + 3 * slices + 1);
    indices.push_back(4 * slices + 1);
    indices.push_back(i + 3 * slices + 1);
  }

  return graphicsContext.requestMesh(vertexBuffer, indexBuffer, GraphicsContext::triangleList);
}

GraphicsContext::Mesh* Primitives::createDisk(GraphicsContext& graphicsContext, float inner, float outer, unsigned int slices)
{
  if(slices < 3 || inner < 0.f || outer < 0.f)
    return nullptr;

  GraphicsContext::VertexBuffer<GraphicsContext::VertexPN>* vertexBuffer = graphicsContext.requestVertexBuffer<GraphicsContext::VertexPN>();
  auto& vertices = vertexBuffer->vertices;
  vertices.reserve(slices * 4);
  for(unsigned int i = 0; i < slices; ++i)
  {
    const float c = std::cos(static_cast<float>(i) * 2.f * pi / static_cast<float>(slices));
    const float s = std::sin(static_cast<float>(i) * 2.f * pi / static_cast<float>(slices));
    const float xInner = c * inner;
    const float yInner = s * inner;
    const float xOuter = c * outer;
    const float yOuter = s * outer;
    vertices.emplace_back(Vector3f(xInner, yInner, 0.f), Vector3f(0.f, 0.f, 1.f));
    vertices.emplace_back(Vector3f(xOuter, yOuter, 0.f), Vector3f(0.f, 0.f, 1.f));
    vertices.emplace_back(Vector3f(xInner, yInner, 0.f), Vector3f(0.f, 0.f, -1.f));
    vertices.emplace_back(Vector3f(xOuter, yOuter, 0.f), Vector3f(0.f, 0.f, -1.f));
  }
  vertexBuffer->finish();

  GraphicsContext::IndexBuffer* indexBuffer = graphicsContext.requestIndexBuffer();
  auto& indices = indexBuffer->indices;
  indices.reserve(slices * 12);
  for(unsigned int side = 0; side < 2; ++side)
  {
    for(unsigned int i = 0; i < slices; ++i)
    {
      indices.push_back(4 * i + side + 2 * side);
      indices.push_back(4 * i + (1 - side) + 2 * side);
      indices.push_back(4 * ((i + 1) % slices) + side + 2 * side);
      indices.push_back(4 * ((i + 1) % slices) + side + 2 * side);
      indices.push_back(4 * ((i + 1) % slices) + (1 - side) + 2 * side);
      indices.push_back(4 * i + side + 2 * side);
    }
  }

  return graphicsContext.requestMesh(vertexBuffer, indexBuffer, GraphicsContext::triangleList);
}

GraphicsContext::Mesh* Primitives::createBox(GraphicsContext& graphicsContext, float width, float height, float depth)
{
  const float w_2 = width * 0.5f;
  const float h_2 = height * 0.5f;
  const float d_2 = depth * 0.5f;

  GraphicsContext::VertexBuffer<GraphicsContext::VertexPN>* vertexBuffer = graphicsContext.requestVertexBuffer<GraphicsContext::VertexPN>();
  auto& vertices = vertexBuffer->vertices;
  vertices.reserve(24);
  // y=-w_2 face
  vertices.emplace_back(Vector3f(d_2, -w_2, -h_2), Vector3f(0.f, -1.f, 0.f));
  vertices.emplace_back(Vector3f(d_2, -w_2, h_2), Vector3f(0.f, -1.f, 0.f));
  vertices.emplace_back(Vector3f(-d_2, -w_2, h_2), Vector3f(0.f, -1.f, 0.f));
  vertices.emplace_back(Vector3f(-d_2, -w_2, -h_2), Vector3f(0.f, -1.f, 0.f));
  // y=w_2 face
  vertices.emplace_back(Vector3f(-d_2, w_2, h_2), Vector3f(0.f, 1.f, 0.f));
  vertices.emplace_back(Vector3f(d_2, w_2, h_2), Vector3f(0.f, 1.f, 0.f));
  vertices.emplace_back(Vector3f(d_2, w_2, -h_2), Vector3f(0.f, 1.f, 0.f));
  vertices.emplace_back(Vector3f(-d_2, w_2, -h_2), Vector3f(0.f, 1.f, 0.f));
  // x=-d_2 face
  vertices.emplace_back(Vector3f(-d_2, -w_2, -h_2), Vector3f(-1.f, 0.f, 0.f));
  vertices.emplace_back(Vector3f(-d_2, -w_2, h_2), Vector3f(-1.f, 0.f, 0.f));
  vertices.emplace_back(Vector3f(-d_2, w_2, h_2), Vector3f(-1.f, 0.f, 0.f));
  vertices.emplace_back(Vector3f(-d_2, w_2, -h_2), Vector3f(-1.f, 0.f, 0.f));
  // x=d_2 face
  vertices.emplace_back(Vector3f(d_2, -w_2, -h_2), Vector3f(1.f, 0.f, 0.f));
  vertices.emplace_back(Vector3f(d_2, w_2, -h_2), Vector3f(1.f, 0.f, 0.f));
  vertices.emplace_back(Vector3f(d_2, w_2, h_2), Vector3f(1.f, 0.f, 0.f));
  vertices.emplace_back(Vector3f(d_2, -w_2, h_2), Vector3f(1.f, 0.f, 0.f));
  // z=-h_2 face
  vertices.emplace_back(Vector3f(-d_2, -w_2, -h_2), Vector3f(0.f, 0.f, -1.f));
  vertices.emplace_back(Vector3f(-d_2, w_2, -h_2), Vector3f(0.f, 0.f, -1.f));
  vertices.emplace_back(Vector3f(d_2, w_2, -h_2), Vector3f(0.f, 0.f, -1.f));
  vertices.emplace_back(Vector3f(d_2, -w_2, -h_2), Vector3f(0.f, 0.f, -1.f));
  // z=h_2 face
  vertices.emplace_back(Vector3f(-d_2, -w_2, h_2), Vector3f(0.f, 0.f, 1.f));
  vertices.emplace_back(Vector3f(d_2, -w_2, h_2), Vector3f(0.f, 0.f, 1.f));
  vertices.emplace_back(Vector3f(d_2, w_2, h_2), Vector3f(0.f, 0.f, 1.f));
  vertices.emplace_back(Vector3f(-d_2, w_2, h_2), Vector3f(0.f, 0.f, 1.f));
  vertexBuffer->finish();

  GraphicsContext::IndexBuffer* indexBuffer = graphicsContext.requestIndexBuffer();
  auto& indices = indexBuffer->indices;
  indices.reserve(36);
  for(unsigned int i = 0; i < 24; i += 4)
  {
    indices.push_back(i);
    indices.push_back(i + 1);
    indices.push_back(i + 2);
    indices.push_back(i);
    indices.push_back(i + 2);
    indices.push_back(i + 3);
  }

  return graphicsContext.requestMesh(vertexBuffer, indexBuffer, GraphicsContext::triangleList);
}

GraphicsContext::Mesh* Primitives::createCapsule(GraphicsContext& graphicsContext, float radius, float height, unsigned int slices, unsigned int stacks)
{
  if(slices < 3 || stacks < 3 || !(stacks % 2))
    return nullptr;

  const float cylinderHeight = height - radius - radius;
  if(cylinderHeight < 0.f)
    return nullptr;

  GraphicsContext::VertexBuffer<GraphicsContext::VertexPN>* vertexBuffer = graphicsContext.requestVertexBuffer<GraphicsContext::VertexPN>();
  auto& vertices = vertexBuffer->vertices;
  vertices.reserve(1 + (stacks - 1) * slices + 1);

  vertices.emplace_back(Vector3f(0.f, 0.f, -height * 0.5f), Vector3f(0.f, 0.f, -1.f));
  for(unsigned int i = 1; i < stacks; ++i)
  {
    const bool lowerPart = i <= stacks / 2;
    const float z = -radius * std::cos(static_cast<float>(lowerPart ? i : i - 1) * pi / (stacks - 1));
    const float r = radius * std::sin(static_cast<float>(lowerPart ? i : i - 1) * pi / (stacks - 1));
    for(unsigned int j = 0; j < slices; ++j)
    {
      const float x = r * std::cos(static_cast<float>(j) * 2.f * pi / slices);
      const float y = r * std::sin(static_cast<float>(j) * 2.f * pi / slices);
      Vector3f position(x, y, z);
      const Vector3f normal = position.normalized();
      position.z() += (lowerPart ? -cylinderHeight : cylinderHeight) * 0.5f;
      vertices.emplace_back(position, normal);
    }
  }
  vertices.emplace_back(Vector3f(0.f, 0.f, height * 0.5f), Vector3f(0.f, 0.f, 1.f));
  vertexBuffer->finish();

  GraphicsContext::IndexBuffer* indexBuffer = graphicsContext.requestIndexBuffer();
  auto& indices = indexBuffer->indices;
  indices.reserve(3 * (slices + 2 * (stacks - 2) * slices + slices));
  for(unsigned int i = 0; i < slices; ++i)
  {
    indices.push_back(0);
    indices.push_back(((i + 1) % slices) + 1);
    indices.push_back(i + 1);
  }
  for(unsigned int i = 0; i < stacks - 2; ++i)
  {
    for(unsigned int j = 0; j < slices; ++j)
    {
      indices.push_back(j + i * slices + 1);
      indices.push_back(((j + 1) % slices) + i * slices + 1);
      indices.push_back(((j + 1) % slices) + (i + 1) * slices + 1);
      indices.push_back(((j + 1) % slices) + (i + 1) * slices + 1);
      indices.push_back(j + (i + 1) * slices + 1);
      indices.push_back(j + i * slices + 1);
    }
  }
  for(unsigned int i = 0; i < slices; ++i)
  {
    indices.push_back((stacks - 1) * slices + 1);
    indices.push_back(i + (stacks - 2) * slices + 1);
    indices.push_back(((i + 1) % slices) + (stacks - 2) * slices + 1);
  }

  return graphicsContext.requestMesh(vertexBuffer, indexBuffer, GraphicsContext::triangleList);
}

GraphicsContext::Mesh* Primitives::createPyramid(GraphicsContext& graphicsContext, float width, float height, float depth)
{
  GraphicsContext::VertexBuffer<GraphicsContext::VertexPN>* vertexBuffer = graphicsContext.requestVertexBuffer<GraphicsContext::VertexPN>();
  auto& vertices = vertexBuffer->vertices;
  vertices.reserve(5);
  vertices.emplace_back(Vector3f(0.f, 0.f, 0.f), Vector3f(0.f, 0.f, 1.f));
  vertices.emplace_back(Vector3f(depth, -width * 0.5f, -height * 0.5f), Vector3f(0.f, 0.f, 1.f));
  vertices.emplace_back(Vector3f(depth, -width * 0.5f, height * 0.5f), Vector3f(0.f, 0.f, 1.f));
  vertices.emplace_back(Vector3f(depth, width * 0.5f, height * 0.5f), Vector3f(0.f, 0.f, 1.f));
  vertices.emplace_back(Vector3f(depth, width * 0.5f, -height * 0.5f), Vector3f(0.f, 0.f, 1.f));
  vertexBuffer->finish();

  GraphicsContext::IndexBuffer* indexBuffer = graphicsContext.requestIndexBuffer();
  auto& indices = indexBuffer->indices;
  indices.reserve(16);
  for(unsigned int i = 1; i <= 4; ++i)
  {
    indices.push_back(0);
    indices.push_back(i);
  }
  for(unsigned int i = 1; i <= 4; ++i)
  {
    indices.push_back(i);
    indices.push_back((i % 4) + 1);
  }

  return graphicsContext.requestMesh(vertexBuffer, indexBuffer, GraphicsContext::lineList);
}
