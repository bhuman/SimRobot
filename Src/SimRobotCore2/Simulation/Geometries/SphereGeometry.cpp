/**
 * @file Simulation/Geometries/SphereGeometry.cpp
 * Implementation of class SphereGeometry
 * @author Colin Graf
 */

#include "SphereGeometry.h"
#include "Graphics/Primitives.h"
#include "Platform/Assert.h"
#include <ode/collision.h>

dGeomID SphereGeometry::createGeometry(dSpaceID space)
{
  Geometry::createGeometry(space);
  innerRadius = radius;
  innerRadiusSqr = innerRadius * innerRadius;
  outerRadius = radius;
  return dCreateSphere(space, radius);
}

void SphereGeometry::createPhysics(GraphicsContext& graphicsContext)
{
  Geometry::createPhysics(graphicsContext);

  ASSERT(!sphere);
  sphere = Primitives::createSphere(graphicsContext, radius, 16, 16, false);
}

void SphereGeometry::drawPhysics(GraphicsContext& graphicsContext, unsigned int flags) const
{
  if(flags & SimRobotCore2::Renderer::showPhysics)
    graphicsContext.draw(sphere, modelMatrix, surface);

  Geometry::drawPhysics(graphicsContext, flags);
}
