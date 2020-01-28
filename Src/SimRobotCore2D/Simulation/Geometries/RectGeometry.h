/**
 * @file RectGeometry.h
 *
 * This file declares an axis-aligned rectangle geometry class.
 *
 * @author Arne Hasselbring
 */

#pragma once

#include "Simulation/Geometries/Geometry.h"
#include <QColor>

class RectGeometry : public Geometry
{
public:
  float width = 0.f; /**< The width of the rectangle (i.e. length along the x-axis). */
  float height = 0.f; /**< The height of the rectangle (i.e. length along the y-axis). */
  QColor color; /**< The color in which to draw the geometry. */

protected:
  /**
   * Creates an instance of the rectangle shape.
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
