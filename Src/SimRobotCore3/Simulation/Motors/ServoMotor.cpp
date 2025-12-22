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

  positionSensor.sensorType = SimRobotCore3::SensorPort::floatSensor;
  positionSensor.dimensions.push_back(1);
  velocitySensor.sensorType = SimRobotCore3::SensorPort::floatSensor;
  velocitySensor.dimensions.push_back(1);
}

void ServoMotor::create(Joint* joint)
{
  this->joint = joint;
  positionSensor.servoMotor = velocitySensor.servoMotor = this;
  lastPos = joint->axis->deflection ? joint->axis->deflection->offset : 0.f;

  mjsActuator* actuator = mjs_addActuator(Simulation::simulation->spec, nullptr);

  mjs_setName(actuator->element, Simulation::simulation->getName(mjOBJ_ACTUATOR, "ServoMotor", &ctrlIndex));
  actuator->gaintype = mjGAIN_FIXED;
  actuator->gainprm[0] = 1.f;
  actuator->biastype = mjBIAS_NONE;
  actuator->dyntype = mjDYN_NONE;
  actuator->trntype = mjTRN_JOINT;
  actuator->gear[0] = 1.f;

  mjs_setString(actuator->target, joint->jointName);

  actuator->ctrllimited = mjLIMITED_TRUE;
  actuator->ctrlrange[0] = -maxForce;
  actuator->ctrlrange[1] = maxForce;

  targetSize = 1u + static_cast<unsigned>(std::ceil(delay / Simulation::simulation->scene->stepLength));
  target = new NextTargets[targetSize];
}

void ServoMotor::act()
{
  if(!isInitialized)
  {
    ASSERT(Simulation::simulation->model->jnt_type[joint->jointIndex] == mjJNT_HINGE ||
           Simulation::simulation->model->jnt_type[joint->jointIndex] == mjJNT_SLIDE);
    isInitialized = true;

    Simulation::simulation->model->dof_damping[Simulation::simulation->model->jnt_dofadr[joint->jointIndex]] = 0.01f;
    Simulation::simulation->model->dof_armature[Simulation::simulation->model->jnt_dofadr[joint->jointIndex]] = 0.01f;
    Simulation::simulation->model->dof_frictionloss[Simulation::simulation->model->jnt_dofadr[joint->jointIndex]] = 0.0f;
    for(unsigned i = 0; i < targetSize; i++)
      target[i] = { static_cast<float>(Simulation::simulation->data->qpos[Simulation::simulation->model->jnt_qposadr[joint->jointIndex]]), static_cast<float>(Simulation::simulation->simulatedTime) };
  }

  // For puppets just overwrite the values
  if(isPuppet)
  {
    mju_f2n(&Simulation::simulation->data->qpos[Simulation::simulation->model->jnt_qposadr[joint->jointIndex]], &this->setpoint, 1);
    const float zero = 0.f;
    mju_f2n(&Simulation::simulation->data->qvel[Simulation::simulation->model->jnt_dofadr[joint->jointIndex]], &zero, 1);
    return;
  }

  target[index] = { this->setpoint, static_cast<float>(Simulation::simulation->simulatedTime + delay) };

  unsigned searchIndex = index;
  while(true)
  {
    if(Simulation::simulation->simulatedTime >= target[searchIndex].executionTimestamp &&
       target[searchIndex].executionTimestamp > lastExecutedSetpoint.executionTimestamp)
      lastExecutedSetpoint = target[searchIndex];
    searchIndex++;
    if(searchIndex > targetSize)
      searchIndex = 0;
    if(searchIndex == index)
      break;
  }

  index++;
  if(index >= targetSize)
    index = 0;

  float setpoint = lastExecutedSetpoint.setPoint;
  float currentPos = static_cast<float>(Simulation::simulation->data->qpos[Simulation::simulation->model->jnt_qposadr[joint->jointIndex]]);
  const float currentVel = static_cast<float>(Simulation::simulation->data->qvel[Simulation::simulation->model->jnt_dofadr[joint->jointIndex]]);

  if(Simulation::simulation->model->jnt_type[joint->jointIndex] == mjJNT_HINGE)
  {
    const float diff = normalize(currentPos - normalize(lastPos));
    currentPos = lastPos + diff;
  }

  float newVel = controller.getOutput(currentPos, setpoint, currentVel);
  if(newVel > maxForce)
    newVel = maxForce;
  if(newVel < -maxForce)
    newVel = -maxForce;
  Simulation::simulation->data->ctrl[ctrlIndex] = newVel * stiffness;

  lastPos = currentPos;
}

float ServoMotor::Controller::getOutput(float currentPos, float setpoint, float vel)
{
  const float error = setpoint - currentPos;
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

void ServoMotor::setStiffness(float stiffness)
{
  this->stiffness = stiffness / 100.f;
  if(this->stiffness > 1.f)
    this->stiffness = 1.f;
  else if(this->stiffness < 0.05f)
    this->stiffness = 0.05f;
}

void ServoMotor::setPuppetState(bool isPuppet)
{
  this->isPuppet = isPuppet;
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
  data.floatValue = static_cast<float>(Simulation::simulation->data->qpos[Simulation::simulation->model->jnt_qposadr[servoMotor->joint->jointIndex]]);
  if(Simulation::simulation->model->jnt_type[servoMotor->joint->jointIndex] == mjJNT_HINGE)
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
  data.floatValue = static_cast<float>(Simulation::simulation->data->qvel[Simulation::simulation->model->jnt_dofadr[servoMotor->joint->jointIndex]]);
}

bool ServoMotor::VelocitySensor::getMinAndMax(float& min, float& max) const
{
  min = -servoMotor->maxVelocity;
  max = servoMotor->maxVelocity;
  return true;
}

void ServoMotor::registerObjects()
{
  if(Simulation::simulation->model->jnt_type[joint->jointIndex] == mjJNT_HINGE)
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
