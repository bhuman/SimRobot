/**
 * @file Simulation/Body.cpp
 * Implementation of class Body
 * @author Colin Graf
 */

#include "Body.h"
#include "Platform/Assert.h"
#include "Platform/OpenGL.h"
#include "Simulation/Geometries/Geometry.h"
#include "Simulation/Masses/Mass.h"
#include "Simulation/Scene.h"
#include "Simulation/Simulation.h"
#include "Tools/ODETools.h"
#include "Tools/OpenGLTools.h"
#include <ode/collision.h>
#include <ode/objects.h>

Body::Body()
{
  dMassSetZero(&mass);
}

void Body::addParent(Element& element)
{
  ASSERT(!parent);
  parent = dynamic_cast<::PhysicalObject*>(&element);
  parent->physicalChildren.push_back(this);
  SimObject::addParent(element);
}

Body::~Body()
{
  if(body)
    dBodyDestroy(body);
}

void Body::createPhysics()
{
  ASSERT(!body);

  // register body at parent body
  if(parentBody)
  {
    parentBody->bodyChildren.push_back(this);
    rootBody = parentBody->rootBody;
  }
  else
  {
    Simulation::simulation->scene->bodies.push_back(this);
    rootBody = this;
  }

  // create ode object
  body = dBodyCreate(Simulation::simulation->physicalWorld);
  dBodySetData(body, this);

  // add masses
  for(SimObject* iter : children)
  {
    auto* mass = dynamic_cast<Mass*>(iter);
    if(mass)
      addMass(*mass);
  }

  // compute moment of inertia tensor at center of mass and center of mass position
  centerOfMass += Vector3f(static_cast<float>(mass.c[0]), static_cast<float>(mass.c[1]), static_cast<float>(mass.c[2]));
  dMassTranslate(&mass, -mass.c[0], -mass.c[1], -mass.c[2]);

  // set mass
  dBodySetMass(body, &mass);
  OpenGLTools::convertTransformation(nullptr, &centerOfMass, centerOfMassTransformation);

  // set position
  Pose3f comPose = pose;
  comPose.translate(centerOfMass);
  dBodySetPosition(body, comPose.translation.x(), comPose.translation.y(), comPose.translation.z());
  dMatrix3 matrix3;
  ODETools::convertMatrix(comPose.rotation, matrix3);
  dBodySetRotation(body, matrix3);

  // add geometries
  const Pose3f geomOffset(-centerOfMass);
  for(::PhysicalObject* iter : physicalDrawings)
  {
    auto* geometry = dynamic_cast<Geometry*>(iter);
    if(geometry)
      addGeometry(geomOffset, *geometry);
  }

  //
  ::PhysicalObject::createPhysics();

  OpenGLTools::convertTransformation(pose, transformation);
}

void Body::addGeometry(const Pose3f& parentOffset, Geometry& geometry)
{
  // compute geometry offset
  Pose3f offset = parentOffset;
  if(geometry.translation)
    offset.translate(*geometry.translation);
  if(geometry.rotation)
    offset.rotate(*geometry.rotation);

  // create space if required
  if(!rootBody->bodySpace)
    rootBody->bodySpace = dHashSpaceCreate(Simulation::simulation->movableSpace);

  // create and attach geometry
  dGeomID geom = geometry.createGeometry(rootBody->bodySpace);
  if(geom)
  {
    dGeomSetData(geom, &geometry);
    dGeomSetBody(geom, body);

    // set offset
    dGeomSetOffsetPosition(geom, offset.translation.x(), offset.translation.y(), offset.translation.z());
    dMatrix3 matrix3;
    ODETools::convertMatrix(offset.rotation, matrix3);
    dGeomSetOffsetRotation(geom, matrix3);
  }

  // handle nested geometries
  for(::PhysicalObject* iter : geometry.physicalDrawings)
  {
    Geometry* geometry = dynamic_cast<Geometry*>(iter);
    ASSERT(geometry);
    addGeometry(offset, *geometry);
  }
}

void Body::addMass(Mass& mass)
{
  if(this->mass.mass == 0.f)
  {
    this->mass = mass.createMass();
    if(mass.rotation)
    {
      dMatrix3 matrix;
      ODETools::convertMatrix(*mass.rotation, matrix);
      dMassRotate(&this->mass, matrix);
    }
    if(mass.translation)
      centerOfMass = *mass.translation;
  }
  else
  {
    if(centerOfMass != Vector3f::Zero())
    {
      dMassTranslate(&this->mass, centerOfMass.x(), centerOfMass.y(), centerOfMass.z());
      centerOfMass = Vector3f::Zero();
    }

    const dMass& constAdditionalMass = mass.createMass();
    if(mass.rotation || mass.translation)
    {
      dMass additionalMass = constAdditionalMass;
      if(mass.rotation)
      {
        dMatrix3 matrix;
        ODETools::convertMatrix(*mass.rotation, matrix);
        dMassRotate(&additionalMass, matrix);
      }
      if(mass.translation)
        dMassTranslate(&additionalMass, mass.translation->x(), mass.translation->y(), mass.translation->z());
      dMassAdd(&this->mass, &additionalMass);
    }
    else
      dMassAdd(&this->mass, &constAdditionalMass);
  }
}

void Body::createGraphics()
{
  GraphicalObject::createGraphics();
  for(Body* child : bodyChildren)
    child->createGraphics();
}

void Body::updateTransformation()
{
  // get pose from ode
  ODETools::convertVector(dBodyGetPosition(body), pose.translation);
  ODETools::convertMatrix(dBodyGetRotation(body), pose.rotation);
  pose.translate(-centerOfMass);

  // convert to OpenGL transformation
  OpenGLTools::convertTransformation(pose, transformation);

  //
  for(Body* child : bodyChildren)
    child->updateTransformation();
}

void Body::drawAppearances(SurfaceColor color, bool drawControllerDrawings) const
{
  glPushMatrix();
  glMultMatrixf(transformation);
  GraphicalObject::drawAppearances(color, drawControllerDrawings);
  glPopMatrix();
  for(const Body* child : bodyChildren)
    child->drawAppearances(color, drawControllerDrawings);
}

void Body::drawPhysics(unsigned int flags) const
{
  glPushMatrix();
  glMultMatrixf(transformation);

  if(flags & SimRobotCore2::Renderer::showPhysics)
  {
    // draw center of mass
    glPushMatrix();
    glMultMatrixf(centerOfMassTransformation);
    GLUquadricObj* q = gluNewQuadric();
    glNormal3f(0, 0, 1.f);
    glColor3f(0.8f, 0.f, 0.f);
    gluSphere(q, 0.003, 10, 10);
    gluDeleteQuadric(q);
    glPopMatrix();
  }

  // draw children
  ::PhysicalObject::drawPhysics(flags);

  glPopMatrix();

  for(const Body* child : bodyChildren)
    child->drawPhysics(flags);
}

void Body::move(const Vector3f& offset)
{
  const dReal* pos = dBodyGetPosition(body);
  dBodySetPosition(body, pos[0] + offset.x(), pos[1] + offset.y(), pos[2] + offset.z());
  for(Body* child : bodyChildren)
    child->move(offset);

  Simulation::simulation->scene->lastTransformationUpdateStep = Simulation::simulation->simulationStep - 1; // enforce transformation update
}

void Body::rotate(const RotationMatrix& rotation, const Vector3f& point)
{
  Pose3f comPose;
  ODETools::convertVector(dBodyGetPosition(body), comPose.translation);
  ODETools::convertMatrix(dBodyGetRotation(body), comPose.rotation);

  comPose.translation = rotation * (comPose.translation - point) + point;
  comPose.rotation = rotation * comPose.rotation;

  dBodySetPosition(body, comPose.translation.x(), comPose.translation.y(), comPose.translation.z());
  dMatrix3 matrix3;
  ODETools::convertMatrix(comPose.rotation, matrix3);
  dBodySetRotation(body, matrix3);

  for(Body* body : bodyChildren)
    body->rotate(rotation, point);

  Simulation::simulation->scene->lastTransformationUpdateStep = Simulation::simulation->simulationStep - 1; // enforce transformation update
}

const float* Body::getPosition() const
{
  Pose3f& pose = const_cast<Body*>(this)->pose;
  ODETools::convertVector(dBodyGetPosition(body), pose.translation);
  ODETools::convertMatrix(dBodyGetRotation(body), pose.rotation);
  pose.translate(-centerOfMass);
  return pose.translation.data();
}

bool Body::getPose(float* pos, float (*rot)[3]) const
{
  Pose3f& pose = const_cast<Body*>(this)->pose;
  ODETools::convertVector(dBodyGetPosition(body), pose.translation);
  ODETools::convertMatrix(dBodyGetRotation(body), pose.rotation);
  pose.translate(-centerOfMass);

  pos[0] = pose.translation.x(); pos[1] = pose.translation.y(); pos[2] = pose.translation.z();

  rot[0][0] = pose.rotation(0, 0); rot[0][1] = pose.rotation(1, 0); rot[0][2] = pose.rotation(2, 0);
  rot[1][0] = pose.rotation(0, 1); rot[1][1] = pose.rotation(1, 1); rot[1][2] = pose.rotation(2, 1);
  rot[2][0] = pose.rotation(0, 2); rot[2][1] = pose.rotation(1, 2); rot[2][2] = pose.rotation(2, 2);
  return true;
}

void Body::move(const float* pos)
{
  // get pose from ode
  ODETools::convertVector(dBodyGetPosition(body), pose.translation);
  ODETools::convertMatrix(dBodyGetRotation(body), pose.rotation);
  pose.translate(-centerOfMass);

  // compute position offset
  Vector3f offset = Vector3f(pos[0], pos[1], pos[2]) - pose.translation;

  // move object to new position
  move(offset);
}

void Body::move(const float* pos, const float (*rot)[3])
{
  // get pose from ode
  ODETools::convertVector(dBodyGetPosition(body), pose.translation);
  ODETools::convertMatrix(dBodyGetRotation(body), pose.rotation);
  pose.translate(-centerOfMass);

  // compute position offset
  Pose3f newPose((Matrix3f() << rot[0][0], rot[1][0], rot[2][0],
                                rot[0][1], rot[1][1], rot[2][1],
                                rot[0][2], rot[1][2], rot[2][2]).finished(),
                 Vector3f(pos[0], pos[1], pos[2]));
  Pose3f offset;
  offset.translation = newPose.translation - pose.translation;
  offset.rotation = newPose.rotation * pose.rotation.inverse();

  // move object to new pose
  move(offset.translation);
  rotate(offset.rotation, newPose.translation);
}

void Body::resetDynamics()
{
  dBodySetLinearVel(body, REAL(0.), REAL(0.), REAL(0.));
  dBodySetAngularVel(body, REAL(0.), REAL(0.), REAL(0.));
  for(Body* child : bodyChildren)
    child->resetDynamics();
}

void Body::enablePhysics(bool enable)
{
  enable ? dBodyEnable(body) : dBodyDisable(body);

  if(rootBody->bodySpace)
  {
    if(enable)
      dGeomEnable(reinterpret_cast<dGeomID>(rootBody->bodySpace));
    else
      dGeomDisable(reinterpret_cast<dGeomID>(rootBody->bodySpace));
  }

  for(Body* child : bodyChildren)
    child->enablePhysics(enable);
}
