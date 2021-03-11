/**
 * @file Simulation/Actuators/Joint.h
 * Declaration of class Joint
 * @author Colin Graf
 * @author Thomas Röfer
 */

#pragma once

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

private:
  /**
   * Draws physical primitives of the object (including children) on the currently selected OpenGL context
   * @param flags Flags to enable or disable certain features
   */
  void drawPhysics(unsigned int flags) const override;

  /** Registers this object with children, actuators and sensors at SimRobot's GUI */
  void registerObjects() override;
};
