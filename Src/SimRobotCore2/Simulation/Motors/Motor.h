/**
 * @file Simulation/Motors/Motor.h
 * Declaration of class Motor
 * @author Colin Graf
 */

#pragma once

#include "Simulation/Actuators/Actuator.h"

class Joint;

/**
 * @class Motor
 * Abstract class for joint motors
 */
class Motor : public Actuator::Port
{
public:
  float setpoint = 0.f;

  /**
   * Creates the physical representation of the motor
   * @param joint The jointed that is controlled by this motor
   */
  virtual void create(Joint* joint) = 0;

  /** Registers this object at SimRobot's GUI */
  virtual void registerObjects() = 0;

protected:
  Joint* joint = nullptr; /**< The joint controlled by this motor */
};
