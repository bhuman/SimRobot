/**
 * @file Simulation/Sensors/Accelerometer.cpp
 * Implementation of class Accelerometer
 * @author Colin Graf
 */

#include "Accelerometer.h"
#include "CoreModule.h"
#include "Platform/Assert.h"
#include "Simulation/Body.h"
#include "Simulation/Simulation.h"
#include <mujoco/mujoco.h>

Accelerometer::Accelerometer()
{
  sensor.sensorType = SimRobotCore2::SensorPort::floatArraySensor;
  sensor.unit = "m/s\xb2";
  sensor.descriptions.append("x");
  sensor.descriptions.append("y");
  sensor.descriptions.append("z");
  sensor.dimensions.append(3);
  sensor.data.floatArray = sensor.linearAcc;
  sensor.linearAcc[0] = sensor.linearAcc[1] = sensor.linearAcc[2] = 0.f;
}

void Accelerometer::createPhysics(GraphicsContext& graphicsContext)
{
  Sensor::createPhysics(graphicsContext);

  const char* siteName = Simulation::simulation->getName(mjOBJ_SITE, "Accelerometer");

  mjsSite* site = mjs_addSite(sensor.body->body, nullptr);
  mjs_setName(site->element, siteName);
  if(translation)
    mju_f2n(site->pos, translation->data(), 3);
  if(rotation)
  {
    mjtNum buf[9];
    mju_f2n(buf, rotation->data(), 9);
    mju_mat2Quat(site->quat, buf);
    mju_negQuat(site->quat, site->quat); // column major -> row major
  }

  mjsSensor* sensor = mjs_addSensor(Simulation::simulation->spec);
  mjs_setName(sensor->element, Simulation::simulation->getName(mjOBJ_SENSOR, "Accelerometer", &(this->sensor.sensorIndex)));
  sensor->type = mjSENS_ACCELEROMETER;
  sensor->objtype = mjOBJ_SITE;
  mjs_setString(sensor->objname, siteName);
}

void Accelerometer::addParent(Element& element)
{
  sensor.body = dynamic_cast<Body*>(&element);
  ASSERT(sensor.body);
  Sensor::addParent(element);
}

void Accelerometer::registerObjects()
{
  sensor.fullName = fullName + ".acceleration";
  CoreModule::application->registerObject(*CoreModule::module, sensor, this);

  Sensor::registerObjects();
}

void Accelerometer::AccelerometerSensor::updateValue()
{
  ASSERT(Simulation::simulation->model->sensor_dim[sensorIndex] == 3);
  mju_n2f(linearAcc, Simulation::simulation->data->sensordata + Simulation::simulation->model->sensor_adr[sensorIndex], 3);
}
