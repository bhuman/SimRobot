/**
 * @file Simulation/Geometries/SphereGeometry.cpp
 * Implementation of class SphereGeometry
 * @author Colin Graf
 */

#include "SphereGeometry.h"
#include "Graphics/Primitives.h"
#include "Platform/Assert.h"
#include <mujoco/mujoco.h>

mjsGeom* SphereGeometry::createGeometry(mjsBody* body)
{
  Geometry::createGeometry(body);
  mjsGeom* geom = mjs_addGeom(body, nullptr);
  mjs_setName(geom->element, Simulation::simulation->getName(mjOBJ_GEOM, "SphereGeometry", nullptr, this));
  geom->type = mjGEOM_SPHERE;
  geom->size[0] = radius;
  innerRadius = radius;
  innerRadiusSqr = innerRadius * innerRadius;
  outerRadius = radius;
  return geom;
}

void SphereGeometry::createPhysics(GraphicsContext& graphicsContext)
{
  Geometry::createPhysics(graphicsContext);

  ASSERT(!sphere);
  sphere = Primitives::createSphere(graphicsContext, radius, 16, 16, false);
}

void SphereGeometry::drawPhysics(GraphicsContext& graphicsContext, unsigned int flags) const
{
  if(flags & SimRobotCore3::Renderer::showPhysics)
    graphicsContext.draw(sphere, modelMatrix, surface);

  Geometry::drawPhysics(graphicsContext, flags);
}
