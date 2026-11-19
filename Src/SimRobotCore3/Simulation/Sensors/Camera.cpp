/**
 * @file Simulation/Sensors/Camera.cpp
 * Implementation of class Camera
 * @author Colin Graf
 */

#include "Camera.h"
#include "CoreModule.h"
#include "Graphics/Primitives.h"
#include "Platform/Assert.h"
#include "Simulation/Body.h"
#include "Simulation/Scene.h"
#include <cmath>

Camera::Camera()
{
  sensor.sensorType = SimRobotCore3::SensorPort::rgbCameraSensor;
}

void Camera::createPhysics(GraphicsContext& graphicsContext)
{
  Sensor::createPhysics(graphicsContext);

  if(format == GraphicsContext::yuyvImage)
    sensor.sensorType = SimRobotCore3::SensorPort::yuyvCameraSensor;
  sensor.initialize(graphicsContext, imageWidth, imageHeight, angleX, angleY, format);

  if(translation)
    sensor.offset.translation = *translation;
  if(rotation)
    sensor.offset.rotation = *rotation;

  ASSERT(!pyramid);
  pyramid = Primitives::createPyramid(graphicsContext, std::tan(angleX * 0.5f) * 2.f, std::tan(angleY * 0.5f) * 2.f, 1.f);

  ASSERT(!surface);
  static const float color[] = {0.f, 0.f, 0.5f};
  surface = graphicsContext.requestSurface(color);
}

void Camera::addParent(Element& element)
{
  sensor.physicalObject = dynamic_cast<::PhysicalObject*>(&element);
  ASSERT(sensor.physicalObject);
  Sensor::addParent(element);
}

void Camera::registerObjects()
{
  sensor.fullName = fullName + ".image";
  CoreModule::application->registerObject(*CoreModule::module, sensor, this);

  Sensor::registerObjects();
}

void Camera::CameraSensor::drawScene(GraphicsContext& graphicsContext) const
{
  Simulation::simulation->scene->drawAppearances(graphicsContext);
}

void Camera::recordPhysics(GraphicsContext& graphicsContext, unsigned int parts) const
{
  if(parts & sensorShapes)
    graphicsContext.draw(pyramid, modelMatrix, surface);

  Sensor::recordPhysics(graphicsContext, parts);
}
