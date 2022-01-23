/**
 * @file Simulation/GraphicalObject.cpp
 * Implementation of class GraphicalObject
 * @author Colin Graf
 */

#include "GraphicalObject.h"
#include "SimObjectRenderer.h"

void GraphicalObject::createGraphics(GraphicsContext& graphicsContext)
{
  for(GraphicalObject* graphicalObject : graphicalDrawings)
    graphicalObject->createGraphics(graphicsContext);
}

void GraphicalObject::drawAppearances(GraphicsContext& graphicsContext, bool drawControllerDrawings) const
{
  if(drawControllerDrawings)
    for(SimRobotCore2::Controller3DDrawing* drawing : controllerDrawings)
      drawing->draw(nullptr, nullptr, nullptr); // TODO
  else
    for(const GraphicalObject* graphicalObject : graphicalDrawings)
      graphicalObject->drawAppearances(graphicsContext, false);
}

void GraphicalObject::addParent(Element& element)
{
  dynamic_cast<GraphicalObject*>(&element)->graphicalDrawings.push_back(this);
}

bool GraphicalObject::registerDrawing(SimRobotCore2::Controller3DDrawing& drawing)
{
  controllerDrawings.push_back(&drawing);
  for(SimObjectRenderer* renderer : registeredRenderers)
    renderer->addToRegisterQueue(&drawing);
  return true;
}

bool GraphicalObject::unregisterDrawing(SimRobotCore2::Controller3DDrawing& drawing)
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

void GraphicalObject::registerDrawingContext(SimObjectRenderer* renderer)
{
  registeredRenderers.push_back(renderer);
  for(auto* drawing : controllerDrawings)
    drawing->registerContext();
}

void GraphicalObject::unregisterDrawingContext(SimObjectRenderer* renderer)
{
  registeredRenderers.remove(renderer);
  for(auto* drawing : controllerDrawings)
    drawing->unregisterContext();
}
