/**
 * @file Simulation/Sensors/CollisionSensor.cpp
 * Implementation of class CollisionSensor
 * @author Colin Graf
 */

#include "CollisionSensor.h"
#include "CoreModule.h"
#include "Platform/OpenGL.h"
#include "Simulation/Body.h"
#include "Simulation/Geometries/Geometry.h"
#include "Tools/OpenGLTools.h"

CollisionSensor::CollisionSensor()
{
  sensor.sensorType = SimRobotCore2::SensorPort::boolSensor;
}

void CollisionSensor::createPhysics()
{
  OpenGLTools::convertTransformation(rotation, translation, transformation);

  // add geometries
  for(auto iter = physicalDrawings.begin(), end = physicalDrawings.end(); iter != end; ++iter)
  {
    Geometry* geometry = dynamic_cast<Geometry*>(*iter);
    if(geometry)
    {
      hasGeometries = true;
      Pose3f geomOffset(-parentBody->centerOfMass);
      if(translation)
        geomOffset.translate(*translation);
      if(rotation)
        geomOffset.rotate(*rotation);
      parentBody->addGeometry(geomOffset, *geometry);
      for(++iter; iter != end; ++iter) // avoid constructing geomOffset again
      {
        geometry = dynamic_cast<Geometry*>(*iter);
        if(geometry)
          parentBody->addGeometry(geomOffset, *geometry);
      }
      break;
    }
  }

  // register collision callback function
  if(hasGeometries)
    registerCollisionCallback(physicalDrawings, true);
  else // in case the sensor has no geometries use the geometries of the body to which the sensor is attached
    registerCollisionCallback(parentBody->physicalDrawings, false);
}

void CollisionSensor::registerCollisionCallback(std::list< ::PhysicalObject*>& geometries, bool setNotCollidable)
{
  for(::PhysicalObject* i : geometries)
  {
    Geometry* geometry = dynamic_cast<Geometry*>(i);
    if(geometry && !geometry->immaterial)
    {
      if(setNotCollidable)
        geometry->immaterial = true;
      geometry->registerCollisionCallback(sensor);
      registerCollisionCallback(geometry->physicalDrawings, setNotCollidable);
    }
  }
}

void CollisionSensor::registerObjects()
{
  sensor.fullName = fullName + ".contact";
  CoreModule::application->registerObject(*CoreModule::module, sensor, this);

  Sensor::registerObjects();
}

void CollisionSensor::CollisionSensorPort::updateValue()
{
  data.boolValue = lastCollisionStep == Simulation::simulation->simulationStep;
}

void CollisionSensor::CollisionSensorPort::collided(SimRobotCore2::Geometry&, SimRobotCore2::Geometry&)
{
  lastCollisionStep = Simulation::simulation->simulationStep;
}

void CollisionSensor::drawPhysics(unsigned int flags) const
{
  if(flags & SimRobotCore2::Renderer::showSensors && !hasGeometries)
    for(const ::PhysicalObject* drawing : parentBody->physicalDrawings)
      drawing->drawPhysics(SimRobotCore2::Renderer::showPhysics);

  glPushMatrix();
  glMultMatrixf(transformation);

  if(flags & SimRobotCore2::Renderer::showSensors && hasGeometries)
    for(const ::PhysicalObject* drawing : physicalDrawings)
      drawing->drawPhysics(SimRobotCore2::Renderer::showPhysics);

  Sensor::drawPhysics(flags);
  glPopMatrix();
}
