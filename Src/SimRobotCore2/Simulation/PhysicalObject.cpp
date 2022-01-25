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
    object->pose = pose;
    if(object->translation)
      object->pose.translate(*object->translation);
    if(object->rotation)
      object->pose.rotate(*object->rotation);

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
  if(modelMatrix)
    for(SimRobotCore2::Controller3DDrawing* drawing : controllerDrawings)
      drawing->draw();
  const_cast<PhysicalObject*>(this)->visitPhysicalControllerDrawings([](PhysicalObject& child){child.drawControllerDrawings();});
}

void PhysicalObject::beforeControllerDrawings(const float* projection, const float* view) const
{
  if(modelMatrix)
    for(SimRobotCore2::Controller3DDrawing* drawing : controllerDrawings)
      drawing->beforeFrame(projection, view, modelMatrix->getPointer());
  const_cast<PhysicalObject*>(this)->visitPhysicalControllerDrawings([projection, view](PhysicalObject& child){child.beforeControllerDrawings(projection, view);});
}

void PhysicalObject::afterControllerDrawings() const
{
  if(modelMatrix)
    for(SimRobotCore2::Controller3DDrawing* drawing : controllerDrawings)
      drawing->afterFrame();
  const_cast<PhysicalObject*>(this)->visitPhysicalControllerDrawings([](PhysicalObject& child){child.afterControllerDrawings();});
}

void PhysicalObject::registerDrawingContext(SimObjectRenderer* renderer)
{
  registeredRenderers.push_back(renderer);
  for(auto* drawing : controllerDrawings)
    drawing->registerContext();
  visitPhysicalControllerDrawings([renderer](PhysicalObject& child){child.registerDrawingContext(renderer);});
}

void PhysicalObject::unregisterDrawingContext(SimObjectRenderer* renderer)
{
  registeredRenderers.remove(renderer);
  for(auto* drawing : controllerDrawings)
    drawing->unregisterContext();
  visitPhysicalControllerDrawings([renderer](PhysicalObject& child){child.unregisterDrawingContext(renderer);});
}

void PhysicalObject::visitPhysicalControllerDrawings(const std::function<void(PhysicalObject&)>& accept)
{
  for(PhysicalObject* drawing : physicalDrawings)
    accept(*drawing);
}

bool PhysicalObject::registerDrawing(SimRobotCore2::Controller3DDrawing& drawing)
{
  controllerDrawings.push_back(&drawing);
  for(SimObjectRenderer* renderer : registeredRenderers)
    renderer->addToRegisterQueue(&drawing);
  return true;
}

bool PhysicalObject::unregisterDrawing(SimRobotCore2::Controller3DDrawing& drawing)
{
  for(auto iter = controllerDrawings.begin(), end = controllerDrawings.end(); iter != end; ++iter)
    if(*iter == &drawing)
    {
      // It is impossible to unregister contexts here.
      ASSERT(registeredRenderers.empty());
      // The above assertion also guarantees that the drawing is not referenced in any register queue of a renderer.
      controllerDrawings.erase(iter);
      return true;
    }
  return false;
}

SimRobotCore2::Body* PhysicalObject::getParentBody()
{
  return parentBody;
}
