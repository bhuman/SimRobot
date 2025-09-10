/**
 * @file Simulation/Sensors/SingleDistanceSensor.cpp
 * Implementation of class SingleDistanceSensor
 * @author Colin Graf
 */

#include "SingleDistanceSensor.h"
#include "CoreModule.h"
#include "Graphics/Primitives.h"
#include "Platform/Assert.h"
#include <mujoco/mujoco.h>
#include <cmath>

SingleDistanceSensor::SingleDistanceSensor()
{
  sensor.sensorType = SimRobotCore2::SensorPort::floatSensor;
  sensor.unit = "m";
}

void SingleDistanceSensor::createPhysics(GraphicsContext& graphicsContext)
{
  Sensor::createPhysics(graphicsContext);

  sensor.min = min;
  sensor.max = max;
  if(translation)
    sensor.offset.translation = *translation;
  if(rotation)
    sensor.offset.rotation = *rotation;

  ASSERT(!ray);
  ray = Primitives::createLine(graphicsContext, Vector3f::Zero(), Vector3f(max, 0.f, 0.f));

  ASSERT(!surface);
  static const float color[] = {0.5f, 0.f, 0.f, 1.f};
  surface = graphicsContext.requestSurface(color, color);
}

void SingleDistanceSensor::registerObjects()
{
  sensor.fullName = fullName + ".distance";
  CoreModule::application->registerObject(*CoreModule::module, sensor, this);

  Sensor::registerObjects();
}

void SingleDistanceSensor::addParent(Element& element)
{
  sensor.physicalObject = dynamic_cast<::PhysicalObject*>(&element);
  ASSERT(sensor.physicalObject);
  Sensor::addParent(element);
}

void SingleDistanceSensor::DistanceSensor::updateValue()
{
  pose = physicalObject->poseInWorld;
  pose.conc(offset);

  mjtNum origin[3], dir[3];
  mju_f2n(origin, pose.translation.data(), 3);
  mju_f2n(dir, pose.rotation.col(0).data(), 3);

  const float dist = static_cast<float>(mj_ray(Simulation::simulation->model, Simulation::simulation->data, origin, dir, nullptr, 1, -1, nullptr));
  if(dist < 0.f)
    data.floatValue = max;
  else if(dist < min)
    data.floatValue = min;
  else if(dist > max)
    data.floatValue = max;
  else
    data.floatValue = dist;
}

bool SingleDistanceSensor::DistanceSensor::getMinAndMax(float& min, float& max) const
{
  min = this->min;
  max = this->max;
  return true;
}

void SingleDistanceSensor::drawPhysics(GraphicsContext& graphicsContext, unsigned int flags) const
{
  if(flags & SimRobotCore2::Renderer::showSensors)
    graphicsContext.draw(ray, modelMatrix, surface);

  Sensor::drawPhysics(graphicsContext, flags);
}
