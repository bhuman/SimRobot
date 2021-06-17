/**
 * @file ConvexGeometry.cpp
 *
 * This file implements a convex polygon geometry class.
 *
 * @author Arne Hasselbring
 */

#include "ConvexGeometry.h"
#include <box2d/b2_polygon_shape.h>
#include <QPainter>

b2Shape* ConvexGeometry::createShape(const b2Transform& pose)
{
  qtPoints.reserve(vertices.size());
  for(const b2Vec2& v : vertices)
    qtPoints.emplace_back(v.x, v.y);

  auto* const shape = new b2PolygonShape;
  for(b2Vec2& v : vertices)
    v = b2Mul(pose, v);
  shape->Set(vertices.data(), static_cast<int32>(vertices.size()));
  return shape;
}

void ConvexGeometry::drawShape(QPainter& painter) const
{
  painter.setPen(Qt::NoPen);
  painter.setBrush(QBrush(color));
  painter.drawPolygon(qtPoints.data(), static_cast<int>(qtPoints.size()));
}
