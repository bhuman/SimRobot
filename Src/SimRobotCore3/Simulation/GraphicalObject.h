/**
 * @file Simulation/GraphicalObject.h
 * Declaration of class GraphicalObject
 * @author Colin Graf
 */

#pragma once

#include "SimRobotCore3.h"
#include "Graphics/GraphicsContext.h"
#include "Simulation/SimObject.h"
#include <list>

class GraphicsContext;

/**
 * @class GraphicalObject
 * Abstract class for scene graph objects with graphical representation or subordinate graphical representation
 */
class GraphicalObject
{
public:
  std::list<GraphicalObject*> graphicalDrawings; /**< List of subordinate graphical scene graph objects */

  /**
   * Creates resources to later draw the object in the given graphics context
   * @param graphicsContext The graphics context to create resources in
   */
  virtual void createGraphics(GraphicsContext& graphicsContext);

  /**
   * Submits draw calls for appearance primitives of the object (including children) in the given graphics context
   * @param graphicsContext The graphics context to draw the object to
   */
  virtual void drawAppearances(GraphicsContext& graphicsContext) const;

  /**
   * Draws the same as \c drawAppearances, but through a draw list of the graphics context, which is recorded the first
   * time and then drawn with fewer draw calls. Must only be used after the scene has been created and if the appearances
   * of this object do not change.
   * @param graphicsContext The graphics context to draw the object to
   */
  void drawAppearancesCached(GraphicsContext& graphicsContext) const;

  GraphicsContext::ModelMatrix* modelMatrix = nullptr; /**< The model matrix of this graphical object (if it has something to draw) */

protected:
  /**
   * Registers an element as parent
   * @param element The element to register
   */
  virtual void addParent(Element& element);
};
