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
   * Records draw calls for the appearance primitives of the object (including children) in the draw list that the graphics
   * context is currently recording (see \c GraphicsContextBase::getDrawList).
   * @param graphicsContext The graphics context to record the draw calls in
   */
  virtual void recordAppearances(GraphicsContext& graphicsContext) const;

  /**
   * Draws the appearances of the object (including children) through a draw list of the graphics context, which is
   * recorded by \c recordAppearances the first time and then drawn with fewer draw calls. Must only be used after the
   * scene has been created and if the appearances of this object do not change.
   * @param graphicsContext The graphics context to draw the object to
   */
  void drawAppearances(GraphicsContext& graphicsContext) const;

  GraphicsContext::ModelMatrix* modelMatrix = nullptr; /**< The model matrix of this graphical object (if it has something to draw) */

protected:
  /**
   * Registers an element as parent
   * @param element The element to register
   */
  virtual void addParent(Element& element);
};
