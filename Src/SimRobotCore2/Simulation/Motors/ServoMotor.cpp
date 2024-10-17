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
    dJointSetHingeParam(joint->joint, dParamFMax, maxForce);
    if(fudgeFactor != -1.f)
      dJointSetHingeParam(joint->joint, dParamFudgeFactor, fudgeFactor);
    lastPos = static_cast<float>(dJointGetHingeAngle(joint->joint));
  }
  else
  {
    dJointSetSliderParam(joint->joint, dParamFMax, maxForce);
    if(fudgeFactor != -1.f)
      dJointSetSliderParam(joint->joint, dParamFudgeFactor, fudgeFactor);
  }
  dJointSetFeedback(joint->joint, &feedback);
}

void ServoMotor::act()
{
  const float usedForce = Vector3f(feedback.f1[0], feedback.f1[1], feedback.f1[2]).norm();
  const float ratio = std::max(0.f, std::min(1.f, std::abs(usedForce) / 52.f));
  const float counterForce = ratio * maxForce + (1.f - ratio) * 0.5f + 0.5f;
  currentForce = std::min(maxForce * stiffness, std::min(counterForce, currentForce + 0.5f));
  dJointSetHingeParam(joint->joint, dParamFMax, currentForce);
  float currentPos = dJointGetType(joint->joint) == dJointTypeHinge
                     ? static_cast<float>(dJointGetHingeAngle(joint->joint))
                     : static_cast<float>(dJointGetSliderPosition(joint->joint));
  if(dJointGetType(joint->joint) == dJointTypeHinge)
  {
    const float diff = normalize(currentPos - normalize(lastPos));
    currentPos = lastPos + diff;
    lastPos = currentPos;
  }

  float setpoint = this->lastSetPoint - (joint->axis->deflection ? joint->axis->deflection->offset : 0.f);
  float newVel = controller.getOutput(currentPos, setpoint);
  if(std::abs(newVel - currentPos) > maxVelocity)
  {
    if(newVel < currentPos)
      newVel = currentPos - maxVelocity;
    else
      newVel = currentPos + maxVelocity;
  }

  if(joint->axis->deflection)
  {
    // Near the limits we want to set the cfm to 0. Otherwise the joint position will jump
    // The fudge factor should also be set to 0, but the left over jump is small enough to be ignored
    const float maxVelPerFrame = maxVelocity * Simulation::simulation->scene->stepLength;
    const float nextPos = currentPos + newVel * Simulation::simulation->scene->stepLength;
    const float maxDiff = std::min(std::abs(nextPos - joint->axis->deflection->min), std::abs(nextPos - joint->axis->deflection->max));
    // Expected position distance to the limits will be less than the velocity
    // Prevent "collisions" with the limits by setting cfm and fudge to 0
    const float ratio = maxDiff > maxVelPerFrame ? 1.f : 0.f;
    if(joint->axis->cfm != -1.f)
      dJointSetHingeParam(joint->joint, dParamCFM, ratio * joint->axis->cfm);
  }

  if(dJointGetType(joint->joint) == dJointTypeHinge)
    dJointSetHingeParam(joint->joint, dParamVel, newVel);
  else
    dJointSetSliderParam(joint->joint, dParamVel, newVel);
}

float ServoMotor::Controller::getOutput(float currentPos, float setpoint)
{
  const float deltaTime = Simulation::simulation->scene->stepLength;
  const float error = setpoint - currentPos;
  errorSum += i * error * deltaTime;
  const float requestVel = setpoint - lastSetpoint;
  const float result = p * error + errorSum + (d * requestVel) / deltaTime;
  lastError = error;
  lastSetpoint = setpoint;
  lastCurrentPos = currentPos;
  return result;
}

void ServoMotor::setValue(float value)
{
  lastSetPoint = setpoint;
  setpoint = value;
  const Axis::Deflection* deflection = joint->axis->deflection;
  if(deflection)
  {
    if(setpoint > deflection->max)
      setpoint = deflection->max;
    else if(setpoint < deflection->min)
      setpoint = deflection->min;
  }
}

void ServoMotor::setStiffness(float value)
{
  stiffness = value / 100.f;
  if(stiffness < 0.2f)
    stiffness = 0.2f;
  if(stiffness > 1.f)
    stiffness = 1.f;

  if(dJointGetType(joint->joint) == dJointTypeHinge)
    dJointSetHingeParam(joint->joint, dParamFMax, maxForce * stiffness);
  else
    dJointSetSliderParam(joint->joint, dParamFMax, maxForce * stiffness);
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
    const float diff = normalize(data.floatValue - normalize(servoMotor->lastPos));
    data.floatValue = servoMotor->lastPos + diff;
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
