/**
 * @file Simulation/Geometries/CapsuleGeometry.cpp
 * Implementation of class CapsuleGeometry
 * @author Colin Graf
 */

#include "CapsuleGeometry.h"
#include "Graphics/Primitives.h"
#include "Platform/Assert.h"
#include <ode/collision.h>
#include <algorithm>

dGeomID CapsuleGeometry::createGeometry(dSpaceID space)
{
  Geometry::createGeometry(space);
  innerRadius = radius;
  innerRadiusSqr = innerRadius * innerRadius;
  outerRadius = std::max(radius, height * 0.5f);
  return dCreateCapsule(space, radius, height - radius - radius);
}

void CapsuleGeometry::createPhysics(GraphicsContext& graphicsContext)
{
  Geometry::createPhysics(graphicsContext);

  ASSERT(!capsule);
  capsule = Primitives::createCapsule(graphicsContext, radius, height, 16, 17);
}

void CapsuleGeometry::drawPhysics(GraphicsContext& graphicsContext, unsigned int flags) const
{
  if(flags & SimRobotCore2::Renderer::showPhysics)
    graphicsContext.draw(capsule, modelMatrix, surface);

  Geometry::drawPhysics(graphicsContext, flags);
}
