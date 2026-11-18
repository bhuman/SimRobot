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

void GraphicalObject::recordAppearances(GraphicsContext& graphicsContext) const
{
  for(const GraphicalObject* graphicalObject : graphicalDrawings)
    graphicalObject->recordAppearances(graphicsContext);
}

void GraphicalObject::drawAppearances(GraphicsContext& graphicsContext) const
{
  graphicsContext.drawList(graphicsContext.getDrawList({this}, [&]{recordAppearances(graphicsContext);}));
}

void GraphicalObject::addParent(Element& element)
{
  dynamic_cast<GraphicalObject*>(&element)->graphicalDrawings.push_back(this);
}
