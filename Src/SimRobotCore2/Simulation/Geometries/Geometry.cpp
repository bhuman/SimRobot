/**
 * @file Simulation/Geometries/Geometry.cpp
 * Implementation of class Geometry
 * @author Colin Graf
 */

#include "Geometry.h"
#include "Platform/Assert.h"
#include "Platform/OpenGL.h"
#include "Tools/OpenGLTools.h"

Geometry::Geometry()
{
  color[0] = color[1] = color[2] = 0.8f;
  color[3] = 1.0f;
}

Geometry::~Geometry()
{
  delete collisionCallbacks;
}

void Geometry::addParent(Element& element)
{
  ::PhysicalObject::addParent(element);
}

dGeomID Geometry::createGeometry(dSpaceID)
{
  if(!created)
  {
    OpenGLTools::convertTransformation(rotation, translation, transformation);
    created = true;
  }
  return nullptr;
}

void Geometry::drawPhysics(unsigned int flags) const
{
  glPushMatrix();
  glMultMatrixf(transformation);
  ::PhysicalObject::drawPhysics(flags);
  glPopMatrix();
}

bool Geometry::registerCollisionCallback(SimRobotCore2::CollisionCallback& collisionCallback)
{
  if(!collisionCallbacks)
    collisionCallbacks = new std::list<SimRobotCore2::CollisionCallback*>();
  collisionCallbacks->push_back(&collisionCallback);
  return false;
}

bool Geometry::unregisterCollisionCallback(SimRobotCore2::CollisionCallback& collisionCallback)
{
  for(auto iter = collisionCallbacks->begin(), end = collisionCallbacks->end(); iter != end; ++iter)
    if(*iter == &collisionCallback)
    {
      collisionCallbacks->erase(iter);
      if(collisionCallbacks->empty())
      {
        delete collisionCallbacks;
        collisionCallbacks = nullptr;
      }
      return true;
    }
  return false;
}

void Geometry::Material::addParent(Element& element)
{
  Geometry* geometry = dynamic_cast<Geometry*>(&element);
  ASSERT(!geometry->material);
  geometry->material = this;
}

bool Geometry::Material::getFriction(const Material& other, float& friction) const
{
  {
    const auto iter = materialToFriction.find(&other);
    if(iter != materialToFriction.end())
    {
      friction = iter->second;
      return friction >= 0.f;
    }
  }

  friction = 0.f;
  int frictionValues = 0;

  {
    const auto iter = frictions.find(other.name);
    if(iter != frictions.end())
    {
      friction += iter->second;
      ++frictionValues;
    }
  }

  {
    const auto iter = other.frictions.find(name);
    if(iter != other.frictions.end())
    {
      friction += iter->second;
      ++frictionValues;
    }
  }

  const bool frictionDefined = frictionValues > 0;
  if(frictionDefined)
    friction /= static_cast<float>(frictionValues);
  else
    friction = -1.f;

  materialToFriction[&other] = friction;
  return frictionDefined;
}

bool Geometry::Material::getRollingFriction(const Material& other, float& rollingFriction) const
{
  {
    const auto iter = materialToRollingFriction.find(&other);
    if(iter != materialToRollingFriction.end())
    {
      rollingFriction = iter->second;
      return rollingFriction >= 0.f;
    }
  }

  {
    const auto iter = rollingFrictions.find(other.name);
    if(iter != rollingFrictions.end())
    {
      rollingFriction = iter->second;
      materialToRollingFriction[&other] = rollingFriction;
      return true;
    }
  }

  rollingFriction = -1.f;
  materialToRollingFriction[&other] = rollingFriction;
  return false;
}
