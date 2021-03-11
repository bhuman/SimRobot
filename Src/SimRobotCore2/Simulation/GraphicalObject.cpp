/**
 * @file Simulation/GraphicalObject.cpp
 * Implementation of class GraphicalObject
 * @author Colin Graf
 */


#include "GraphicalObject.h"
#include "Platform/Assert.h"
#include "Platform/OpenGL.h"
#include "Simulation/Scene.h"
#include "Simulation/Simulation.h"

GraphicalObject::~GraphicalObject()
{
  if(listId)
    glDeleteLists(listId, 1);
}

void GraphicalObject::createGraphics()
{
  ++initializedContexts;
  for(GraphicalObject* graphicalObject : graphicalDrawings)
  {
    if(graphicalObject->initializedContexts != initializedContexts)
    {
      graphicalObject->createGraphics();
      graphicalObject->initializedContexts = initializedContexts;
    }
  }

  // create display list
  unsigned int listId = glGenLists(1);
  ASSERT(listId > 0);
  ASSERT(this->listId == 0 || this->listId == listId);
  this->listId = listId;
  glNewList(listId, GL_COMPILE);
    assembleAppearances(SurfaceColor::ownColor);
  glEndList();
}

void GraphicalObject::drawAppearances(SurfaceColor color, bool drawControllerDrawings) const
{
  if(drawControllerDrawings)
    for(SimRobotCore2::Controller3DDrawing* drawing : controllerDrawings)
      drawing->draw();
  else if(color == ownColor)
  {
    ASSERT(listId);
    glCallList(listId);
  }
  else
    assembleAppearances(color);
}

void GraphicalObject::assembleAppearances(SurfaceColor color) const
{
  for(const GraphicalObject* graphicalObject : graphicalDrawings)
    graphicalObject->drawAppearances(color, false);
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
