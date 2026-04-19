/**
 * @file Simulation/Actuators/Joint.cpp
 * Implementation of class Joint
 * @author Tim Laue
 * @author Kai Spiess
 * @author Colin Graf
 * @author Thomas Röfer
 */

#include "Joint.h"
#include "Graphics/Primitives.h"
#include "Platform/Assert.h"
#include "SimRobotCore3.h"
#include "Simulation/Axis.h"
#include "Simulation/Motors/Motor.h"
#include <cmath>

void Joint::createPhysics(GraphicsContext& graphicsContext)
{
  Actuator::createPhysics(graphicsContext);

  ASSERT(!axisLine);
  axisLine = Primitives::createLine(graphicsContext, -0.05f * Vector3f(axis->x, axis->y, axis->z), 0.05f * Vector3f(axis->x, axis->y, axis->z));

  ASSERT(!sphere);
  sphere = Primitives::createSphere(graphicsContext, 0.002f, 10, 10, false);

  ASSERT(!surface);
  const float color[] = {std::abs(axis->x), std::abs(axis->y), std::abs(axis->z), 1.f};
  surface = graphicsContext.requestSurface(color, color);
}

void Joint::drawPhysics(GraphicsContext& graphicsContext, unsigned int flags) const
{
  if(flags & SimRobotCore3::Renderer::showPhysics)
  {
    graphicsContext.draw(axisLine, modelMatrix, surface);
    graphicsContext.draw(sphere, modelMatrix, surface);
  }

  Actuator::drawPhysics(graphicsContext, flags);
}

void Joint::registerObjects()
{
  // add sensors and actuators
  if(axis->motor)
    axis->motor->registerObjects();

  // add children
  ::PhysicalObject::registerObjects();
}
