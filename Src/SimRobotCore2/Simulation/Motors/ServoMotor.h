/**
 * @file Simulation/Motors/ServoMotor.h
 * Declaration of class ServoMotor
 * @author Colin Graf
 */

#pragma once

#include "Simulation/Motors/Motor.h"
#include "Simulation/Sensors/Sensor.h"

/**
 * @class ServoMotor
 * A motor for controlling the angle of an axis
 */
class ServoMotor : public Motor
{
public:
  /**
   * @class Controller
   * A PID controller that controls the motor
   */
  class Controller
  {
  public:
    float p = 0.f;
    float i = 0.f;
    float d = 0.f;

    /**
     * Computes the controller output. Do not call this more than once per simulation step
     * @param currentPos A measured value
     * @param setpoint The desired value
     * @return The controller output
     */
    float getOutput(float currentPos, float setpoint, float vel);

  private:
    float lastError = 0.f;
  };

  Controller controller; /**< A PID controller that controls the motor */
  float maxVelocity = 0.f;
  float stiffness = 1.f;
  float maxForce = 0.f;
  float delay = 1;
  bool isInitialized = false;

  /** Default constructor */
  ServoMotor();

private:
  /**
   * @class PositionSensor
   * A position sensor interface
   */
  class PositionSensor : public Sensor::Port
  {
  public:
    ServoMotor* servoMotor;

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
    ServoMotor* servoMotor;

    //API
    void updateValue() override;
    bool getMinAndMax(float& min, float& max) const override;
  } velocitySensor;

  /** Last position of an angular hinge. */
  float lastPos = 0;

  struct NextTargets
  {
    float setPoint = 0.f;
    float executionTimestamp = 0.f;
  };

  NextTargets* target;
  unsigned targetSize = 1;
  unsigned index = 0;
  NextTargets lastExecutedSetpoint;

  bool isPuppet = false;

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
  void setStiffness(float stiffness) override;
  void setPuppetState(bool isPuppet) override;
  bool getMinAndMax(float& min, float& max) const override;
};
