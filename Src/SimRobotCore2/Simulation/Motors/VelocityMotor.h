/**
 * @file Simulation/Motors/VelocityMotor.h
 * Declaration of class VelocityMotor
 * @author Colin Graf
 * @author Thomas Röfer
 */

#pragma once

#include "Simulation/Motors/Motor.h"
#include "Simulation/Sensors/Sensor.h"

/**
 * @class VelocityMotor
 * A motor for controlling the rotational speed of an axis
 */
class VelocityMotor : public Motor
{
public:
  float maxVelocity = 0.f;
  float maxForce = 0.f;

  /** Default constructor */
  VelocityMotor();

private:
  /**
   * @class PositionSensor
   * A position sensor interface
   */
  class PositionSensor : public Sensor::Port
  {
  public:
    Joint* joint;
    float lastPos;

    //API
    void updateValue() override;
    bool getMinAndMax(float& min, float& max) const override;
  } positionSensor;

  /**
   * @class VelocitySensor
   * A velocity sensor interface
   */
  class VelocitySensor : public Sensor::Port
  {
  public:
    Joint* joint;
    float maxVelocity;

    //API
    void updateValue() override;
    bool getMinAndMax(float& min, float& max) const override;
  } velocitySensor;

  /**
   * Initializes the motor
   * @param joint The joint that is controlled by this motor
   */
  void create(Joint* joint) override;

  /** Called before computing a simulation step to update the joint */
  void act() override;

  /** Registers this object at SimRobot's GUI */
  void registerObjects() override;

  // actuator API
  void setValue(float value) override;
  void setStiffness(float) override {};
  bool getMinAndMax(float& min, float& max) const override;
};
