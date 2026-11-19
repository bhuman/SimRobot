/**
 * @file Simulation/Actuators/Joint.h
 * Declaration of class Joint
 * @author Colin Graf
 * @author Thomas Röfer
 */

#pragma once

#include "Graphics/GraphicsContext.h"
#include "Simulation/Actuators/Actuator.h"

class Axis;

/**
 * @class Joint
 * A Joint joint that connects two bodies
 */
class Joint : public Actuator
{
public:
  Axis* axis = nullptr;
  const char* jointName = nullptr;
  int jointIndex = -1;

protected:
  /**
   * Creates the physical objects used by MuJoCo.
   * These are a geometry object for collision detection and/or a body,
   * if the simulation object is movable.
   * @param graphicsContext The graphics context to create resources in
   */
  void createPhysics(GraphicsContext& graphicsContext) override;

private:
  /**
   * Records draw calls for the physical primitives of the object (including children) in the draw list that the graphics
   * context is currently recording (see \c GraphicsContextBase::getDrawList).
   * @param graphicsContext The graphics context to record the draw calls in
   * @param parts The parts of the object to record (see \c PhysicsParts)
   */
  void recordPhysics(GraphicsContext& graphicsContext, unsigned int parts) const override;

  /** Registers this object with children, actuators and sensors at SimRobot's GUI */
  void registerObjects() override;

  GraphicsContext::Mesh* axisLine = nullptr;
  GraphicsContext::Mesh* sphere = nullptr;
  GraphicsContext::Surface* surface = nullptr;
};
