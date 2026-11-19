/**
 * @file Simulation/Sensors/ObjectSegmentedImageSensor.cpp
 * Implementation of class ObjectSegmentedImageSensor
 *
 * @author Jesse Richter-Klug
 *  includes a lot of duplicated code origin in Simulation/Sensors/Camera.cpp authored by Colin Graf
 */

#include "ObjectSegmentedImageSensor.h"
#include "CoreModule.h"
#include "Graphics/Primitives.h"
#include "Platform/Assert.h"
#include "Simulation/Body.h"
#include "Simulation/Scene.h"
#include <cmath>

static constexpr std::size_t numOfBodySurfaces = 16;
static float surfaceColors[numOfBodySurfaces][3] =
{
  {1.0f, 0.0f, 0.0f}, // red,
  {0.0f, 0.0f, 1.0f}, // blue,
  {0.0f, 0.5f, 0.0f}, // green,
  {1.0f, 1.0f, 0.0f}, // yellow,
  {0.5f, .12f, .12f}, // brown,
  {1.0f, .37f, .73f}, // pink,
  {0.5f, 0.0f, 0.5f}, // purple,
  {0.0f, 0.0f, 0.5f}, // navy,
  {.54f, .17f, .89f}, // blue violet
  {0.0f, .75f, 1.0f}, // deep sky blue,
  {0.5f, 0.5f, 0.0f}, // olive,
  {0.0f, 1.0f, 0.0f}, // lime,
  {.13f, .70f, .67f}, // light sea green,
  {.82f, .41f, .12f}, // chocolate,
  {1.0f, .65f, 0.0f}, // orange,
  {1.0f, .55f, 0.0f}  // dark orange,
};

ObjectSegmentedImageSensor::ObjectSegmentedImageSensor() :
  surfaces(Simulation::simulation->bodySurfaces)
{
  sensor.camera = this;
  sensor.sensorType = SimRobotCore3::SensorPort::rgbCameraSensor;
}

void ObjectSegmentedImageSensor::createPhysics(GraphicsContext& graphicsContext)
{
  Sensor::createPhysics(graphicsContext);

  sensor.initialize(graphicsContext, imageWidth, imageHeight, angleX, angleY, GraphicsContext::rgbImage);

  if(translation)
    sensor.offset.translation = *translation;
  if(rotation)
    sensor.offset.rotation = *rotation;

  if(surfaces.empty())
  {
    surfaces.reserve(numOfBodySurfaces);
    for(std::size_t i = 0; i < numOfBodySurfaces; ++i)
      surfaces.push_back(graphicsContext.requestSurface(surfaceColors[i]));
  }

  ASSERT(!pyramid);
  pyramid = Primitives::createPyramid(graphicsContext, std::tan(angleX * 0.5f) * 2.f, std::tan(angleY * 0.5f) * 2.f, 1.f);

  ASSERT(!surface);
  static const float color[] = {0.f, 0.f, 0.5f};
  surface = graphicsContext.requestSurface(color);
}

void ObjectSegmentedImageSensor::addParent(Element& element)
{
  sensor.physicalObject = dynamic_cast<::PhysicalObject*>(&element);
  ASSERT(sensor.physicalObject);
  Sensor::addParent(element);
}

void ObjectSegmentedImageSensor::registerObjects()
{
  sensor.fullName = fullName + ".image";
  CoreModule::application->registerObject(*CoreModule::module, sensor, this);

  Sensor::registerObjects();
}

void ObjectSegmentedImageSensor::ObjectSegmentedImageSensorPort::drawScene(GraphicsContext& graphicsContext) const
{
  // Everything is in one draw list (all sensors use the same surfaces for the bodies): the scene itself, and each body with its own surface.
  static const char sceneKey = 0;
  graphicsContext.drawList(graphicsContext.getDrawList({&sceneKey}, [&]
  {
    Simulation::simulation->scene->GraphicalObject::recordAppearances(graphicsContext);
    int j = 0;
    for(auto iter = Simulation::simulation->scene->bodies.begin(),
        end = Simulation::simulation->scene->bodies.end(); iter != end; ++iter, ++j)
    {
      graphicsContext.setForcedSurface(camera->surfaces[j % numOfBodySurfaces]);
      (*iter)->recordAppearances(graphicsContext);
    }
    graphicsContext.setForcedSurface(nullptr);
  }));
}

void ObjectSegmentedImageSensor::recordPhysics(GraphicsContext& graphicsContext, unsigned int parts) const
{
  if(parts & sensorShapes)
    graphicsContext.draw(pyramid, modelMatrix, surface);

  Sensor::recordPhysics(graphicsContext, parts);
}
