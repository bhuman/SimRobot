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
  if(flags & SimRobotCore2::Renderer::showControllerDrawings)
    for(SimRobotCore2::Controller3DDrawing* drawing : controllerDrawings)
      drawing->draw(nullptr, nullptr, nullptr); // TODO
  for(const PhysicalObject* drawing : physicalDrawings)
    drawing->drawPhysics(graphicsContext, flags);
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

void PhysicalObject::registerDrawingContext(SimObjectRenderer* renderer)
{
  registeredRenderers.push_back(renderer);
  for(auto* drawing : controllerDrawings)
    drawing->registerContext();
  for(PhysicalObject* drawing : physicalDrawings)
    drawing->registerDrawingContext(renderer);
}

void PhysicalObject::unregisterDrawingContext(SimObjectRenderer* renderer)
{
  registeredRenderers.remove(renderer);
  for(auto* drawing : controllerDrawings)
    drawing->unregisterContext();
  for(PhysicalObject* drawing : physicalDrawings)
    drawing->unregisterDrawingContext(renderer);
}

SimRobotCore2::Body* PhysicalObject::getParentBody()
{
  return parentBody;
}
