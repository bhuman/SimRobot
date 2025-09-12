/**
 * @file Simulation/Geometries/BoxGeometry.cpp
 * Implementation of class BoxGeometry
 * @author Colin Graf
 */

#include "BoxGeometry.h"
#include "Graphics/Primitives.h"
#include "Platform/Assert.h"
#include <mujoco/mujoco.h>
#include <algorithm>
#include <cmath>

mjsGeom* BoxGeometry::createGeometry(mjsBody* body)
{
  Geometry::createGeometry(body);
  mjsGeom* geom = mjs_addGeom(body, nullptr);
  mjs_setName(geom->element, Simulation::simulation->getName(mjOBJ_GEOM, "BoxGeometry", nullptr, this));
  geom->type = mjGEOM_BOX;
  geom->size[0] = 0.5f * depth;
  geom->size[1] = 0.5f * width;
  geom->size[2] = 0.5f * height;
  innerRadius = std::min(std::min(depth, width), height) * 0.5f;
  innerRadiusSqr = innerRadius * innerRadius;
  outerRadius = std::sqrt(depth * depth * 0.25f + width * width * 0.25f + height * height * 0.25f);
  return geom;
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
