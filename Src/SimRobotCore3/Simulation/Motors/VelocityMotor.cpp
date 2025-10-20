/**
 * @file Simulation/Motors/VelocityMotor.cpp
 * Implementation of class VelocityMotor
 * @author Colin Graf
 * @author Thomas Röfer
 */

#include "VelocityMotor.h"
#include "CoreModule.h"
#include "Platform/Assert.h"
#include "Simulation/Actuators/Joint.h"
#include "Simulation/Axis.h"
#include "Simulation/Scene.h"
#include "Simulation/Simulation.h"
#include "Tools/Math.h"
#include <mujoco/mujoco.h>
#include <cmath>

VelocityMotor::VelocityMotor()
{
  Simulation::simulation->scene->actuators.push_back(this);

  positionSensor.sensorType = SimRobotCore3::SensorPort::floatSensor;
  positionSensor.dimensions.push_back(1);

  velocitySensor.sensorType = SimRobotCore3::SensorPort::floatSensor;
  velocitySensor.dimensions.push_back(1);
}

void VelocityMotor::create(Joint* joint)
{
  this->joint = positionSensor.joint = velocitySensor.joint = joint;
  positionSensor.lastPos = joint->axis->deflection ? joint->axis->deflection->offset : 0.f;
  velocitySensor.maxVelocity = maxVelocity;

  mjsActuator* actuator = mjs_addActuator(Simulation::simulation->spec, nullptr);

  // This actually configures a P-controller for velocity.
  static const float gain = 0.2f;
  mjs_setName(actuator->element, Simulation::simulation->getName(mjOBJ_ACTUATOR, "VelocityMotor", &ctrlIndex));
  actuator->gaintype = mjGAIN_FIXED;
  actuator->gainprm[0] = gain;
  actuator->biastype = mjBIAS_AFFINE;
  actuator->biasprm[0] = 0;
  actuator->biasprm[1] = 0;
  actuator->biasprm[2] = -gain;
  actuator->dyntype = mjDYN_NONE;
  actuator->trntype = mjTRN_JOINT;
  actuator->gear[0] = 1.f;
  mjs_setString(actuator->target, joint->jointName);

  actuator->ctrllimited = mjLIMITED_TRUE;
  actuator->ctrlrange[0] = -maxVelocity;
  actuator->ctrlrange[1] = maxVelocity;

  actuator->forcelimited = mjLIMITED_TRUE;
  actuator->forcerange[0] = -maxForce;
  actuator->forcerange[1] = maxForce;
}

void VelocityMotor::act()
{
  if(Simulation::simulation->model->jnt_type[joint->jointIndex] == mjJNT_HINGE)
    positionSensor.lastPos += normalize(static_cast<float>(Simulation::simulation->data->qpos[Simulation::simulation->model->jnt_qposadr[joint->jointIndex]]) - normalize(positionSensor.lastPos));
  Simulation::simulation->data->ctrl[ctrlIndex] = setpoint;
}

void VelocityMotor::setValue(float value)
{
  if(value > maxVelocity)
    setpoint = maxVelocity;
  else if(value < -maxVelocity)
    setpoint = -maxVelocity;
  else
    setpoint = value;
}

bool VelocityMotor::getMinAndMax(float& min, float& max) const
{
  min = -maxVelocity;
  max = maxVelocity;
  return true;
}

void VelocityMotor::PositionSensor::updateValue()
{
  data.floatValue = static_cast<float>(Simulation::simulation->data->qpos[Simulation::simulation->model->jnt_qposadr[joint->jointIndex]]);
  if(Simulation::simulation->model->jnt_type[joint->jointIndex] == mjJNT_HINGE)
    data.floatValue = lastPos + normalize(data.floatValue - normalize(lastPos));
}

bool VelocityMotor::PositionSensor::getMinAndMax(float& min, float& max) const
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

void VelocityMotor::VelocitySensor::updateValue()
{
  data.floatValue = static_cast<float>(Simulation::simulation->data->qvel[Simulation::simulation->model->jnt_dofadr[joint->jointIndex]]);
}

bool VelocityMotor::VelocitySensor::getMinAndMax(float& min, float& max) const
{
  min = -maxVelocity;
  max = maxVelocity;
  return true;
}

void VelocityMotor::registerObjects()
{
  if(Simulation::simulation->model->jnt_type[joint->jointIndex] == mjJNT_HINGE)
  {
    positionSensor.unit = QString::fromUtf8("°");
    velocitySensor.unit = unit = QString::fromUtf8("°/s");
  }
  else
  {
    positionSensor.unit = "m";
    velocitySensor.unit = unit = "m/s";
  }

  positionSensor.fullName = joint->fullName + ".position";
  CoreModule::application->registerObject(*CoreModule::module, positionSensor, joint);

  velocitySensor.fullName = joint->fullName + ".velocity";
  CoreModule::application->registerObject(*CoreModule::module, velocitySensor, joint);

  fullName = joint->fullName + ".velocity";
  CoreModule::application->registerObject(*CoreModule::module, *this, joint);
}
