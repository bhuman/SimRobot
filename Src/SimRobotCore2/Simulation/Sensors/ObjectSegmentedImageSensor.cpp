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
#include "Tools/OpenGLTools.h"
#include <cmath>

static constexpr std::size_t numOfBodySurfaces = 16;
static float surfaceColors[numOfBodySurfaces][4] =
{
  {1.0f, 0.0f, 0.0f, 1.0f}, // red,
  {0.0f, 0.0f, 1.0f, 1.0f}, // blue,
  {0.0f, 0.5f, 0.0f, 1.0f}, // green,
  {1.0f, 1.0f, 0.0f, 1.0f}, // yellow,
  {0.5f, .12f, .12f, 1.0f}, // brown,
  {1.0f, .37f, .73f, 1.0f}, // pink,
  {0.5f, 0.0f, 0.5f, 1.0f}, // purple,
  {0.0f, 0.0f, 0.5f, 1.0f}, // navy,
  {.54f, .17f, .89f, 1.0f}, // blueviolet
  {0.0f, .75f, 1.0f, 1.0f}, // deepskyblue,
  {0.5f, 0.5f, 0.0f, 1.0f}, // olive,
  {0.0f, 1.0f, 0.0f, 1.0f}, // lime,
  {.13f, .70f, .67f, 1.0f}, // lightseagreen,
  {.82f, .41f, .12f, 1.0f}, // chocolate,
  {1.0f, .65f, 0.0f, 1.0f}, // orange,
  {1.0f, .55f, 0.0f, 1.0f}  // darkorange,
};

ObjectSegmentedImageSensor::ObjectSegmentedImageSensor() :
  surfaces(Simulation::simulation->bodySurfaces)
{
  sensor.camera = this;
  sensor.sensorType = SimRobotCore2::SensorPort::cameraSensor;
  sensor.imageBuffer = nullptr;
  sensor.imageBufferSize = 0;
}

ObjectSegmentedImageSensor::~ObjectSegmentedImageSensor()
{
  if(sensor.imageBuffer)
    delete[] sensor.imageBuffer;
}

void ObjectSegmentedImageSensor::createPhysics(GraphicsContext& graphicsContext)
{
  Sensor::createPhysics(graphicsContext);

  sensor.dimensions.append(imageWidth);
  sensor.dimensions.append(imageHeight);
  sensor.dimensions.append(3);

  if(translation)
    sensor.offset.translation = *translation;
  if(rotation)
    sensor.offset.rotation = *rotation;

  float aspect = std::tan(angleX * 0.5f) / std::tan(angleY * 0.5f);
  OpenGLTools::computePerspective(angleY, aspect, 0.01f, 500.f, sensor.projection);

  if(surfaces.empty())
  {
    surfaces.reserve(numOfBodySurfaces);
    for(std::size_t i = 0; i < numOfBodySurfaces; ++i)
      surfaces.push_back(graphicsContext.requestSurface(surfaceColors[i], surfaceColors[i]));
  }

  ASSERT(!pyramid);
  pyramid = Primitives::createPyramid(graphicsContext, std::tan(angleX * 0.5f) * 2.f, std::tan(angleY * 0.5f) * 2.f, 1.f);

  ASSERT(!surface);
  static const float color[] = {0.f, 0.f, 0.5f, 1.f};
  surface = graphicsContext.requestSurface(color, color);
}

void ObjectSegmentedImageSensor::addParent(Element& element)
{
  sensor.physicalObject = dynamic_cast< ::PhysicalObject*>(&element);
  ASSERT(sensor.physicalObject);
  Sensor::addParent(element);
}

void ObjectSegmentedImageSensor::registerObjects()
{
  sensor.fullName = fullName + ".image";
  CoreModule::application->registerObject(*CoreModule::module, sensor, this);

  Sensor::registerObjects();
}

void ObjectSegmentedImageSensor::ObjectSegmentedImageSensorPort::updateValue()
{
  // allocate buffer
  const unsigned int imageWidth = camera->imageWidth;
  const unsigned int imageHeight = camera->imageHeight;
  const unsigned int imageSize = imageWidth * imageHeight * 3;
  if(imageBufferSize < imageSize)
  {
    if(imageBuffer)
      delete[] imageBuffer;
    imageBuffer = new unsigned char[imageSize];
    imageBufferSize = imageSize;
  }

  // make sure the poses of all movable objects are up to date
  Simulation::simulation->scene->updateTransformations();

  GraphicsContext& graphicsContext = Simulation::simulation->graphicsContext;
  graphicsContext.makeCurrent(imageWidth, imageHeight);
  graphicsContext.updateModelMatrices(GraphicsContext::ModelMatrix::appearance, false);

  // setup camera position
  Pose3f pose = physicalObject->poseInWorld;
  pose.conc(offset);
  static const RotationMatrix cameraRotation = (Matrix3f() << Vector3f(0.f, -1.f, 0.f), Vector3f(0.f, 0.f, 1.f), Vector3f(-1.f, 0.f, 0.f)).finished();
  pose.rotate(cameraRotation);
  Matrix4f transformation;
  OpenGLTools::convertTransformation(pose.invert(), transformation);

  graphicsContext.startColorRendering(projection, transformation, 0, 0, imageWidth, imageHeight, true, false, false, false);

  // draw all objects
  Simulation::simulation->scene->GraphicalObject::drawAppearances(graphicsContext);
  int j = 0;
  for(auto iter = Simulation::simulation->scene->bodies.begin(),
      end = Simulation::simulation->scene->bodies.end(); iter != end; ++iter, ++j)
  {
    graphicsContext.setForcedSurface(camera->surfaces[j % numOfBodySurfaces]);
    (*iter)->drawAppearances(graphicsContext);
  }
  graphicsContext.setForcedSurface(nullptr);

  graphicsContext.finishRendering();

  // read frame buffer
  graphicsContext.finishImageRendering(imageBuffer, imageWidth, imageHeight);
  data.byteArray = imageBuffer;
}

bool ObjectSegmentedImageSensor::ObjectSegmentedImageSensorPort::renderCameraImages(SimRobotCore2::SensorPort** cameras, unsigned int count)
{
  if(lastSimulationStep == Simulation::simulation->simulationStep)
    return true;

  // allocate buffer
  const unsigned int imageWidth = camera->imageWidth;
  const unsigned int imageHeight = camera->imageHeight;
  const unsigned int imageSize = imageWidth * imageHeight * 3;
  int imagesOfCurrentSize = 0;
  for(unsigned int i = 0; i < count; ++i)
  {
    ObjectSegmentedImageSensorPort* sensor = static_cast<ObjectSegmentedImageSensorPort*>(cameras[i]);
    if(sensor && sensor->lastSimulationStep != Simulation::simulation->simulationStep &&
       sensor->camera->imageWidth == imageWidth && sensor->camera->imageHeight == imageHeight)
      ++imagesOfCurrentSize;
  }
  const unsigned int multiImageBufferSize = imageSize * imagesOfCurrentSize;

  if(imageBufferSize < multiImageBufferSize)
  {
    if(imageBuffer)
      delete[] imageBuffer;
    imageBuffer = new unsigned char[multiImageBufferSize];
    imageBufferSize = multiImageBufferSize;
  }

  // make sure the poses of all movable objects are up to date
  Simulation::simulation->scene->updateTransformations();

  GraphicsContext& graphicsContext = Simulation::simulation->graphicsContext;
  graphicsContext.makeCurrent(imageWidth, imageHeight * count);
  graphicsContext.updateModelMatrices(GraphicsContext::ModelMatrix::appearance, false);

  // render images
  int currentHorizontalPos = 0;
  unsigned char* currentBufferPos = imageBuffer;
  for(unsigned int i = 0; i < count; ++i)
  {
    ObjectSegmentedImageSensorPort* sensor = static_cast<ObjectSegmentedImageSensorPort*>(cameras[i]);
    if(sensor && sensor->lastSimulationStep != Simulation::simulation->simulationStep &&
       sensor->camera->imageWidth == imageWidth && sensor->camera->imageHeight == imageHeight)
    {
      // setup camera position
      Pose3f pose = sensor->physicalObject->poseInWorld;
      pose.conc(sensor->offset);
      static const RotationMatrix cameraRotation = (Matrix3f() << Vector3f(0.f, -1.f, 0.f), Vector3f(0.f, 0.f, 1.f), Vector3f(-1.f, 0.f, 0.f)).finished();
      pose.rotate(cameraRotation);
      Matrix4f transformation;
      OpenGLTools::convertTransformation(pose.invert(), transformation);

      graphicsContext.startColorRendering(sensor->projection, transformation, 0, currentHorizontalPos, imageWidth, imageHeight, !currentHorizontalPos, false, false, false);

      // draw all objects
      Simulation::simulation->scene->GraphicalObject::drawAppearances(graphicsContext);
      int j = 0;
      for(auto iter = Simulation::simulation->scene->bodies.begin(),
          end = Simulation::simulation->scene->bodies.end(); iter != end; ++iter, ++j)
      {
        graphicsContext.setForcedSurface(camera->surfaces[j % numOfBodySurfaces]);
        (*iter)->drawAppearances(graphicsContext);
      }
      graphicsContext.setForcedSurface(nullptr);

      graphicsContext.finishRendering();

      sensor->data.byteArray = currentBufferPos;
      sensor->lastSimulationStep = Simulation::simulation->simulationStep;

      currentHorizontalPos += imageHeight;
      currentBufferPos += imageSize;
    }
  }

  // read frame buffer
  graphicsContext.finishImageRendering(imageBuffer, imageWidth, currentHorizontalPos);
  return true;
}

void ObjectSegmentedImageSensor::drawPhysics(GraphicsContext& graphicsContext, unsigned int flags) const
{
  if(flags & SimRobotCore2::Renderer::showSensors)
    graphicsContext.draw(pyramid, modelMatrix, surface);

  Sensor::drawPhysics(graphicsContext, flags);
}
