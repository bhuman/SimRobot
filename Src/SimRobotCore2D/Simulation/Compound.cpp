/**
 * @file Compound.cpp
 *
 * This file implements a class for compounds (i.e. static bodies).
 *
 * @author Arne Hasselbring
 */

#include "Compound.h"
#include "Simulation/Body.h"
#include "Simulation/Geometries/Geometry.h"
#include "Simulation/Simulation.h"
#include "Tools/QtTools.h"
#include <QPainter>

void Compound::createPhysics()
{
  // Attach child geometries.
  for(::PhysicalObject* child : physicalDrawings)
  {
    auto* const geometry = dynamic_cast<Geometry*>(child);
    if(geometry)
      geometry->createGeometry(Simulation::simulation->staticBody, pose);
  }

  // Initialize children.
  ::PhysicalObject::createPhysics();

  // Create transformation.
  QtTools::convertTransformation(rotation, translation, transformation);
}

void Compound::drawPhysics(QPainter& painter) const
{
  painter.save();
  painter.setTransform(transformation, true);
  ::PhysicalObject::drawPhysics(painter);
  painter.restore();
}

const QString& Compound::getFullName() const
{
  return SimObject::getFullName();
}

const QIcon* Compound::getIcon() const
{
  return SimObject::getIcon();
}

SimRobot::Widget* Compound::createWidget()
{
  return SimObject::createWidget();
}

SimRobotCore2D::Painter* Compound::createPainter()
{
  return SimObject::createPainter();
}

SimRobotCore2D::Body* Compound::getParentBody() const
{
  return parentBody;
}
