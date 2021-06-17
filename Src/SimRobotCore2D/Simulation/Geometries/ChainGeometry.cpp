/**
 * @file ChainGeometry.cpp
 *
 * This file implements an edge chain geometry class.
 *
 * @author Arne Hasselbring
 */

#include "ChainGeometry.h"
#include <box2d/b2_chain_shape.h>
#include <QPainter>

b2Shape* ChainGeometry::createShape(const b2Transform& pose)
{
  qtPoints.reserve(vertices.size());
  for(const b2Vec2& v : vertices)
    qtPoints.emplace_back(v.x, v.y);

  auto* const shape = new b2ChainShape;
  for(b2Vec2& v : vertices)
    v = b2Mul(pose, v);
  if(loop)
    shape->CreateLoop(vertices.data(), static_cast<int32>(vertices.size()));
  else
    shape->CreateChain(vertices.data(), static_cast<int32>(vertices.size()), vertices.front(), vertices.back());
  return shape;
}

void ChainGeometry::drawShape(QPainter& painter) const
{
  QPen pen(color);
  pen.setWidthF(0.01f);
  painter.setPen(pen);
  if(loop)
    painter.drawPolygon(qtPoints.data(), static_cast<int>(qtPoints.size()));
  else
    painter.drawPolyline(qtPoints.data(), static_cast<int>(qtPoints.size()));
}
