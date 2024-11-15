/**
 * @file Simulation/Sensors/SingleDistanceSensor.cpp
 * Implementation of class SingleDistanceSensor
 * @author Colin Graf
 */


#include "SingleDistanceSensor.h"
#include "CoreModule.h"
#include "Graphics/Primitives.h"
#include "Simulation/Body.h"
#include "Platform/Assert.h"
#include <ode/collision.h>
#include <cmath>

SingleDistanceSensor::SingleDistanceSensor()
{
  sensor.sensorType = SimRobotCore2::SensorPort::floatSensor;
  sensor.unit = "m";
}

void SingleDistanceSensor::createPhysics(GraphicsContext& graphicsContext)
{
  Sensor::createPhysics(graphicsContext);

  sensor.geom = dCreateRay(Simulation::simulation->rootSpace, max);
  sensor.min = min;
  sensor.max = max;
  sensor.maxSqrDist = max * max;
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
  sensor.physicalObject = dynamic_cast< ::PhysicalObject*>(&element);
  ASSERT(sensor.physicalObject);
  Sensor::addParent(element);
}

void SingleDistanceSensor::DistanceSensor::staticCollisionCallback(SingleDistanceSensor::DistanceSensor* sensor, dGeomID geom1, dGeomID geom2)
{
  ASSERT(geom1 == sensor->geom);
  ASSERT(!dGeomIsSpace(geom2));

  dContactGeom contactGeoms[4];
  int contacts = dCollide(geom1, geom2, 4, contactGeoms, sizeof(dContactGeom));

  for(int i = 0; i < contacts; ++i)
  {
    const dContactGeom& contactGeom = contactGeoms[i];
    const float sqrDistance = (Vector3f(static_cast<float>(contactGeom.pos[0]), static_cast<float>(contactGeom.pos[1]), static_cast<float>(contactGeom.pos[2])) - sensor->pose.translation).squaredNorm();
    if(sqrDistance < sensor->closestSqrDistance)
    {
      sensor->closestSqrDistance = sqrDistance;
      sensor->closestGeom = geom2;
    }
  }
}

void SingleDistanceSensor::DistanceSensor::staticCollisionWithSpaceCallback(SingleDistanceSensor::DistanceSensor* sensor, dGeomID geom1, dGeomID geom2)
{
  ASSERT(geom1 == sensor->geom);
  ASSERT(dGeomIsSpace(geom2));
  dSpaceCollide2(geom1, geom2, sensor, reinterpret_cast<dNearCallback*>(&staticCollisionCallback));
}

void SingleDistanceSensor::DistanceSensor::updateValue()
{
  pose = physicalObject->poseInWorld;
  pose.conc(offset);
  const Vector3f& pos = pose.translation;
  const Vector3f dir = pose.rotation.col(0);
  dGeomRaySet(geom, static_cast<dReal>(pos.x()), static_cast<dReal>(pos.y()), static_cast<dReal>(pos.z()),
              static_cast<dReal>(dir.x()), static_cast<dReal>(dir.y()), static_cast<dReal>(dir.z()));
  closestGeom = nullptr;
  closestSqrDistance = maxSqrDist;
  dSpaceCollide2(geom, reinterpret_cast<dGeomID>(Simulation::simulation->movableSpace), this, reinterpret_cast<dNearCallback*>(&staticCollisionWithSpaceCallback));
  dSpaceCollide2(geom, reinterpret_cast<dGeomID>(Simulation::simulation->staticSpace), this, reinterpret_cast<dNearCallback*>(&staticCollisionCallback));
  if(closestGeom)
  {
    data.floatValue = std::sqrt(closestSqrDistance);
    if(data.floatValue < min)
      data.floatValue = min;
  }
  else
    data.floatValue = max;
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
