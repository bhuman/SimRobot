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
    float getOutput(float currentPos, float setpoint);

  private:
    float errorSum = 0.f;
    float lastError = 0.f;
    float lastSetpoint = 0.f;
    float lastRequestVel = 0.f;
    float lastCurrentPos = 0.f;
  };

  Controller controller; /**< A PID controller that controls the motor */
  float maxVelocity = 0.f;
  float maxForce = 0.f;
  float fudgeFactor = 0.f;
  float lastSetPoint = 0.f;
  float lastCurrentpoint = 0.f;
  dJointFeedback feedback;
  float currentForce = 0.f;

  /** Default constructor */
  ServoMotor();

private:
  /**
   * @class PositionSensor
   * A angle sensor interface
   */
  class PositionSensor : public Sensor::Port
  {
  public:
    ServoMotor* servoMotor;

    //API
    void updateValue() override;
    bool getMinAndMax(float& min, float& max) const override;
  } positionSensor;

  /** Last position of an angular hinge. */
  float lastPos;

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
  void setStiffness(float value) override;
  bool getMinAndMax(float& min, float& max) const override;
};
