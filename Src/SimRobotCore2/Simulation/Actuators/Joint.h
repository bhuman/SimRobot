/**
 * @file Simulation/Actuators/Joint.h
 * Declaration of class Joint
 * @author Colin Graf
 * @author Thomas Röfer
 */

#pragma once

#include "Graphics/GraphicsContext.h"
#include "Simulation/Actuators/Actuator.h"
#include <ode/common.h>

class Axis;

/**
 * @class Joint
 * A Joint joint that connects two bodies
 */
class Joint : public Actuator
{
public:
  Axis* axis = nullptr;
  dJointID joint = nullptr;

  /** Destructor */
  ~Joint();

protected:
  /**
   * Creates the physical objects used by the OpenDynamicsEngine (ODE).
   * These are a geometry object for collision detection and/or a body,
   * if the simulation object is movable.
   * @param graphicsContext The graphics context to create resources in
   */
  void createPhysics(GraphicsContext& graphicsContext) override;

private:
  /**
   * Submits draw calls for physical primitives of the object (including children) in the given graphics context
   * @param graphicsContext The graphics context to draw the object to
   * @param flags Flags to enable or disable certain features
   */
  void drawPhysics(GraphicsContext& graphicsContext, unsigned int flags) const override;

  /** Registers this object with children, actuators and sensors at SimRobot's GUI */
  void registerObjects() override;

  GraphicsContext::Mesh* axisLine = nullptr;
  GraphicsContext::Mesh* sphere = nullptr;
  GraphicsContext::Surface* surface = nullptr;
};
