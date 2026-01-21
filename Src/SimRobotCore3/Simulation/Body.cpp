/**
 * @file Simulation/Body.cpp
 * Implementation of class Body
 * @author Colin Graf
 */

#include "Body.h"
#include "Graphics/Primitives.h"
#include "Platform/Assert.h"
#include "Simulation/Actuators/Joint.h"
#include "Simulation/Geometries/Geometry.h"
#include "Simulation/Masses/Mass.h"
#include "Simulation/Scene.h"
#include "Simulation/Simulation.h"
#include <mujoco/mujoco.h>

void Body::addParent(Element& element)
{
  ASSERT(!parent);
  parent = dynamic_cast<::PhysicalObject*>(&element);
  parent->physicalChildren.push_back(this);
  SimObject::addParent(element);
}

void Body::createPhysics(GraphicsContext& graphicsContext)
{
  ASSERT(!body);

  // register body at parent body
  if(parentBody)
  {
    parentBody->bodyChildren.push_back(this);
    body = mjs_addBody(parentBody->body, nullptr);
    rootBody = parentBody->rootBody;
    collisionGroup = rootBody->collisionGroup;
  }
  else
  {
    Simulation::simulation->scene->bodies.push_back(this);
    body = mjs_addBody(Simulation::simulation->worldBody, nullptr);
    if(!dynamic_cast<Joint*>(parent))
    {
      mjs_addFreeJoint(body);
      rootBody = this;
      collisionGroup = Simulation::simulation->scene->detectBodyCollisions ? Simulation::simulation->nextCollisionGroup++ : 1;
    }
    else
    {
      rootBody = nullptr;
      collisionGroup = 0; // belongs to the static world, doesn't collide with it
    }
  }

  mjs_setName(body->element, Simulation::simulation->getName(mjOBJ_BODY, "Body", &bodyIndex, this));

  // add masses
  for(SimObject* iter : children)
  {
    auto* mass = dynamic_cast<Mass*>(iter);
    if(mass)
      addMass(*mass);
  }

  // set position
  {
    const Pose3f poseInParentBody = parentBody ? parentBody->poseInWorld.inverse() * poseInWorld : poseInWorld;
    mju_f2n(body->pos, poseInParentBody.translation.data(), 3);
    mjtNum buf[9];
    mju_f2n(buf, poseInParentBody.rotation.data(), 9);
    mju_mat2Quat(body->quat, buf);
    mju_negQuat(body->quat, body->quat); // column major -> row major
  }

  // add geometries
  const Pose3f geomOffset;
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

void Body::addGeometry(const Pose3f& parentOffset, Geometry& geometry, bool immaterial)
{
  // compute geometry offset
  Pose3f offset = parentOffset;
  if(geometry.translation)
    offset.translate(*geometry.translation);
  if(geometry.rotation)
    offset.rotate(*geometry.rotation);

  // create and attach geometry
  geometry.createGeometry(body, collisionGroup, offset, immaterial);

  // handle nested geometries
  for(::PhysicalObject* iter : geometry.physicalDrawings)
  {
    Geometry* geometry = dynamic_cast<Geometry*>(iter);
    ASSERT(geometry);
    addGeometry(offset, *geometry, immaterial);
  }
}

void Body::addMass(Mass& mass)
{
  if(body->mass == 0.f)
  {
    Vector3f com;
    float inertia[6];
    body->mass = mass.createMass(com, inertia);
    mju_f2n(body->fullinertia, inertia, 6);
    mju_f2n(body->ipos, com.data(), 3);
    /*
    if(mass.rotation)
      rotate mass; -> rotate tensor of inertia and center of mass
    if(mass.translation)
      mju_f2n(body->ipos, mass.translation->data(), 3); // by moving the inertia frame, no adjustment to the tensor of inertia is needed
    */
    mju_addTo3(body->ipos, body->pos);
  }
  /*
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
   */
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
  // get pose from MuJoCo
  mju_n2f(poseInWorld.translation.data(), Simulation::simulation->data->xpos + bodyIndex * 3, 3);
  mju_n2f(poseInWorld.rotation.data(), Simulation::simulation->data->xmat + bodyIndex * 9, 9);
  // from MuJoCo's row major format to column major
  poseInWorld.rotation.transposeInPlace();

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
  if(flags & SimRobotCore3::Renderer::showPhysics)
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
  if(rootBody != this)
    return;
  const mjtNum* pos = Simulation::simulation->data->xpos + bodyIndex * 3;
  ASSERT(Simulation::simulation->model->body_jntnum[bodyIndex] == 1);
  const int jointIndex = Simulation::simulation->model->body_jntadr[bodyIndex];
  ASSERT(Simulation::simulation->model->jnt_type[jointIndex] == mjJNT_FREE);
  const int poseIndex = Simulation::simulation->model->jnt_qposadr[jointIndex];
  Simulation::simulation->data->qpos[poseIndex] = pos[0] + offset.x();
  Simulation::simulation->data->qpos[poseIndex + 1] = pos[1] + offset.y();
  Simulation::simulation->data->qpos[poseIndex + 2] = pos[2] + offset.z();

  // Unfortunately it seems that forward kinematics have to be done for the entire model again.
  mj_kinematics(Simulation::simulation->model, Simulation::simulation->data);

  Simulation::simulation->scene->lastTransformationUpdateStep = Simulation::simulation->simulationStep - 1; // enforce transformation update
}

void Body::rotate(const RotationMatrix& rotation, const Vector3f& point)
{
  if(rootBody != this)
    return;
  Pose3f comPose;
  mju_n2f(comPose.translation.data(), Simulation::simulation->data->xpos + bodyIndex * 3, 3);
  mju_n2f(comPose.rotation.data(), Simulation::simulation->data->xmat + bodyIndex * 9, 9);
  comPose.rotation.transposeInPlace();

  comPose.translation = rotation * (comPose.translation - point) + point;
  comPose.rotation = rotation * comPose.rotation;

  ASSERT(Simulation::simulation->model->body_jntnum[bodyIndex] == 1);
  const int jointIndex = Simulation::simulation->model->body_jntadr[bodyIndex];
  ASSERT(Simulation::simulation->model->jnt_type[jointIndex] == mjJNT_FREE);
  const int poseIndex = Simulation::simulation->model->jnt_qposadr[jointIndex];
  mju_f2n(Simulation::simulation->data->qpos + poseIndex, comPose.translation.data(), 3);
  mjtNum buf[9];
  mju_f2n(buf, comPose.rotation.data(), 9);
  mju_mat2Quat(Simulation::simulation->data->qpos + poseIndex + 3, buf);
  mju_negQuat(Simulation::simulation->data->qpos + poseIndex + 3, Simulation::simulation->data->qpos + poseIndex + 3);

  // Unfortunately it seems that forward kinematics have to be done for the entire model again.
  mj_kinematics(Simulation::simulation->model, Simulation::simulation->data);

  Simulation::simulation->scene->lastTransformationUpdateStep = Simulation::simulation->simulationStep - 1; // enforce transformation update
}

const float* Body::getPosition() const
{
  Pose3f& pose = const_cast<Body*>(this)->poseInWorld;
  mju_n2f(pose.translation.data(), Simulation::simulation->data->xpos + bodyIndex * 3, 3);
  return pose.translation.data();
}

bool Body::getPose(float* pos, float (*rot)[3]) const
{
  Pose3f& pose = const_cast<Body*>(this)->poseInWorld;
  mju_n2f(pose.translation.data(), Simulation::simulation->data->xpos + bodyIndex * 3, 3);
  mju_n2f(pose.rotation.data(), Simulation::simulation->data->xmat + bodyIndex * 9, 9);
  pose.rotation.transposeInPlace();

  pos[0] = pose.translation.x();
  pos[1] = pose.translation.y();
  pos[2] = pose.translation.z();

  rot[0][0] = pose.rotation(0, 0);
  rot[0][1] = pose.rotation(1, 0);
  rot[0][2] = pose.rotation(2, 0);
  rot[1][0] = pose.rotation(0, 1);
  rot[1][1] = pose.rotation(1, 1);
  rot[1][2] = pose.rotation(2, 1);
  rot[2][0] = pose.rotation(0, 2);
  rot[2][1] = pose.rotation(1, 2);
  rot[2][2] = pose.rotation(2, 2);
  return true;
}

const float* Body::getVelocity() const
{
  if(rootBody != this)
    return nullptr;
  // This is only possible for bodies that are connected to the worldbody via a freejoint.
  Vector3f& velocity = const_cast<Body*>(this)->velocityInWorld;

  ASSERT(Simulation::simulation->model->body_jntnum[bodyIndex] == 1);
  const int jointIndex = Simulation::simulation->model->body_jntadr[bodyIndex];
  ASSERT(Simulation::simulation->model->jnt_type[jointIndex] == mjJNT_FREE);
  const int velocityIndex = Simulation::simulation->model->jnt_dofadr[jointIndex];
  mju_n2f(velocity.data(), Simulation::simulation->data->qvel + velocityIndex, 3);
  return velocity.data();
}

void Body::setVelocity(const float* velocity)
{
  if(rootBody != this)
    return;
  // TODO: Is this world or body coordinates?
  ASSERT(Simulation::simulation->model->body_jntnum[bodyIndex] == 1);
  const int jointIndex = Simulation::simulation->model->body_jntadr[bodyIndex];
  ASSERT(Simulation::simulation->model->jnt_type[jointIndex] == mjJNT_FREE);
  const int velocityIndex = Simulation::simulation->model->jnt_dofadr[jointIndex];
  mju_f2n(Simulation::simulation->data->qvel + velocityIndex, velocity, 3);
}

void Body::move(const float* pos)
{
  if(rootBody != this)
    return;
  ASSERT(Simulation::simulation->model->body_jntnum[bodyIndex] == 1);
  const int jointIndex = Simulation::simulation->model->body_jntadr[bodyIndex];
  ASSERT(Simulation::simulation->model->jnt_type[jointIndex] == mjJNT_FREE);
  const int poseIndex = Simulation::simulation->model->jnt_qposadr[jointIndex];
  mju_f2n(Simulation::simulation->data->qpos + poseIndex, pos, 3);

  // Unfortunately it seems that forward kinematics have to be done for the entire model again.
  mj_kinematics(Simulation::simulation->model, Simulation::simulation->data);

  Simulation::simulation->scene->lastTransformationUpdateStep = Simulation::simulation->simulationStep - 1; // enforce transformation update
}

void Body::move(const float* pos, const float (*rot)[3])
{
  if(rootBody != this)
    return;
  // Set translation
  ASSERT(Simulation::simulation->model->body_jntnum[bodyIndex] == 1);
  const int jointIndex = Simulation::simulation->model->body_jntadr[bodyIndex];
  ASSERT(Simulation::simulation->model->jnt_type[jointIndex] == mjJNT_FREE);
  const int poseIndex = Simulation::simulation->model->jnt_qposadr[jointIndex];
  mju_f2n(Simulation::simulation->data->qpos + poseIndex, pos, 3);

  // Set rotation
  mjtNum buf[9];
  mju_f2n(buf, rot[0], 3);
  mju_f2n(buf + 3, rot[1], 3);
  mju_f2n(buf + 6, rot[2], 3);
  mju_mat2Quat(Simulation::simulation->data->qpos + poseIndex + 3, buf);
  mju_negQuat(Simulation::simulation->data->qpos + poseIndex + 3, Simulation::simulation->data->qpos + poseIndex + 3);

  // Unfortunately it seems that forward kinematics have to be done for the entire model again.
  mj_kinematics(Simulation::simulation->model, Simulation::simulation->data);

  Simulation::simulation->scene->lastTransformationUpdateStep = Simulation::simulation->simulationStep - 1; // enforce transformation update
}

void Body::resetDynamics()
{
  mju_zero(Simulation::simulation->data->qvel + Simulation::simulation->model->body_dofadr[bodyIndex], Simulation::simulation->model->body_dofnum[bodyIndex]);
  for(Body* child : bodyChildren)
    child->resetDynamics();
}

void Body::enablePhysics(bool enable)
{
  // enable/disable dynamics
  if(enable)
    --Simulation::simulation->model->ngravcomp;
  else
    ++Simulation::simulation->model->ngravcomp;
  Simulation::simulation->model->body_gravcomp[bodyIndex] = enable ? 0.f : 1.f;

  // enable/disable collisions with associated geoms
  Simulation::simulation->model->body_contype[bodyIndex] = Simulation::simulation->model->body_conaffinity[bodyIndex] = enable ? 1 : 0;

  for(Body* child : bodyChildren)
    child->enablePhysics(enable);
}

void Body::enableGravity(bool enable)
{
  Simulation::simulation->model->body_gravcomp[bodyIndex] = enable ? 0.f : 1.f; // TODO
  for(Body* child : bodyChildren)
    child->enableGravity(enable);
}
