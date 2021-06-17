/**
 * @file ChainGeometry.h
 *
 * This file implements an edge chain geometry class.
 *
 * @author Arne Hasselbring
 */

#pragma once

#include "Simulation/Geometries/Geometry.h"
#include <box2d/b2_math.h>
#include <QColor>
#include <QPointF>
#include <vector>

class ChainGeometry : public Geometry
{
public:
  bool loop = false; /**< Whether the chain should be closed from its last to first vertex. */
  std::vector<b2Vec2> vertices; /**< The vertices of the chain. */
  QColor color; /**< The color in which to draw the geometry. */

protected:
  /**
   * Creates an instance of the edge chain shape.
   * @param pose The pose of the shape relative to the Box2D body.
   * @return A pointer to a new shape.
   */
  b2Shape* createShape(const b2Transform& pose) override;

  /**
   * Draws the shape.
   * @param painter The drawing helper (which already has the correct transformation).
   */
  void drawShape(QPainter& painter) const override;

private:
  std::vector<QPointF> qtPoints; /**< The vertices, converted to Qt points (because Qt uses doubles). */
};
