/**
 * @file Simulation/PhysicalObject.h
 * Declaration of class PhysicalObject
 * @author Colin Graf
 */

#pragma once

#include "SimRobotCore2.h"
#include "Graphics/GraphicsContext.h"
#include "Simulation/SimObject.h"
#include "Tools/Math/Pose3f.h"
#include <list>

class Body;
class GraphicsContext;
class SimObjectRenderer;

/**
 * @class PhysicalObject
 * Abstract class for scene graph objects with physical representation
 */
class PhysicalObject : public SimObject
{
public:
  PhysicalObject* parent = nullptr; /**< The only parent of the primary object (or \c 0 in case that this is the root object) */
  Body* parentBody = nullptr; /**< The superior body object (might be 0) */

  Pose3f pose; /**< The absolute pose of the object */
  std::list<PhysicalObject*> physicalChildren; /**< List of subordinate physical scene graph objects */
  std::list<PhysicalObject*> physicalDrawings; /**< List of subordinate physical objects that will be drawn relative to this one */

  /**
   * Creates the physical objects used by the OpenDynamicsEngine (ODE).
   * These are a geometry object for collision detection and/or a body,
   * if the simulation object is movable.
   * @param graphicsContext The graphics context to create resources in
   */
  virtual void createPhysics(GraphicsContext& graphicsContext);

  /**
   * Submits draw calls for physical primitives of the object (including children) in the given graphics context
   * @param graphicsContext The graphics context to draw the object to
   * @param flags Flags to enable or disable certain features
   */
  virtual void drawPhysics(GraphicsContext& graphicsContext, unsigned int flags) const;

  /**
   * Draws controller drawings of this physical object (and children)
   * @param projection Pointer to a column-major 4x4 projection matrix
   * @param view Pointer to a column-major 4x4 view matrix
   */
  void drawControllerDrawings(const float* projection, const float* view) const;

  /**
   * Registers a renderer's context for all drawings on this physical object (and children)
   * @param renderer The renderer
   */
  void registerDrawingContext(SimObjectRenderer* renderer);

  /**
   * Unregisters a renderer's context for all drawings on this physical object (and children)
   * @param renderer The renderer
   */
  void unregisterDrawingContext(SimObjectRenderer* renderer);

protected:
  /**
   * Visits controller drawings of physical children
   * @param accept The functor to apply to every child
   */
  virtual void visitPhysicalControllerDrawings(const std::function<void(PhysicalObject&)>& accept);

  /**
   * Registers an element as parent
   * @param element The element to register
   */
  void addParent(Element& element) override;

  GraphicsContext::ModelMatrix* modelMatrix = nullptr; /**< The model matrix of this physical object (if it has something to draw) */

private:
  std::list<SimRobotCore2::Controller3DDrawing*> controllerDrawings; /**< Drawings registered by another SimRobot module */
  std::list<SimObjectRenderer*> registeredRenderers; /**< Renderers that draw this physical object */

protected:
  // API
  virtual bool registerDrawing(SimRobotCore2::Controller3DDrawing& drawing);
  virtual bool unregisterDrawing(SimRobotCore2::Controller3DDrawing& drawing);
  virtual SimRobotCore2::Body* getParentBody();
};
