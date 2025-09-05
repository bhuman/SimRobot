/**
 * @file Simulation/Sensors/Gyroscope.cpp
 * Implementation of class Gyroscope
 * @author Colin Graf
 */

#include "Gyroscope.h"
#include "CoreModule.h"
#include "Platform/Assert.h"
#include "Simulation/Body.h"
#include "Simulation/Simulation.h"
#include <mujoco/mujoco.h>

Gyroscope::Gyroscope()
{
  sensor.sensorType = SimRobotCore2::SensorPort::floatArraySensor;
  sensor.unit = QString::fromUtf8("°/s");
  sensor.descriptions.append("x");
  sensor.descriptions.append("y");
  sensor.descriptions.append("z");
  sensor.dimensions.append(3);
  sensor.data.floatArray = sensor.angularVel;
}

void Gyroscope::createPhysics(GraphicsContext& graphicsContext)
{
  Sensor::createPhysics(graphicsContext);

  const char* sitename = Simulation::simulation->getName(mjOBJ_SITE, "site");

  mjsSite* site = mjs_addSite(sensor.body->body, nullptr);
  mjs_setName(site->element, sitename);
  if(translation)
    mju_f2n(site->pos, translation->data(), 3);
  /*
  if(rotation)
    site->quat = ...;
   */

  mjsSensor* sensor = mjs_addSensor(Simulation::simulation->spec);
  mjs_setName(sensor->element, Simulation::simulation->getName(mjOBJ_SENSOR, "gyroscope", &(this->sensor.sensorID)));
  sensor->type = mjSENS_GYRO;
  sensor->objtype = mjOBJ_SITE;
  mjs_setString(sensor->objname, sitename);
}

void Gyroscope::addParent(Element& element)
{
  sensor.body = dynamic_cast<Body*>(&element);
  ASSERT(sensor.body);
  Sensor::addParent(element);
}

void Gyroscope::registerObjects()
{
  sensor.fullName = fullName + ".angularVelocities";
  CoreModule::application->registerObject(*CoreModule::module, sensor, this);

  Sensor::registerObjects();
}

void Gyroscope::GyroscopeSensor::updateValue()
{
  ASSERT(Simulation::simulation->model->sensor_dim[sensorID] == 3);
  mju_n2f(angularVel, Simulation::simulation->data->sensordata + Simulation::simulation->model->sensor_adr[sensorID], 3);
}
