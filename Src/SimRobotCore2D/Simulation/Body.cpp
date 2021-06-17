/**
 * @file Body.cpp
 *
 * This file implements a class for dynamic bodies.
 *
 * @author Arne Hasselbring
 */

#include "Body.h"
#include "Platform/Assert.h"
#include "Simulation/Geometries/Geometry.h"
#include "Simulation/Masses/Mass.h"
#include "Simulation/Scene.h"
#include "Simulation/Simulation.h"
#include "Tools/Math.h"
#include "Tools/QtTools.h"
#include <box2d/b2_body.h>
#include <box2d/b2_world.h>
#include <QPainter>

Body::~Body()
{
  // This also frees all associated fixtures.
  if(body)
    Simulation::simulation->world->DestroyBody(body);
}

void Body::createPhysics()
{
  ASSERT(!body);

  // Add the body to its parent (or to the scene if it does not have a parent body) and set its root body.
  if(parentBody)
  {
    parentBody->bodyChildren.push_back(this);
    rootBody = parentBody->rootBody;
  }
  else
  {
    Simulation::simulation->scene->bodies.push_back(this);
    rootBody = this;
  }

  // Create the Box2D body.
  b2BodyDef bodyDef;
  bodyDef.type = b2_dynamicBody;
  bodyDef.position = pose.p;
  bodyDef.angle = pose.q.GetAngle();
  reinterpret_cast<Body*&>(bodyDef.userData.pointer) = this;
  body = Simulation::simulation->world->CreateBody(&bodyDef);

  // Add geometries.
  b2Transform geometryPose;
  geometryPose.SetIdentity();
  for(::PhysicalObject* child : physicalDrawings)
  {
    auto* const geometry = dynamic_cast<Geometry*>(child);
    if(geometry)
      geometry->createGeometry(body, geometryPose);
  }

  // Add masses.
  b2MassData massData;
  massData.mass = 0.f;
  massData.center.SetZero();
  massData.I = 0.f;
  for(SimObject* child : children)
  {
    auto* const mass = dynamic_cast<Mass*>(child);
    if(mass)
      mass->addMassData(massData);
  }
  body->SetMassData(&massData);

  // Initialize children.
  ::PhysicalObject::createPhysics();

  // Create transformation.
  QtTools::convertTransformation(pose, transformation);
}

void Body::drawPhysics(QPainter& painter) const
{
  // Draw physical drawings with the transformation of this body.
  painter.save();
  painter.setTransform(transformation, true);
  ::PhysicalObject::drawPhysics(painter);
  painter.restore();

  // Draw body children without transformation (because they are absolute).
  for(const Body* child : bodyChildren)
    child->drawPhysics(painter);
}

void Body::updateTransformation()
{
  // Get the transformation from Box2D and traverse children.
  QtTools::convertTransformation(body->GetAngle(), body->GetPosition(), transformation);
  for(Body* child : bodyChildren)
    child->updateTransformation();
}

void Body::addParent(Element& element)
{
  // Bodies should not be physical drawings of their parents.
  ASSERT(!parent);
  parent = dynamic_cast<::PhysicalObject*>(&element);
  ASSERT(parent);
  parent->physicalChildren.push_back(this);
  SimObject::addParent(element);
}

const QString& Body::getFullName() const
{
  return SimObject::getFullName();
}

const QIcon* Body::getIcon() const
{
  return SimObject::getIcon();
}

SimRobot::Widget* Body::createWidget()
{
  return SimObject::createWidget();
}

SimRobotCore2D::Painter* Body::createPainter()
{
  return SimObject::createPainter();
}

SimRobotCore2D::Body* Body::getParentBody() const
{
  return parentBody;
}

const float* Body::getPosition() const
{
  return &body->GetPosition().x;
}

void Body::getPose(float* position, float* rotation) const
{
  if(position)
  {
    const b2Vec2& pos = body->GetPosition();
    position[0] = pos.x;
    position[1] = pos.y;
  }
  if(rotation)
    *rotation = normalize(body->GetAngle());
}

void Body::move(const float* position)
{
  body->SetTransform(b2Vec2(position[0], position[1]), body->GetAngle());
}

void Body::move(const float* position, float rotation)
{
  body->SetTransform(b2Vec2(position[0], position[1]), rotation);
}

void Body::getVelocity(float* velocity) const
{
  const b2Vec2& vel = body->GetLinearVelocity();
  velocity[0] = vel.x;
  velocity[1] = vel.y;
}

void Body::getVelocity(float* linear, float* angular) const
{
  if(linear)
  {
    const b2Vec2& vel = body->GetLinearVelocity();
    linear[0] = vel.x;
    linear[1] = vel.y;
  }
  if(angular)
    *angular = body->GetAngularVelocity();
}

void Body::setVelocity(const float* velocity)
{
  body->SetLinearVelocity(b2Vec2(velocity[0], velocity[1]));
}

void Body::setVelocity(const float* linear, float angular)
{
  body->SetLinearVelocity(b2Vec2(linear[0], linear[1]));
  body->SetAngularVelocity(angular);
}

void Body::resetDynamics()
{
  body->SetLinearVelocity(b2Vec2_zero);
  body->SetAngularVelocity(0.f);
  for(Body* child : bodyChildren)
    child->resetDynamics();
}

SimRobotCore2D::Body* Body::getRootBody() const
{
  return rootBody;
}

void Body::enablePhysics(bool enable)
{
  body->SetEnabled(enable);
  for(Body* child : bodyChildren)
    child->enablePhysics(enable);
}
