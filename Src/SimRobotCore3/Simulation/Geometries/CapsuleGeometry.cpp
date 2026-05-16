/**
 * @file Simulation/Geometries/CapsuleGeometry.cpp
 * Implementation of class CapsuleGeometry
 * @author Colin Graf
 */

#include "CapsuleGeometry.h"
#include "Graphics/Primitives.h"
#include "Platform/Assert.h"
#include "Simulation/Simulation.h"
#include <mujoco/mujoco.h>
#include <algorithm>

mjsGeom* CapsuleGeometry::assembleGeometry(mjsBody* body)
{
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

  ASSERT(!mesh);
  mesh = Primitives::createCapsule(graphicsContext, radius, height, 16, 17);
}
