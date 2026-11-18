/**
 * @file Simulation/PhysicalObject.h
 * Implementation of class PhysicalObject
 * @author Colin Graf
 */

#include "PhysicalObject.h"
#include "Platform/Assert.h"
#include "Simulation/Body.h"

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

void PhysicalObject::recordPhysics(GraphicsContext& graphicsContext, unsigned int flags) const
{
  for(const PhysicalObject* drawing : physicalDrawings)
    drawing->recordPhysics(graphicsContext, flags);
}

void PhysicalObject::drawPhysics(GraphicsContext& graphicsContext, unsigned int flags) const
{
  // Only these flags change what is drawn.
  flags &= SimRobotCore3::Renderer::showPhysics | SimRobotCore3::Renderer::showSensors;
  if(flags)
    graphicsContext.drawList(graphicsContext.getDrawList({this, flags}, [&]{recordPhysics(graphicsContext, flags);}));
}

void PhysicalObject::collectPhysicalDrawings(GraphicsContext::DrawingList& list) const
{
  ASSERT(modelMatrix);
  drawings.collect(list, modelMatrix->getPointer());
  for(const PhysicalObject* child : physicalDrawings)
    child->collectPhysicalDrawings(list);
}

void PhysicalObject::setDrawing(const void* key, SimRobotCore3::Drawing&& drawing)
{
  drawings.set(key, std::move(drawing));
}

bool PhysicalObject::removeDrawing(const void* key)
{
  return drawings.remove(key);
}

SimRobotCore3::Body* PhysicalObject::getParentBody()
{
  return parentBody;
}
