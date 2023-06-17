/**
 * @file Simulation/Body.cpp
 * Implementation of class Body
 * @author Colin Graf
 */

#include "Body.h"
#include "Graphics/Primitives.h"
#include "Platform/Assert.h"
#include "Simulation/Geometries/Geometry.h"
#include "Simulation/Masses/Mass.h"
#include "Simulation/Scene.h"
#include "Simulation/Simulation.h"
#include "Tools/ODETools.h"
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

void Body::createPhysics(GraphicsContext& graphicsContext)
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

  // set position
  Pose3f comPose = poseInWorld;
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

  poseInParent = poseInWorld;

  graphicsContext.pushModelMatrixStack();

  graphicsContext.pushModelMatrixByReference(poseInParent);

  ASSERT(!::PhysicalObject::modelMatrix);
  ::PhysicalObject::modelMatrix = graphicsContext.requestModelMatrix(GraphicsContext::ModelMatrix::controllerDrawing);

  const Pose3f centerOfMassPose(centerOfMass);
  graphicsContext.pushModelMatrix(centerOfMassPose);
  ASSERT(!comModelMatrix);
  comModelMatrix = graphicsContext.requestModelMatrix(GraphicsContext::ModelMatrix::physicalDrawing);
  graphicsContext.popModelMatrix();

  //
  ::PhysicalObject::createPhysics(graphicsContext);

  graphicsContext.popModelMatrix();
  ASSERT(graphicsContext.emptyModelMatrixStack());
  graphicsContext.popModelMatrixStack();
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

void Body::createGraphics(GraphicsContext& graphicsContext)
{
  ASSERT(graphicsContext.emptyModelMatrixStack());
  graphicsContext.pushModelMatrixByReference(poseInParent);
  ASSERT(!GraphicalObject::modelMatrix);
  GraphicalObject::modelMatrix = graphicsContext.requestModelMatrix(GraphicsContext::ModelMatrix::controllerDrawing);
  GraphicalObject::createGraphics(graphicsContext);
  graphicsContext.popModelMatrix();
  for(Body* child : bodyChildren)
    child->createGraphics(graphicsContext);
}

void Body::updateTransformation()
{
  // get pose from ode
  ODETools::convertVector(dBodyGetPosition(body), poseInWorld.translation);
  ODETools::convertMatrix(dBodyGetRotation(body), poseInWorld.rotation);
  poseInWorld.translate(-centerOfMass);

  // Bodies are always relative to the world.
  poseInParent = poseInWorld;

  //
  for(Body* child : bodyChildren)
    child->updateTransformation();
}

void Body::drawAppearances(GraphicsContext& graphicsContext) const
{
  GraphicalObject::drawAppearances(graphicsContext);
  for(const Body* child : bodyChildren)
    child->drawAppearances(graphicsContext);
}

void Body::visitGraphicalControllerDrawings(const std::function<void(GraphicalObject&)>& accept)
{
  GraphicalObject::visitGraphicalControllerDrawings(accept);
  for(Body* child : bodyChildren)
    accept(*child);
}

void Body::drawPhysics(GraphicsContext& graphicsContext, unsigned int flags) const
{
  // draw center of mass
  if(flags & SimRobotCore2::Renderer::showPhysics)
    graphicsContext.draw(Simulation::simulation->bodyComSphereMesh, comModelMatrix, Simulation::simulation->bodyComSphereSurface);

  // draw children
  ::PhysicalObject::drawPhysics(graphicsContext, flags);

  for(const Body* child : bodyChildren)
    child->drawPhysics(graphicsContext, flags);
}

void Body::visitPhysicalControllerDrawings(const std::function<void(::PhysicalObject&)>& accept)
{
  ::PhysicalObject::visitPhysicalControllerDrawings(accept);
  for(Body* child : bodyChildren)
    accept(*child);
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
  Pose3f& pose = const_cast<Body*>(this)->poseInWorld;
  ODETools::convertVector(dBodyGetPosition(body), pose.translation);
  ODETools::convertMatrix(dBodyGetRotation(body), pose.rotation);
  pose.translate(-centerOfMass);
  return pose.translation.data();
}

bool Body::getPose(float* pos, float (*rot)[3]) const
{
  Pose3f& pose = const_cast<Body*>(this)->poseInWorld;
  ODETools::convertVector(dBodyGetPosition(body), pose.translation);
  ODETools::convertMatrix(dBodyGetRotation(body), pose.rotation);
  pose.translate(-centerOfMass);

  pos[0] = pose.translation.x(); pos[1] = pose.translation.y(); pos[2] = pose.translation.z();

  rot[0][0] = pose.rotation(0, 0); rot[0][1] = pose.rotation(1, 0); rot[0][2] = pose.rotation(2, 0);
  rot[1][0] = pose.rotation(0, 1); rot[1][1] = pose.rotation(1, 1); rot[1][2] = pose.rotation(2, 1);
  rot[2][0] = pose.rotation(0, 2); rot[2][1] = pose.rotation(1, 2); rot[2][2] = pose.rotation(2, 2);
  return true;
}

const float* Body::getVelocity() const
{
  Vector3f& velocity = const_cast<Body*>(this)->velocityInWorld;
  ODETools::convertVector(dBodyGetLinearVel(body), velocity);
  return velocity.data();
}

void Body::setVelocity(const float* velocity)
{
  dBodySetLinearVel(body, REAL(velocity[0]), REAL(velocity[1]), REAL(velocity[2]));
}

void Body::move(const float* pos)
{
  // get pose from ode
  ODETools::convertVector(dBodyGetPosition(body), poseInWorld.translation);
  ODETools::convertMatrix(dBodyGetRotation(body), poseInWorld.rotation);
  poseInWorld.translate(-centerOfMass);

  // compute position offset
  Vector3f offset = Vector3f(pos[0], pos[1], pos[2]) - poseInWorld.translation;

  // move object to new position
  move(offset);
}

void Body::move(const float* pos, const float (*rot)[3])
{
  // get pose from ode
  ODETools::convertVector(dBodyGetPosition(body), poseInWorld.translation);
  ODETools::convertMatrix(dBodyGetRotation(body), poseInWorld.rotation);
  poseInWorld.translate(-centerOfMass);

  // compute position offset
  Pose3f newPose((Matrix3f() << rot[0][0], rot[1][0], rot[2][0],
                                rot[0][1], rot[1][1], rot[2][1],
                                rot[0][2], rot[1][2], rot[2][2]).finished(),
                 Vector3f(pos[0], pos[1], pos[2]));
  Pose3f offset;
  offset.translation = newPose.translation - poseInWorld.translation;
  offset.rotation = newPose.rotation * poseInWorld.rotation.inverse();

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
