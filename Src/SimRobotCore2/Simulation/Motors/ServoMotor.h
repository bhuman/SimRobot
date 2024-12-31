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
     * @param lastSetpoint The previous desired value
     * @param isNaoMotor Is the control for a NAO motor?
     * @return The controller output
     */
    float getOutput(const float currentPos, const float setpoint, const float lastSetpoint, const bool isNaoMotor);

  private:
    float errorSum = 0.f;
    float lastError = 0.f;
  };

  class ForceController
  {
  public:
    float minFeedbackForce = -1.f; /**< Minimum force to be used. */
    float maxFeedbackForce = -1.f; /**< Maximum scaling force parameter. Used in combination with the dJointFeedback to scale the actual used maximum force. */
    float maxPositionDiff = -1.f; /**< Position diff between setpoint and currentPos to use the maximum force. */
    float maxForceGrowth = -1.f; /**< Maximum force growth for the used force. Reducing the used forced is uncapped. */
    float maxForce = 0.f; /**< Maximum force. */
    float maxVelocity = 0.f; /**< Maximum allowed velocity. */
    float fudgeFactor = 0.f; /**< Fudge factor of ODE. */

    bool isActive = true; /**< Is the force controller active? Calculate once to safe computation time. */
    float currentForce = 0.f; /** Currently used force. */

    /**< Scale the actual used force between minFeedbackForce and maxForce, to simulate a more realistic servo of the NAO robot. */
    void updateForce(const float positionDiff, const dJointID joint, const dJointFeedback& feedback, const float stiffness);
  };

  Controller controller; /**< A PID controller that controls the motor */
  ForceController forceController; /**< Controller that scales the applied maximum force based on the parameterized maximum force. */
  bool isNaoMotor = false; /**< Is the servo simulating a NAO servo? */

  float bufferedSetpoint = 0.f; /**< The last requested setpoint. */
  float lastCurrentPos = 0.f; /**< The last actual position. */
  dJointFeedback feedback; /**< The force and torque feedback. */

  float lastSetpoint = 0.f; /**< The last executed setpoint. */
  float currentSetpoint = 0.f; /**< The current to be executed setpoint. */

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
   * Initializes the motor
   * @param joint The joint that is controlled by this motor
   */
  void create(Joint* joint) override;

  /** Called before computing a simulation step to update the joint */
  void act() override;

  /** Registers this object at SimRobot's GUI */
  void registerObjects() override;

  /**
   * Special handling new the position limits. ODE is not behaving as expected, therefore deactivate some parameters
   * @param currentPos The current position
   * @param newVel The planned velocity
   */
  void handleLimits(const float currentPos, const float newVel);

  /** Clip the planned velocity to ensure to not exceed the maximum velocity */
  void clipVelocity(float& velocity, const float currentPos);

  /** Clip the setpoint based on the maximum velocity */
  void clipSetpoint(float& setpoint, const float currentPos);

  // Actuator API
  void setValue(float value) override;
  void setStiffness(float value) override;
  bool getMinAndMax(float& min, float& max) const override;
};
