/**
 * @file Simulation/PhysicalObject.h
 * Implementation of class PhysicalObject
 * @author Colin Graf
 */

#include "PhysicalObject.h"
#include "Platform/Assert.h"
#include "Simulation/Body.h"
#include "SimObjectRenderer.h"

void PhysicalObject::addParent(Element& element)
{
  ASSERT(!parent);
  parent = dynamic_cast<PhysicalObject*>(&element);
  ASSERT(parent);
  parent->physicalChildren.push_back(this);
  parent->physicalDrawings.push_back(this);
  SimObject::addParent(element);
}

void PhysicalObject::createPhysics(GraphicsContext& graphicsContext)
{
  // find parent body for child objects
  Body* body = dynamic_cast<Body*>(this);
  if(!body)
    body = parentBody;

  // initialize and call createPhysics() for each child object
  for(PhysicalObject* object : physicalChildren)
  {
    // compute pose of child object
    object->poseInWorld = poseInWorld;
    if(object->translation)
      object->poseInWorld.translate(*object->translation);
    if(object->rotation)
      object->poseInWorld.rotate(*object->rotation);

    //
    object->parentBody = body;
    object->createPhysics(graphicsContext);
  }
}

void PhysicalObject::drawPhysics(GraphicsContext& graphicsContext, unsigned int flags) const
{
  for(const PhysicalObject* drawing : physicalDrawings)
    drawing->drawPhysics(graphicsContext, flags);
}

void PhysicalObject::drawControllerDrawings() const
{
  for(SimRobotCore2::Controller3DDrawing* drawing : controllerDrawings)
    drawing->draw();
  const_cast<PhysicalObject*>(this)->visitPhysicalControllerDrawings([](PhysicalObject& child){child.drawControllerDrawings();});
}

void PhysicalObject::beforeControllerDrawings(const float* projection, const float* view) const
{
  ASSERT(modelMatrix);
  for(SimRobotCore2::Controller3DDrawing* drawing : controllerDrawings)
    drawing->beforeFrame(projection, view, modelMatrix->getPointer());
  const_cast<PhysicalObject*>(this)->visitPhysicalControllerDrawings([projection, view](PhysicalObject& child){child.beforeControllerDrawings(projection, view);});
}

void PhysicalObject::afterControllerDrawings() const
{
  for(SimRobotCore2::Controller3DDrawing* drawing : controllerDrawings)
    drawing->afterFrame();
  const_cast<PhysicalObject*>(this)->visitPhysicalControllerDrawings([](PhysicalObject& child){child.afterControllerDrawings();});
}

void PhysicalObject::visitPhysicalControllerDrawings(const std::function<void(PhysicalObject&)>& accept)
{
  for(PhysicalObject* drawing : physicalDrawings)
    accept(*drawing);
}

bool PhysicalObject::registerDrawing(SimRobotCore2::Controller3DDrawing& drawing)
{
  controllerDrawings.push_back(&drawing);
  return true;
}

bool PhysicalObject::unregisterDrawing(SimRobotCore2::Controller3DDrawing& drawing)
{
  for(auto iter = controllerDrawings.begin(), end = controllerDrawings.end(); iter != end; ++iter)
    if(*iter == &drawing)
    {
      controllerDrawings.erase(iter);
      return true;
    }
  return false;
}

SimRobotCore2::Body* PhysicalObject::getParentBody()
{
  return parentBody;
}
