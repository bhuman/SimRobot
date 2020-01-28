/**
 * @file EdgeGeometry.h
 *
 * This file declares an edge geometry class.
 *
 * @author Arne Hasselbring
 */

#pragma once

#include "Simulation/Geometries/Geometry.h"
#include <QColor>

class EdgeGeometry : public Geometry
{
public:
  float length = 0.f; /**< The length of the edge (along the x-axis). */
  QColor color; /**< The color in which to draw the geometry. */

protected:
  /**
   * Creates an instance of the edge shape.
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
