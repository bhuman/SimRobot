/**
 * @file DiskGeometry.h
 *
 * This file declares a disk geometry class.
 *
 * @author Arne Hasselbring
 */

#pragma once

#include "Simulation/Geometries/Geometry.h"
#include <QColor>

class DiskGeometry : public Geometry
{
public:
  float radius = 0.f; /**< The radius of the disk. */
  QColor color; /**< The color in which to draw the geometry. */

protected:
  /**
   * Creates an instance of the disk shape.
   * @param pose The pose of the shape relative to the Box2D body.
   * @return A pointer to a new shape.
   */
  b2Shape* createShape(const b2Transform& pose) override;

  /**
   * Draws the shape.
   * @param painter The drawing helper (which already has the correct transformation).
   */
  void drawShape(QPainter& painter) const override;
};
