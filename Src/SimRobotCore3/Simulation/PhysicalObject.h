/**
 * @file Simulation/PhysicalObject.h
 * Declaration of class PhysicalObject
 * @author Colin Graf
 */

#pragma once

#include "SimRobotCore3.h"
#include "Graphics/GraphicsContext.h"
#include "Simulation/DrawingStore.h"
#include "Simulation/SimObject.h"
#include "Tools/Math/Pose3f.h"
#include <list>

class Body;
class GraphicsContext;

/**
 * @class PhysicalObject
 * Abstract class for scene graph objects with physical representation
 */
class PhysicalObject : public SimObject
{
public:
  PhysicalObject* parent = nullptr; /**< The only parent of the primary object (or \c 0 in case that this is the root object) */
  Body* parentBody = nullptr; /**< The superior body object (might be 0) */

  Pose3f poseInWorld; /**< The absolute pose of the object */
  std::list<PhysicalObject*> physicalChildren; /**< List of subordinate physical scene graph objects */
  std::list<PhysicalObject*> physicalDrawings; /**< List of subordinate physical objects that will be drawn relative to this one */

  /**
   * Creates the physical objects used by MuJoCo.
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
   * Adds the drawings that controllers attached to this physical object (and its children) to a list
   * @param list The list of drawings of a frame
   */
  virtual void collectPhysicalDrawings(GraphicsContext::DrawingList& list) const;

  GraphicsContext::ModelMatrix* modelMatrix = nullptr; /**< The model matrix of this physical object */

protected:
  /**
   * Registers an element as parent
   * @param element The element to register
   */
  void addParent(Element& element) override;

private:
  DrawingStore drawings; /**< Drawings that another SimRobot module attached to this object */

protected:
  // API
  virtual void setDrawing(const void* key, SimRobotCore3::Drawing&& drawing);
  virtual bool removeDrawing(const void* key);
  virtual SimRobotCore3::Body* getParentBody();
};
