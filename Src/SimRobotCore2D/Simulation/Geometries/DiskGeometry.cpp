/**
 * @file DiskGeometry.cpp
 *
 * This file implements a disk geometry class.
 *
 * @author Arne Hasselbring
 */

#include "DiskGeometry.h"
#include <box2d/b2_circle_shape.h>
#include <QPainter>

b2Shape* DiskGeometry::createShape(const b2Transform& pose)
{
  auto* const shape = new b2CircleShape;
  shape->m_radius = radius;
  shape->m_p = pose.p;
  return shape;
}

void DiskGeometry::drawShape(QPainter& painter) const
{
  painter.setPen(Qt::NoPen);
  painter.setBrush(QBrush(color));
  painter.drawEllipse(QPointF(), radius, radius);
}
