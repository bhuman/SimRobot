/**
 * @file Simulation/Geometries/CapsuleGeometry.cpp
 * Implementation of class CapsuleGeometry
 * @author Colin Graf
 */

#include "CapsuleGeometry.h"
#include "Graphics/Primitives.h"
#include "Platform/Assert.h"
#include <mujoco/mujoco.h>
#include <algorithm>

mjsGeom* CapsuleGeometry::createGeometry(mjsBody* body)
{
  Geometry::createGeometry(body);
  mjsGeom* geom = mjs_addGeom(body, nullptr);
  mjs_setName(geom->element, Simulation::simulation->getName(mjOBJ_GEOM, "CapsuleGeometry", nullptr, this));
  geom->type = mjGEOM_CAPSULE;
  geom->size[0] = radius;
  geom->size[1] = 0.5f * height - radius;
  innerRadius = radius;
  innerRadiusSqr = innerRadius * innerRadius;
  outerRadius = std::max(radius, height * 0.5f);
  return geom;
}

void CapsuleGeometry::createPhysics(GraphicsContext& graphicsContext)
{
  Geometry::createPhysics(graphicsContext);

  ASSERT(!capsule);
  capsule = Primitives::createCapsule(graphicsContext, radius, height, 16, 17);
}

void CapsuleGeometry::drawPhysics(GraphicsContext& graphicsContext, unsigned int flags) const
{
  if(flags & SimRobotCore3::Renderer::showPhysics)
    graphicsContext.draw(capsule, modelMatrix, surface);

  Geometry::drawPhysics(graphicsContext, flags);
}
