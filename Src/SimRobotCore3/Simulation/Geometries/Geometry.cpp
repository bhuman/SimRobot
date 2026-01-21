/**
 * @file Simulation/Geometries/Geometry.cpp
 * Implementation of class Geometry
 * @author Colin Graf
 */

#include "Geometry.h"
#include "Platform/Assert.h"
#include "Tools/OpenGLTools.h"
#include <mujoco/mujoco.h>

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

void Geometry::createGeometry(mjsBody* body, int collisionGroup, const Pose3f& offset, bool immaterial)
{
  if(!created)
  {
    OpenGLTools::convertTransformation(rotation, translation, poseInParent);
    created = true;
  }

  mjsGeom* geom = assembleGeometry(body);
  if(geom)
  {
    ASSERT(collisionGroup < 32);
    geom->contype = 1 << collisionGroup;
    geom->conaffinity = ~geom->contype;

    // set offset
    mju_f2n(geom->pos, offset.translation.data(), 3);
    mjtNum buf[9];
    mju_f2n(buf, offset.rotation.data(), 9);
    mju_mat2Quat(geom->quat, buf);
    mju_negQuat(geom->quat, geom->quat); // column major -> row major

    geom->condim = 3;
    geom->friction[0] = 1.f;
    geom->friction[1] = 0.f;
    geom->friction[2] = 0.f;

    if(material)
    {
      geom->priority = 1;
      if(material->friction > 0.f)
        geom->friction[0] = material->friction;
      else
      {
        geom->friction[0] = 0.f;
        geom->condim = 1;
      }
      if(material->rollingFriction > 0.f)
      {
        geom->friction[1] = material->rollingFriction;
        geom->friction[2] = material->rollingFriction;
        geom->condim = 6;
      }
    }

    if(immaterial)
      geom->gap = std::numeric_limits<decltype(geom->gap)>::max();
  }
}

void Geometry::createPhysics(GraphicsContext& graphicsContext)
{
  // \c createGeometry must have been called before.
  graphicsContext.pushModelMatrix(poseInParent);
  ASSERT(!modelMatrix);
  modelMatrix = graphicsContext.requestModelMatrix(GraphicsContext::ModelMatrix::physicalDrawing);
  ::PhysicalObject::createPhysics(graphicsContext);
  graphicsContext.popModelMatrix();

  ASSERT(!surface);
  surface = graphicsContext.requestSurface(color, color);
}

bool Geometry::registerCollisionCallback(SimRobotCore3::CollisionCallback& collisionCallback)
{
  if(!collisionCallbacks)
    collisionCallbacks = new std::list<SimRobotCore3::CollisionCallback*>();
  collisionCallbacks->push_back(&collisionCallback);
  return false;
}

bool Geometry::unregisterCollisionCallback(SimRobotCore3::CollisionCallback& collisionCallback)
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
  ASSERT(geometry);
  ASSERT(!geometry->material);
  geometry->material = this;
}
