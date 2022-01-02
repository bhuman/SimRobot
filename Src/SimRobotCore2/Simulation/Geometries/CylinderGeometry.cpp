/**
 * @file Simulation/Geometries/CylinderGeometry.cpp
 * Implementation of class CylinderGeometry
 * @author Colin Graf
 */

#include "CylinderGeometry.h"
#include "Graphics/Primitives.h"
#include "Platform/Assert.h"
#include <ode/collision.h>
#include <cmath>

dGeomID CylinderGeometry::createGeometry(dSpaceID space)
{
  Geometry::createGeometry(space);
  innerRadius = radius;
  innerRadiusSqr = innerRadius * innerRadius;
  outerRadius = std::sqrt(height * height * 0.25f + radius * radius);
  return dCreateCylinder(space, radius, height);
}

void CylinderGeometry::createPhysics(GraphicsContext& graphicsContext)
{
  Geometry::createPhysics(graphicsContext);

  ASSERT(!cylinder);
  cylinder = Primitives::createCylinder(graphicsContext, radius, height, 16);
}

void CylinderGeometry::drawPhysics(GraphicsContext& graphicsContext, unsigned int flags) const
{
  if(flags & SimRobotCore2::Renderer::showPhysics)
    graphicsContext.draw(cylinder, modelMatrix, surface);

  Geometry::drawPhysics(graphicsContext, flags);
}
