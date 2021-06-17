/**
 * @file RectGeometry.cpp
 *
 * This file implements an axis-aligned rectangle geometry class.
 *
 * @author Arne Hasselbring
 */

#include "RectGeometry.h"
#include <box2d/b2_polygon_shape.h>
#include <QPainter>

b2Shape* RectGeometry::createShape(const b2Transform& pose)
{
  auto* const shape = new b2PolygonShape;
  b2Vec2 vertices[4];
  vertices[0] = b2Mul(pose, b2Vec2(-width * 0.5f, -height * 0.5f));
  vertices[1] = b2Mul(pose, b2Vec2(-width * 0.5f, height * 0.5f));
  vertices[2] = b2Mul(pose, b2Vec2(width * 0.5f, height * 0.5f));
  vertices[3] = b2Mul(pose, b2Vec2(width * 0.5f, -height * 0.5f));
  shape->Set(vertices, 4);
  return shape;
}

void RectGeometry::drawShape(QPainter& painter) const
{
  painter.setPen(Qt::NoPen);
  painter.setBrush(QBrush(color));
  painter.drawRect(QRectF(-width * 0.5f, -height * 0.5f, width, height));
}
