/**
 * @file EdgeGeometry.cpp
 *
 * This file implements an edge geometry class.
 *
 * @author Arne Hasselbring
 */

#include "EdgeGeometry.h"
#include <box2d/b2_edge_shape.h>
#include <QPainter>

b2Shape* EdgeGeometry::createShape(const b2Transform& pose)
{
  auto* const shape = new b2EdgeShape;
  shape->SetTwoSided(b2Mul(pose, b2Vec2(-length * 0.5f, 0.f)), b2Mul(pose, b2Vec2(length * 0.5f, 0.f)));
  return shape;
}

void EdgeGeometry::drawShape(QPainter& painter) const
{
  QPen pen(color);
  pen.setWidthF(0.01f);
  painter.setPen(pen);
  painter.drawLine(QPointF(-length * 0.5f, 0.f), QPointF(length * 0.5f, 0.f));
}
