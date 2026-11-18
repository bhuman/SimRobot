/**
 * @file Simulation/Sensors/DepthImageSensor.cpp
 * Implementation of class DepthImageSensor
 * @author Colin Graf
 */

#include "DepthImageSensor.h"
#include "CoreModule.h"
#include "Platform/Assert.h"
#include "Simulation/Body.h"
#include "Simulation/Scene.h"
#include "Tools/Math/Constants.h"
#include "Tools/OpenGLTools.h"
#include <algorithm>
#include <cmath>

DepthImageSensor::DepthImageSensor()
{
  sensor.depthImageSensor = this;
  sensor.sensorType = SimRobotCore3::SensorPort::floatArraySensor;
  sensor.unit = "m";
  sensor.imageBuffer = nullptr;
  sensor.lut = nullptr;
}

DepthImageSensor::~DepthImageSensor()
{
  if(sensor.imageBuffer)
    delete[] sensor.imageBuffer;
  if(sensor.lut)
    delete[] sensor.lut;
}

void DepthImageSensor::createPhysics(GraphicsContext& graphicsContext)
{
  Sensor::createPhysics(graphicsContext);

  sensor.renderHeight = imageHeight;

  if(projection == sphericalProjection)
  {
    ASSERT(imageHeight == 1);

    sensor.imageBuffer = new float[imageWidth * imageHeight];
    sensor.numOfBuffers = static_cast<unsigned int>(std::ceil(angleX / (pi * 2.0f / 3.0f)));
    sensor.bufferWidth = static_cast<unsigned int>(std::ceil(static_cast<float>(imageWidth) / static_cast<float>(sensor.numOfBuffers)));
    sensor.lut = new unsigned int[sensor.bufferWidth];
    sensor.renderAngleX = angleX * sensor.bufferWidth / imageWidth;

    //Compute new resolution of rendering buffer
    float maxAngle(sensor.renderAngleX / 2.0f);
    float minPixelWidth(std::tan(maxAngle/(float(sensor.bufferWidth) / 2.0f)));
    float totalWidth(std::tan(maxAngle));
    float newXRes(totalWidth / minPixelWidth);
    sensor.renderWidth = static_cast<unsigned int>(ceil(newXRes)) * 2;

    //Compute values for LUT (sensor data -> rendering buffer)
    float firstAngle(-maxAngle);
    float step(maxAngle / (static_cast<float>(sensor.bufferWidth) / 2.0f));
    float currentAngle(firstAngle);
    float gToPixelFactor(newXRes / std::tan(maxAngle));
    for(unsigned int i = 0; i < sensor.bufferWidth; ++i)
    {
      float g(std::tan(currentAngle));
      g *= gToPixelFactor;
      int gPixel(static_cast<int>(g) + static_cast<int>(sensor.renderWidth) / 2);
      sensor.lut[i] = static_cast<unsigned int>(gPixel);
      currentAngle += step;
    }
  }
  else
  {
    sensor.numOfBuffers = 1;
    sensor.bufferWidth = imageWidth;
    sensor.renderWidth = imageWidth;
    sensor.renderAngleX = angleX;
  }
  sensor.dimensions.append(imageWidth);
  if(imageHeight > 1)
    sensor.dimensions.append(imageHeight);

  if(translation)
    sensor.offset.translation = *translation;
  if(rotation)
    sensor.offset.rotation = *rotation;

  sensor.min = min;
  sensor.max = max;

  const float zNear = std::max(min, 0.001f); // at least 1mm since zNear must not be zero
  float aspect = std::tan(sensor.renderAngleX * 0.5f) / std::tan(angleY * 0.5f);
  OpenGLTools::computePerspective(angleY, aspect, zNear, max, sensor.projection);

  for(unsigned int i = 0; i < sensor.numOfBuffers; ++i)
    sensor.readbackBuffers.push_back(graphicsContext.requestReadbackBuffer(GraphicsContext::depthImage, sensor.renderWidth, sensor.renderHeight, &sensor.projection));

  ASSERT(!pyramidChain);
  GraphicsContext::VertexBuffer<GraphicsContext::VertexPN>* vertexBuffer = graphicsContext.requestVertexBuffer<GraphicsContext::VertexPN>();
  auto& vertices = vertexBuffer->vertices;
  Vector3f ml;
  if(projection == perspectiveProjection)
    ml = Vector3f(max, -std::tan(angleX * 0.5f) * max, 0);
  else
    ml = Vector3f(std::cos(angleX * 0.5f) * max, -std::sin(angleX * 0.5f) * max, 0);
  Vector3f mt(ml.x(), 0, std::tan(angleY * 0.5f) * max);
  Vector3f tl(ml.x(), ml.y(), mt.z());
  Vector3f tr(ml.x(), -ml.y(), mt.z());
  Vector3f bl(ml.x(), ml.y(), -mt.z());
  Vector3f br(ml.x(), -ml.y(), -mt.z());
  const unsigned segments = static_cast<int>(18.f * angleX / pi);
  vertices.reserve(5 + (projection == sphericalProjection ? 2 * segments : 0));
  vertices.emplace_back(Vector3f::Zero(), Vector3f(0.f, 0.f, 1.f));
  vertices.emplace_back(tl, Vector3f(0.f, 0.f, 1.f));
  vertices.emplace_back(tr, Vector3f(0.f, 0.f, 1.f));
  vertices.emplace_back(bl, Vector3f(0.f, 0.f, 1.f));
  vertices.emplace_back(br, Vector3f(0.f, 0.f, 1.f));
  if(projection == sphericalProjection && segments > 0)
  {
    const float rotX = std::cos(angleX / static_cast<float>(segments));
    const float rotY = std::sin(angleX / static_cast<float>(segments));
    float x = tl.x();
    float y = tl.y();
    for(unsigned int i = 0; i < segments; ++i)
    {
      vertices.emplace_back(Vector3f(x, y, tl.z()), Vector3f(0.f, 0.f, 1.f));
      const float x2 = x * rotX - y * rotY;
      y = y * rotX + x * rotY;
      x = x2;
    }
    for(unsigned int i = 0; i < segments; ++i)
    {
      vertices.emplace_back(Vector3f(x, y, br.z()), Vector3f(0.f, 0.f, 1.f));
      const float x2 = x * rotX + y * rotY;
      y = y * rotX - x * rotY;
      x = x2;
    }
  }
  vertexBuffer->finish();

  GraphicsContext::IndexBuffer* indexBuffer = graphicsContext.requestIndexBuffer();
  auto& indices = indexBuffer->indices;
  if(projection == sphericalProjection && segments > 0)
  {
    indices.reserve(10 + 4 * segments);
    indices.push_back(5);
    for(unsigned int i = 1; i < segments; ++i)
    {
      indices.push_back(5 + i);
      indices.push_back(5 + i);
    }
    indices.push_back(2);
    indices.push_back(2);
    for(unsigned int i = 0; i < segments; ++i)
    {
      indices.push_back(5 + segments + i);
      indices.push_back(5 + segments + i);
    }
    indices.push_back(3);
  }
  else
  {
    indices.reserve(16);
    indices.push_back(1);
    indices.push_back(2);
    indices.push_back(2);
    indices.push_back(4);
    indices.push_back(4);
    indices.push_back(3);
    indices.push_back(3);
    indices.push_back(1);
  }
  indices.push_back(1);
  indices.push_back(0);
  indices.push_back(0);
  indices.push_back(2);
  indices.push_back(3);
  indices.push_back(0);
  indices.push_back(0);
  indices.push_back(4);

  pyramidChain = graphicsContext.requestMesh(vertexBuffer, indexBuffer, GraphicsContext::lineList);

  ASSERT(!surface);
  static const float color[] = {0.f, 0.f, 0.5f};
  surface = graphicsContext.requestSurface(color);
}

void DepthImageSensor::addParent(Element& element)
{
  sensor.physicalObject = dynamic_cast<::PhysicalObject*>(&element);
  ASSERT(sensor.physicalObject);
  Sensor::addParent(element);
}

void DepthImageSensor::registerObjects()
{
  sensor.fullName = fullName + ".image";
  CoreModule::application->registerObject(*CoreModule::module, sensor, this);

  Sensor::registerObjects();
}

void DepthImageSensor::DistanceSensor::updateValue()
{
  // make sure the poses of all movable objects are up to date
  Simulation::simulation->scene->updateTransformations();

  GraphicsContext& graphicsContext = Simulation::simulation->graphicsContext;
  graphicsContext.updateModelMatrices(GraphicsContext::ModelMatrix::appearance, false);

  // setup camera position
  Pose3f pose = physicalObject->poseInWorld;
  pose.conc(offset);
  static const RotationMatrix cameraRotation = (Matrix3f() << Vector3f(0.f, -1.f, 0.f), Vector3f(0.f, 0.f, 1.f), Vector3f(-1.f, 0.f, 0.f)).finished();
  pose.rotate(cameraRotation);
  pose.rotate(RotationMatrix::aroundY((depthImageSensor->angleX - renderAngleX) / 2.0f));

  // The parts are rendered on top of each other. The GPU converts their depths into distances along the viewing direction.
  if(!graphicsContext.startOffscreenRendering(renderWidth, renderHeight * numOfBuffers, true))
    return;
  int y = 0;
  for(const GraphicsContext::ReadbackBuffer* readbackBuffer : readbackBuffers)
  {
    Matrix4f transformation;
    OpenGLTools::convertTransformation(pose.inverse(), transformation);

    graphicsContext.startRendering(projection, transformation, 0, y, renderWidth, renderHeight);

    // draw all objects
    Simulation::simulation->scene->drawAppearancesCached(graphicsContext);

    graphicsContext.finishRendering();

    graphicsContext.readOffscreenImage(*readbackBuffer, y);
    y += static_cast<int>(renderHeight);

    pose.rotate(RotationMatrix::aroundY(-renderAngleX));
  }
  submission = graphicsContext.finishOffscreenRendering();

  if(depthImageSensor->projection == perspectiveProjection)
    data.floatArray = reinterpret_cast<const float*>(readbackBuffers.front()->getMemory());
  else
  {
    // compute the distances of the points from the distances along the viewing directions of the parts
    graphicsContext.waitForOffscreenRendering(submission);
    submission = 0;
    const float fInvSqr = 1.f / (projection(0, 0) * projection(0, 0));
    const int mid = static_cast<int>(lut[bufferWidth / 2]);
    const float factor = 2.0f / float(renderWidth);
    float* val = imageBuffer;
    unsigned int widthLeft = depthImageSensor->imageWidth;
    for(const GraphicsContext::ReadbackBuffer* readbackBuffer : readbackBuffers)
    {
      const float* distances = reinterpret_cast<const float*>(readbackBuffer->getMemory());
      const unsigned int end = std::min(bufferWidth, widthLeft);
      for(unsigned int i = 0; i < end; ++i)
      {
        const float vx = static_cast<float>(static_cast<int>(lut[i]) - mid) * factor;
        *val++ = std::min<float>(distances[lut[i]] * std::sqrt(1.f + vx * vx * fInvSqr), max);
      }
      widthLeft -= end;
    }
    data.floatArray = imageBuffer;
  }
}

bool DepthImageSensor::DistanceSensor::getMinAndMax(float& min, float& max) const
{
  min = this->min;
  max = this->max;
  return true;
}

void DepthImageSensor::drawPhysics(GraphicsContext& graphicsContext, unsigned int flags) const
{
  if(flags & SimRobotCore3::Renderer::showSensors)
    graphicsContext.draw(pyramidChain, modelMatrix, surface);

  Sensor::drawPhysics(graphicsContext, flags);
}
