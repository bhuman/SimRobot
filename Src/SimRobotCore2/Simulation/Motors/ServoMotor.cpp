/**
 * @file Simulation/Motors/ServoMotor.cpp
 * Implementation of class ServoMotor
 * @author Colin Graf
 */

#include "ServoMotor.h"
#include "CoreModule.h"
#include "Platform/Assert.h"
#include "Simulation/Actuators/Joint.h"
#include "Simulation/Axis.h"
#include "Simulation/Scene.h"
#include "Simulation/Simulation.h"
#include "Tools/Math.h"
#include <ode/objects.h>
#include <cmath>

ServoMotor::ServoMotor()
{
  Simulation::simulation->scene->actuators.push_back(this);

  positionSensor.sensorType = SimRobotCore2::SensorPort::floatSensor;
  positionSensor.dimensions.push_back(1);
}

void ServoMotor::create(Joint* joint)
{
  ASSERT(dJointGetType(joint->joint) == dJointTypeHinge || dJointGetType(joint->joint) == dJointTypeSlider);
  this->joint = joint;
  positionSensor.servoMotor = this;
  if(dJointGetType(joint->joint) == dJointTypeHinge)
  {
    dJointSetHingeParam(joint->joint, dParamFMax, forceController.maxForce);
    if(forceController.fudgeFactor != -1.f)
      dJointSetHingeParam(joint->joint, dParamFudgeFactor, forceController.fudgeFactor);
    lastCurrentPos = static_cast<float>(dJointGetHingeAngle(joint->joint));
  }
  else
  {
    dJointSetSliderParam(joint->joint, dParamFMax, forceController.maxForce);
    if(forceController.fudgeFactor != -1.f)
      dJointSetSliderParam(joint->joint, dParamFudgeFactor, forceController.fudgeFactor);
  }
  dJointSetFeedback(joint->joint, &feedback);
  forceController.isActive = forceController.minFeedbackForce != -1.f && forceController.maxFeedbackForce != -1.f && forceController.maxPositionDiff != -1.f && forceController.maxForceGrowth != -1.f && forceController.maxForce > 0;
}

void ServoMotor::act()
{
  float currentPos = dJointGetType(joint->joint) == dJointTypeHinge
                     ? static_cast<float>(dJointGetHingeAngle(joint->joint))
                     : static_cast<float>(dJointGetSliderPosition(joint->joint));
  float setpoint = this->currentSetpoint - (joint->axis->deflection ? joint->axis->deflection->offset : 0.f);

  if(dJointGetType(joint->joint) == dJointTypeHinge)
  {
    const float diff = normalize(currentPos - normalize(lastCurrentPos));
    currentPos = lastCurrentPos + diff;
  }

  if(!isNaoMotor)
    clipSetpoint(setpoint, currentPos);
  float newVel = controller.getOutput(currentPos, setpoint, lastSetpoint, isNaoMotor);
  if(isNaoMotor)
    clipVelocity(newVel, currentPos);
  handleLimits(currentPos, newVel);

  forceController.updateForce(currentPos - setpoint, joint->joint, feedback, stiffness);

  if(dJointGetType(joint->joint) == dJointTypeHinge)
    dJointSetHingeParam(joint->joint, dParamVel, newVel);
  else
    dJointSetSliderParam(joint->joint, dParamVel, newVel);

  lastCurrentPos = currentPos;
}

float ServoMotor::Controller::getOutput(const float currentPos, const float setpoint, const float lastSetpoint, const bool isNaoMotor)
{
  const float deltaTime = Simulation::simulation->scene->stepLength;
  const float error = setpoint - currentPos;
  errorSum += i * error * deltaTime;
  const float result = p * error + errorSum + d * (isNaoMotor ? setpoint - lastSetpoint : error - lastError) / deltaTime;
  lastError = error;
  return result;
}

void ServoMotor::setValue(float value)
{
  lastSetpoint = currentSetpoint;
  currentSetpoint = bufferedSetpoint;
  bufferedSetpoint = value;
  const Axis::Deflection* deflection = joint->axis->deflection;
  if(deflection)
  {
    if(bufferedSetpoint > deflection->max)
      bufferedSetpoint = deflection->max;
    else if(bufferedSetpoint < deflection->min)
      bufferedSetpoint = deflection->min;
  }
  // lastSetpoint does not matter for non nao motors, as those do not have the extra one frame delay
  if(!isNaoMotor)
    currentSetpoint = bufferedSetpoint;
}

void ServoMotor::setStiffness(float value)
{
  stiffness = value / 100.f;
  if(stiffness < 0.2f)
    stiffness = 0.2f;
  if(stiffness > 1.f)
    stiffness = 1.f;

  if(dJointGetType(joint->joint) == dJointTypeHinge)
    dJointSetHingeParam(joint->joint, dParamFMax, forceController.maxForce * stiffness);
  else
    dJointSetSliderParam(joint->joint, dParamFMax, forceController.maxForce * stiffness);
}

bool ServoMotor::getMinAndMax(float& min, float& max) const
{
  const Axis::Deflection* deflection = joint->axis->deflection;
  if(deflection)
  {
    min = deflection->min;
    max = deflection->max;
    return true;
  }
  return false;
}

void ServoMotor::PositionSensor::updateValue()
{
  data.floatValue = (dJointGetType(servoMotor->joint->joint) == dJointTypeHinge
                     ? static_cast<float>(dJointGetHingeAngle(servoMotor->joint->joint))
                     : static_cast<float>(dJointGetSliderPosition(servoMotor->joint->joint))) + (servoMotor->joint->axis->deflection ? servoMotor->joint->axis->deflection->offset : 0.f);
  if(dJointGetType(servoMotor->joint->joint) == dJointTypeHinge)
  {
    const float diff = normalize(data.floatValue - normalize(servoMotor->lastCurrentPos));
    data.floatValue = servoMotor->lastCurrentPos + diff;
  }
}

bool ServoMotor::PositionSensor::getMinAndMax(float& min, float& max) const
{
  const Axis::Deflection* deflection = servoMotor->joint->axis->deflection;
  if(deflection)
  {
    min = deflection->min;
    max = deflection->max;
    return true;
  }
  return false;
}

void ServoMotor::registerObjects()
{
  if(dJointGetType(joint->joint) == dJointTypeHinge)
    positionSensor.unit = unit = QString::fromUtf8("°");
  else
    positionSensor.unit = unit = "m";
  positionSensor.fullName = joint->fullName + ".position";
  fullName = joint->fullName + ".position";

  CoreModule::application->registerObject(*CoreModule::module, positionSensor, joint);
  CoreModule::application->registerObject(*CoreModule::module, *this, joint);
}

void ServoMotor::handleLimits(const float currentPos, const float newVel)
{
  if(joint->axis->deflection && joint->axis->cfm != -1.f)
  {
    // Near the limits we want to set the cfm to 0. Otherwise the joint position will jump
    // The fudge factor should also be set to 0, but the left over jump is small enough to be ignored
    const float maxVelPerFrame = forceController.maxVelocity * Simulation::simulation->scene->stepLength;
    const float nextPos = currentPos + newVel * Simulation::simulation->scene->stepLength;
    const float maxDiff = std::min(std::abs(nextPos - joint->axis->deflection->min), std::abs(nextPos - joint->axis->deflection->max));
    // Expected position distance to the limits will be less than the velocity
    // Prevent "collisions" with the limits by setting cfm and fudge to 0
    const float ratio = maxDiff > maxVelPerFrame ? 1.f : 0.f;
    dJointSetHingeParam(joint->joint, dParamCFM, ratio * joint->axis->cfm);
  }
}

void ServoMotor::clipVelocity(float& velocity, const float currentPos)
{
  if(std::abs(velocity - currentPos) > forceController.maxVelocity)
  {
    if(velocity < currentPos)
      velocity = currentPos - forceController.maxVelocity;
    else
      velocity = currentPos + forceController.maxVelocity;
  }
}

void ServoMotor::clipSetpoint(float& setpoint, const float currentPos)
{
  const float maxPositionChange = Simulation::simulation->scene->stepLength * forceController.maxVelocity;
  if(std::abs(setpoint - currentPos) > maxPositionChange)
  {
    if(setpoint < currentPos)
      setpoint = currentPos - maxPositionChange;
    else
      setpoint = currentPos + maxPositionChange;
  }
}

void ServoMotor::ForceController::updateForce(const float positionDiff, const dJointID joint, const dJointFeedback& feedback, const float stiffness)
{
  if(!isActive)
    return;

  // Force needed to counter act outside forces
  const float usedForce = Vector3f(static_cast<float>(feedback.f1[0]), static_cast<float>(feedback.f1[1]), static_cast<float>(feedback.f1[2])).norm();
  const float outsideRatio = std::max(0.f, std::min(1.f, std::abs(usedForce) / maxFeedbackForce)); // 68
  const float outsideForce = outsideRatio * maxForce + (1.f - outsideRatio) * minFeedbackForce;

  // Force needed to handle the current position difference
  const float positionRatio = std::min(1.f, std::abs(positionDiff) / maxPositionDiff); // 0.087
  const float positionForce = positionRatio * maxForce + (1.f - positionRatio) * minFeedbackForce;

  // Determine used force
  const float maxNeededForce = std::max({ outsideForce, positionForce });
  currentForce = std::min(maxForce * stiffness, std::min(maxNeededForce, currentForce + maxForceGrowth));

  dJointSetHingeParam(joint, dParamFMax, currentForce);
}
