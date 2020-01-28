/**
 * @file Geometry.cpp
 *
 * This file implements a base class for geometries.
 *
 * @author Arne Hasselbring
 */

#include "Geometry.h"
#include "Platform/Assert.h"
#include "Simulation/Body.h"
#include "Tools/QtTools.h"
#include <Box2D/Collision/Shapes/b2Shape.h>
#include <Box2D/Dynamics/b2Body.h>
#include <Box2D/Dynamics/b2Fixture.h>
#include <QPainter>

void Geometry::createGeometry(b2Body* body, const b2Transform& geometryPose)
{
  // Calculate the transformation of the geometry relative to its Box2D body by adding the offset of this object.
  b2Transform pose = geometryPose;
  if(translation)
    pose.p += b2Mul(pose.q, *translation);
  if(rotation)
    pose.q = b2Mul(pose.q, *rotation);

  // Add a fixture with the shape of the derived class.
  ASSERT(!fixture);
  if(auto* const shape = createShape(pose); shape)
  {
    b2FixtureDef fixtureDef;
    fixtureDef.shape = shape;
    fixtureDef.userData = this;
    fixture = body->CreateFixture(&fixtureDef);
    delete shape;
  }

  // Add child geometries (all children must be geometries).
  for(::PhysicalObject* child : physicalDrawings)
  {
    auto* const geometry = dynamic_cast<Geometry*>(child);
    ASSERT(geometry);
    geometry->createGeometry(body, pose);
  }
}

void Geometry::createPhysics()
{
  ::PhysicalObject::createPhysics();
  QtTools::convertTransformation(rotation, translation, transformation);
}

void Geometry::drawPhysics(QPainter& painter) const
{
  painter.save();
  painter.setTransform(transformation, true);
  drawShape(painter);
  ::PhysicalObject::drawPhysics(painter);
  painter.restore();
}

const QString& Geometry::getFullName() const
{
  return SimObject::getFullName();
}

const QIcon* Geometry::getIcon() const
{
  return SimObject::getIcon();
}

SimRobot::Widget* Geometry::createWidget()
{
  return SimObject::createWidget();
}

SimRobotCore2D::Painter* Geometry::createPainter()
{
  return SimObject::createPainter();
}

SimRobotCore2D::Body* Geometry::getParentBody() const
{
  return parentBody;
}

void Geometry::registerCollisionCallback(SimRobotCore2D::CollisionCallback& callback)
{
  callbacks.push_back(&callback);
}

bool Geometry::unregisterCollisionCallback(SimRobotCore2D::CollisionCallback& callback)
{
  for(auto it = callbacks.begin(); it != callbacks.end(); ++it)
    if(*it == &callback)
    {
      callbacks.erase(it);
      return true;
    }
  return false;
}
