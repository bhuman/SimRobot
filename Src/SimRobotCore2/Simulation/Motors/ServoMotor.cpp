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
    dJointSetHingeParam(joint->joint, dParamFudgeFactor, fudgeFactor);
    lastPos = static_cast<float>(dJointGetHingeAngle(joint->joint));
  }
  else
  {
    dJointSetSliderParam(joint->joint, dParamFMax, maxForce);
    dJointSetSliderParam(joint->joint, dParamFudgeFactor, fudgeFactor);
  }
}

void ServoMotor::act()
{
  float currentPos = dJointGetType(joint->joint) == dJointTypeHinge
                     ? static_cast<float>(dJointGetHingeAngle(joint->joint))
                     : static_cast<float>(dJointGetSliderPosition(joint->joint));

  if(dJointGetType(joint->joint) == dJointTypeHinge)
  {
    const float diff = normalize(currentPos - normalize(lastPos));
    currentPos = lastPos + diff;
    lastPos = currentPos;
  }

  float setpoint = this->setpoint - (joint->axis->deflection ? joint->axis->deflection->offset : 0.f);

  float newVel = controller.getOutput(currentPos, setpoint);

  if(std::abs(newVel - currentPos) > maxVelocity)
  {
    if(newVel < currentPos)
      newVel = currentPos - maxVelocity;
    else
      newVel = currentPos + maxVelocity;
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
