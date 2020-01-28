/**
 * @file PhysicalObject.cpp
 *
 * This file declares a base class for objects with physical properties.
 *
 * @author Arne Hasselbring
 */

#include "PhysicalObject.h"
#include "Simulation/Body.h"
#include "Platform/Assert.h"

void PhysicalObject::createPhysics()
{
  // Get the parent body (either this if this if the body or the parent body of this object).
  auto* body = dynamic_cast<Body*>(this);
  if(!body)
    body = parentBody;

  // Calculate initial pose of children, set their parent body and create their physics.
  for(PhysicalObject* child : physicalChildren)
  {
    child->pose = pose;
    if(child->translation)
      child->pose.p += b2Mul(child->pose.q, *child->translation);
    if(child->rotation)
      child->pose.q = b2Mul(child->pose.q, *child->rotation);

    child->parentBody = body;
    child->createPhysics();
  }
}

void PhysicalObject::drawPhysics(QPainter& painter) const
{
  for(const PhysicalObject* child : physicalDrawings)
    child->drawPhysics(painter);
}

void PhysicalObject::addParent(Element& element)
{
  ASSERT(!parent);
  parent = dynamic_cast<PhysicalObject*>(&element);
  ASSERT(parent);
  parent->physicalChildren.push_back(this);
  parent->physicalDrawings.push_back(this);
  SimObject::addParent(element);
}
