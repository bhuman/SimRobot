/**
 * @file Simulation/GraphicalObject.cpp
 * Implementation of class GraphicalObject
 * @author Colin Graf
 */

#include "GraphicalObject.h"
#include "Platform/Assert.h"
#include "SimObjectRenderer.h"

void GraphicalObject::createGraphics(GraphicsContext& graphicsContext)
{
  for(GraphicalObject* graphicalObject : graphicalDrawings)
    graphicalObject->createGraphics(graphicsContext);
}

void GraphicalObject::drawAppearances(GraphicsContext& graphicsContext) const
{
  for(const GraphicalObject* graphicalObject : graphicalDrawings)
    graphicalObject->drawAppearances(graphicsContext);
}

void GraphicalObject::drawControllerDrawings() const
{
  for(SimRobotCore2::Controller3DDrawing* drawing : controllerDrawings)
    drawing->draw();
  const_cast<GraphicalObject*>(this)->visitGraphicalControllerDrawings([](GraphicalObject& child){child.drawControllerDrawings();});
}

void GraphicalObject::beforeControllerDrawings(const float* projection, const float* view) const
{
  ASSERT(modelMatrix);
  for(SimRobotCore2::Controller3DDrawing* drawing : controllerDrawings)
    drawing->beforeFrame(projection, view, modelMatrix->getPointer());
  const_cast<GraphicalObject*>(this)->visitGraphicalControllerDrawings([projection, view](GraphicalObject& child){child.beforeControllerDrawings(projection, view);});
}

void GraphicalObject::afterControllerDrawings() const
{
  for(SimRobotCore2::Controller3DDrawing* drawing : controllerDrawings)
    drawing->afterFrame();
  const_cast<GraphicalObject*>(this)->visitGraphicalControllerDrawings([](GraphicalObject& child){child.afterControllerDrawings();});
}

void GraphicalObject::visitGraphicalControllerDrawings(const std::function<void(GraphicalObject&)>&)
{
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
