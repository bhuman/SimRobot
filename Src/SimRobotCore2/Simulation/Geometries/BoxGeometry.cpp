/**
 * @file Simulation/Geometries/BoxGeometry.cpp
 * Implementation of class BoxGeometry
 * @author Colin Graf
 */

#include "BoxGeometry.h"
#include "Graphics/Primitives.h"
#include "Platform/Assert.h"
#include <ode/collision.h>
#include <algorithm>
#include <cmath>

dGeomID BoxGeometry::createGeometry(dSpaceID space)
{
  Geometry::createGeometry(space);
  innerRadius = std::min(std::min(depth, width), height) * 0.5f;
  innerRadiusSqr = innerRadius * innerRadius;
  outerRadius = std::sqrt(depth * depth * 0.25f + width * width * 0.25f + height * height * 0.25f);
  return dCreateBox(space, depth, width, height);
}

void BoxGeometry::createPhysics(GraphicsContext& graphicsContext)
{
  Geometry::createPhysics(graphicsContext);

  ASSERT(!box);
  box = Primitives::createBox(graphicsContext, width, height, depth);
}

void BoxGeometry::drawPhysics(GraphicsContext& graphicsContext, unsigned int flags) const
{
  if(flags & SimRobotCore2::Renderer::showPhysics)
    graphicsContext.draw(box, modelMatrix, surface);

  Geometry::drawPhysics(graphicsContext, flags);
}
