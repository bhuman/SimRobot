/**
 * @file Simulation/Sensors/ImageSensorPort.cpp
 * Implementation of class ImageSensorPort
 * @author Colin Graf
 * @author Arne Hasselbring
 */

#include "ImageSensorPort.h"
#include "Platform/Assert.h"
#include "Simulation/Scene.h"
#include "Simulation/Simulation.h"
#include "Tools/OpenGLTools.h"
#include <algorithm>
#include <cmath>
#include <typeinfo>

void ImageSensorPort::initialize(GraphicsContext& graphicsContext, unsigned int imageWidth, unsigned int imageHeight, float angleX, float angleY, GraphicsContext::ImageFormat format)
{
  this->imageWidth = imageWidth;
  this->imageHeight = imageHeight;
  dimensions.append(imageWidth);
  dimensions.append(imageHeight);
  dimensions.append(format == GraphicsContext::yuyvImage ? 2 : 3);

  const float aspect = std::tan(angleX * 0.5f) / std::tan(angleY * 0.5f);
  OpenGLTools::computePerspective(angleY, aspect, 0.01f, 500.f, projection);

  readbackBuffer = graphicsContext.requestReadbackBuffer(format, imageWidth, imageHeight);
}

void ImageSensorPort::updateValue()
{
  render({this});
}

bool ImageSensorPort::renderCameraImages(SimRobotCore3::SensorPort** cameras, unsigned int count)
{
  if(lastSimulationStep == Simulation::simulation->simulationStep)
    return true;

  std::vector<ImageSensorPort*> ports;
  ports.reserve(count);
  for(unsigned int i = 0; i < count; ++i)
  {
    ImageSensorPort* port = dynamic_cast<ImageSensorPort*>(cameras[i]);
    if(port && port->lastSimulationStep != Simulation::simulation->simulationStep && typeid(*port) == typeid(*this) &&
       port->imageWidth == imageWidth && port->imageHeight == imageHeight && std::find(ports.begin(), ports.end(), port) == ports.end())
      ports.push_back(port);
  }
  if(!ports.empty())
    render(ports);
  return true;
}

void ImageSensorPort::render(const std::vector<ImageSensorPort*>& ports)
{
  ASSERT(!ports.empty());
  const ImageSensorPort& first = *ports.front();

  // make sure the poses of all movable objects are up to date
  Simulation::simulation->scene->updateTransformations();

  GraphicsContext& graphicsContext = Simulation::simulation->graphicsContext;
  graphicsContext.updateModelMatrices(GraphicsContext::ModelMatrix::appearance, false);
  if(first.lighting)
    graphicsContext.updateShadowMaps([&]{Simulation::simulation->scene->drawAppearances(graphicsContext);}, true, false);
  if(!graphicsContext.startOffscreenRendering(first.imageWidth, first.imageHeight * static_cast<int>(ports.size())))
    return;

  // The images are rendered on top of each other.
  int y = 0;
  for(const ImageSensorPort* port : ports)
  {
    // setup camera position
    Pose3f pose = port->physicalObject->poseInWorld;
    pose.conc(port->offset);
    static const RotationMatrix cameraRotation = (Matrix3f() << Vector3f(0.f, -1.f, 0.f), Vector3f(0.f, 0.f, 1.f), Vector3f(-1.f, 0.f, 0.f)).finished();
    pose.rotate(cameraRotation);
    Matrix4f transformation;
    OpenGLTools::convertTransformation(pose.invert(), transformation);

    graphicsContext.startRendering(port->projection, transformation, 0, y, first.imageWidth, first.imageHeight, first.lighting, first.lighting, first.lighting);
    port->drawScene(graphicsContext);
    graphicsContext.finishRendering();

    graphicsContext.readOffscreenImage(*port->readbackBuffer, y);
    y += first.imageHeight;
  }

  const std::uint64_t submission = graphicsContext.finishOffscreenRendering();
  for(ImageSensorPort* port : ports)
  {
    port->data.byteArray = port->readbackBuffer->getMemory();
    port->submission = submission;
    port->lastSimulationStep = Simulation::simulation->simulationStep;
  }
}
