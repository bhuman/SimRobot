/**
 * @file Mass.cpp
 *
 * This file implements a base class for masses.
 *
 * @author Arne Hasselbring
 */

#include "Mass.h"
#include "Platform/Assert.h"

void Mass::addMassData(b2MassData& massData)
{
  // Ensure that this mass exists.
  createMass();

  // Calculate combined center of mass, mass value and inertia.
  massData.center = (1.f / (massData.mass + mass.mass)) * (massData.mass * massData.center + mass.mass * mass.center);
  massData.mass += mass.mass;
  massData.I += mass.I;
}

void Mass::setMass()
{
  mass.mass = 0.f;
  mass.center.SetZero();
  mass.I = 0.f;
}

void Mass::createMass()
{
  // Masses can be used multiple times in the scene graph, so this one might have already been created.
  if(created)
    return;
  created = true;

  // Initialize mass with own mass, then add child masses (which are then already transformed into this' coordinate system).
  setMass();
  for(SimObject* child : children)
  {
    auto* const childMass = dynamic_cast<Mass*>(child);
    ASSERT(childMass);
    childMass->addMassData(mass);
  }

  // Transform to parent coordinate system.
  if(translation || rotation)
  {
    mass.I -= mass.mass * mass.center.LengthSquared();
    if(rotation)
      mass.center = b2Mul(*rotation, mass.center);
    if(translation)
      mass.center += *translation;
    mass.I += mass.mass * mass.center.LengthSquared();
  }
}

const QString& Mass::getFullName() const
{
  return SimObject::getFullName();
}

const QIcon* Mass::getIcon() const
{
  return SimObject::getIcon();
}

SimRobot::Widget* Mass::createWidget()
{
  return SimObject::createWidget();
}

SimRobotCore2D::Painter* Mass::createPainter()
{
  return SimObject::createPainter();
}
