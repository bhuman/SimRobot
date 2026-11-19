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
  /** The parts of a physical object that can be drawn (they can be combined). */
  enum PhysicsParts : unsigned int
  {
    physicalPrimitives = 1 << 0, /**< The shapes of the physical primitives (geometries, joints, ...). */
    sensorShapes = 1 << 1, /**< The shapes that visualize sensors (the field of view of a camera, the ray of a distance sensor, ...). */
  };

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
   * Records draw calls for the physical primitives of the object (including children) in the draw list that the graphics
   * context is currently recording (see \c GraphicsContextBase::getDrawList).
   * @param graphicsContext The graphics context to record the draw calls in
   * @param parts The parts of the object to record (see \c PhysicsParts)
   */
  virtual void recordPhysics(GraphicsContext& graphicsContext, unsigned int parts) const;

  /**
   * Draws the physical primitives of the object (including children) through a draw list of the graphics context, which
   * is recorded by \c recordPhysics the first time (for each combination of parts) and then drawn with fewer draw calls.
   * Must only be used after the scene has been created.
   * @param graphicsContext The graphics context to draw the object to
   * @param parts The parts of the object to record (see \c PhysicsParts)
   */
  void drawPhysics(GraphicsContext& graphicsContext, unsigned int parts) const;

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
