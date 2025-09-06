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
#include <mujoco/mujoco.h>
#include <cmath>

ServoMotor::ServoMotor()
{
  Simulation::simulation->scene->actuators.push_back(this);

  positionSensor.sensorType = SimRobotCore2::SensorPort::floatSensor;
  positionSensor.dimensions.push_back(1);
  velocitySensor.sensorType = SimRobotCore2::SensorPort::floatSensor;
  velocitySensor.dimensions.push_back(1);
}

void ServoMotor::create(Joint* joint)
{
  this->joint = joint;
  positionSensor.servoMotor = this;
  velocitySensor.servoMotor = this;

  mjsActuator* actuator = mjs_addActuator(Simulation::simulation->spec, nullptr);

  mjs_setName(actuator->element, Simulation::simulation->getName(mjOBJ_ACTUATOR, "servo", &(ctrlID)));
  actuator->gaintype = mjGAIN_FIXED;
  actuator->gainprm[0] = 1.f;
  actuator->dynprm[0] = 1.0;
  actuator->biastype = mjBIAS_NONE;
  actuator->biasprm[0] = 0;
  actuator->dyntype = mjDYN_NONE;
  actuator->trntype = mjTRN_JOINT;
  actuator->gear[0] = 1.f;

  mjs_setString(actuator->target, joint->jointname);

  actuator->ctrllimited = mjLIMITED_TRUE;
  actuator->ctrlrange[0] = -maxForce;
  actuator->ctrlrange[1] = maxForce;
}

void ServoMotor::act()
{
  const Axis::Deflection* deflection = joint->axis->deflection;
  if(!isInitialized)
  {
    ASSERT(joint->joint->type == mjJNT_HINGE || joint->joint->type == mjJNT_SLIDE);
    isInitialized = true;

    Simulation::simulation->model->dof_damping[Simulation::simulation->model->jnt_dofadr[joint->jointID]] = 0.01f;
    Simulation::simulation->model->dof_armature[Simulation::simulation->model->jnt_dofadr[joint->jointID]] = 0.01f;
    Simulation::simulation->model->dof_frictionloss[Simulation::simulation->model->jnt_dofadr[joint->jointID]] = 0.0f;
  }

  float currentPos = static_cast<float>(Simulation::simulation->data->qpos[Simulation::simulation->model->jnt_qposadr[joint->jointID]]);
  float currentVel = static_cast<float>(Simulation::simulation->data->qvel[Simulation::simulation->model->jnt_dofadr[joint->jointID]]);

  float setpoint = this->setpoint;

  float newVel = controller.getOutput(currentPos, setpoint, currentVel);
  if(newVel > maxForce)
    newVel = maxForce;
  if(newVel < -maxForce)
    newVel = -maxForce;
  Simulation::simulation->data->ctrl[ctrlID] = newVel;
}

float ServoMotor::Controller::getOutput(float currentPos, float setpoint, float vel)
{
  //const float deltaTime = Simulation::simulation->scene->stepLength;
  const float error = setpoint - currentPos;
  //errorSum += i * error * deltaTime;
  const float result = p * error - d * vel;
  lastError = error;
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
    data.floatValue = static_cast<float>(Simulation::simulation->data->qpos[Simulation::simulation->model->jnt_qposadr[servoMotor->joint->jointID]]);
  if(Simulation::simulation->model->jnt_type[servoMotor->joint->jointID] == mjJNT_HINGE)
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


void ServoMotor::VelocitySensor::updateValue()
{
    data.floatValue = static_cast<float>(Simulation::simulation->data->qvel[Simulation::simulation->model->jnt_dofadr[servoMotor->joint->jointID]]);
}

bool ServoMotor::VelocitySensor::getMinAndMax(float& min, float& max) const
{
    min = -servoMotor->maxVelocity;
    max = servoMotor->maxVelocity;
    return true;
}

void ServoMotor::registerObjects()
{
    if (Simulation::simulation->model->jnt_type[joint->jointID] == mjJNT_HINGE)
    {
        positionSensor.unit = unit = QString::fromUtf8("°");
        velocitySensor.unit = QString::fromUtf8("°/s");
    }
    else
    {
        positionSensor.unit = unit = "m";
        velocitySensor.unit = "m/s";
    }
  positionSensor.fullName = joint->fullName + ".position";
  velocitySensor.fullName = joint->fullName + ".velocity";
  fullName = joint->fullName + ".position";

  CoreModule::application->registerObject(*CoreModule::module, positionSensor, joint);
  CoreModule::application->registerObject(*CoreModule::module, velocitySensor, joint);
  CoreModule::application->registerObject(*CoreModule::module, *this, joint);
}
