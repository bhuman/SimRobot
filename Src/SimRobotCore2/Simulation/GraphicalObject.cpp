/**
 * @file Simulation/GraphicalObject.cpp
 * Implementation of class GraphicalObject
 * @author Colin Graf
 */

#include "GraphicalObject.h"

void GraphicalObject::createGraphics(GraphicsContext& graphicsContext)
{
  for(GraphicalObject* graphicalObject : graphicalDrawings)
    graphicalObject->createGraphics(graphicsContext);
}

void GraphicalObject::drawAppearances(GraphicsContext& graphicsContext, bool drawControllerDrawings) const
{
  if(drawControllerDrawings)
    for(SimRobotCore2::Controller3DDrawing* drawing : controllerDrawings)
      drawing->draw();
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
  return true;
}

bool GraphicalObject::unregisterDrawing(SimRobotCore2::Controller3DDrawing& drawing)
{
  for(auto iter = controllerDrawings.begin(), end = controllerDrawings.end(); iter != end; ++iter)
    if(*iter == &drawing)
    {
      controllerDrawings.erase(iter);
      return true;
    }
  return false;
}
