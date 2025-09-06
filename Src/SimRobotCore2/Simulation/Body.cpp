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
#include <mujoco/mujoco.h>

Body::Body()
{
  // dMassSetZero(&mass);
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
  /*
  if(body)
    dBodyDestroy(body);
   */
}

void Body::createPhysics(GraphicsContext& graphicsContext)
{
  ASSERT(!body);

  // register body at parent body
  if(parentBody)
  {
    parentBody->bodyChildren.push_back(this);
    rootBody = parentBody->rootBody;
    body = mjs_addBody(parentBody->body, nullptr);
  }
  else
  {
    static int gxxx = 0;
    xxx = ++gxxx;
    Simulation::simulation->scene->bodies.push_back(this);
    rootBody = this;
    body = mjs_addBody(Simulation::simulation->worldbody, nullptr);
    mjs_addFreeJoint(body);
  }

  mjs_setName(body->element, Simulation::simulation->getName(mjOBJ_BODY, "body", &idx));

  // add masses
  for(SimObject* iter : children)
  {
    auto* mass = dynamic_cast<Mass*>(iter);
    if(mass)
      addMass(*mass);
  }

  // set position
  if(parentBody)
  {
    const Pose3f poseInParentBody = parentBody->poseInWorld.inverse() * poseInWorld;
    mju_f2n(body->pos, poseInParentBody.translation.data(), 3);
    mjtNum buf[9];
    mju_f2n(buf, poseInParentBody.rotation.transpose().data(), 9);
    mju_mat2Quat(body->quat, buf);
  }
  else
  {
    mju_f2n(body->pos, poseInWorld.translation.data(), 3);
    mjtNum buf[9];
    mju_f2n(buf, poseInWorld.rotation.transpose().data(), 9);
    mju_mat2Quat(body->quat, buf);
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

void Body::addGeometry(const Pose3f& parentOffset, Geometry& geometry)
{
  // compute geometry offset
  Pose3f offset = parentOffset;
  if(geometry.translation)
    offset.translate(*geometry.translation);
  if(geometry.rotation)
    offset.rotate(*geometry.rotation);

  // create space if required
  // TODO: MJC doesn't work with spaces, but we still need collision group to enable/disable physics
  /*
  if(!rootBody->bodySpace)
    rootBody->bodySpace = dHashSpaceCreate(Simulation::simulation->movableSpace);
   */

  // create and attach geometry
  mjsGeom* geom = geometry.createGeometry(body); // in rootBody->bodySpace
  if(geom)
  {
    // dGeomSetData(geom, &geometry);
    geom->contype = (1 << rootBody->xxx);
    geom->conaffinity = ~geom->contype;

    // set offset
    mju_f2n(geom->pos, offset.translation.data(), 3);
    mjtNum buf[9];
    mju_f2n(buf, offset.rotation.transpose().data(), 9); // convert Eigen's column major data to MuJoCo's row major data
    mju_mat2Quat(geom->quat, buf);

    if(geometry.material && !geometry.material->frictions.empty())
      geom->friction[0] = geometry.material->frictions.begin()->second;
    if(geometry.material && !geometry.material->rollingFrictions.empty())
    {
      geom->friction[1] = geometry.material->rollingFrictions.begin()->second;
      geom->friction[2] = geom->friction[1] * 0.01f;
      geom->condim = 6;
    }
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
  mju_n2f(poseInWorld.translation.data(), Simulation::simulation->data->xpos + idx * 3, 3);
  mju_n2f(poseInWorld.rotation.data(), Simulation::simulation->data->xmat + idx * 9, 9);
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
  if(rootBody != this)
    return;
  const mjtNum* pos = Simulation::simulation->data->xpos + idx * 3;
  ASSERT(Simulation::simulation->model->body_jntnum[idx] == 1);
  const int jidx = Simulation::simulation->model->body_jntadr[idx];
  ASSERT(Simulation::simulation->model->jnt_type[jidx] == mjJNT_FREE);
  const int qidx = Simulation::simulation->model->jnt_qposadr[jidx];
  Simulation::simulation->data->qpos[qidx] = pos[0] + offset.x();
  Simulation::simulation->data->qpos[qidx + 1] = pos[1] + offset.y();
  Simulation::simulation->data->qpos[qidx + 2] = pos[2] + offset.z();

  // Unfortunately it seems that forward kinematics have to be done for the entire model again.
  mj_kinematics(Simulation::simulation->model, Simulation::simulation->data);

  Simulation::simulation->scene->lastTransformationUpdateStep = Simulation::simulation->simulationStep - 1; // enforce transformation update
}

void Body::rotate(const RotationMatrix& rotation, const Vector3f& point)
{
  if(rootBody != this)
    return;
  Pose3f comPose;
  mju_n2f(comPose.translation.data(), Simulation::simulation->data->xpos + idx * 3, 3);
  mju_n2f(comPose.rotation.data(), Simulation::simulation->data->xmat + idx * 9, 9);
  comPose.rotation.transposeInPlace();

  comPose.translation = rotation * (comPose.translation - point) + point;
  comPose.rotation = rotation * comPose.rotation;
  comPose.rotation.transposeInPlace();

  ASSERT(Simulation::simulation->model->body_jntnum[idx] == 1);
  const int jidx = Simulation::simulation->model->body_jntadr[idx];
  ASSERT(Simulation::simulation->model->jnt_type[jidx] == mjJNT_FREE);
  const int qidx = Simulation::simulation->model->jnt_qposadr[jidx];
  mju_f2n(Simulation::simulation->data->qpos + qidx, comPose.translation.data(), 3);
  mjtNum buf[9];
  mju_f2n(buf, comPose.rotation.data(), 9);
  mju_mat2Quat(Simulation::simulation->data->qpos + qidx + 3, buf);

  // Unfortunately it seems that forward kinematics have to be done for the entire model again.
  mj_kinematics(Simulation::simulation->model, Simulation::simulation->data);

  Simulation::simulation->scene->lastTransformationUpdateStep = Simulation::simulation->simulationStep - 1; // enforce transformation update
}

const float* Body::getPosition() const
{
  Pose3f& pose = const_cast<Body*>(this)->poseInWorld;
  mju_n2f(pose.translation.data(), Simulation::simulation->data->xpos + idx * 3, 3);
  return pose.translation.data();
}

bool Body::getPose(float* pos, float (*rot)[3]) const
{
  Pose3f& pose = const_cast<Body*>(this)->poseInWorld;
  mju_n2f(pose.translation.data(), Simulation::simulation->data->xpos + idx * 3, 3);
  mju_n2f(pose.rotation.data(), Simulation::simulation->data->xmat + idx * 9, 9);
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

  ASSERT(Simulation::simulation->model->body_jntnum[idx] == 1);
  const int jidx = Simulation::simulation->model->body_jntadr[idx];
  ASSERT(Simulation::simulation->model->jnt_type[jidx] == mjJNT_FREE);
  const int dqidx = Simulation::simulation->model->jnt_dofadr[jidx];
  mju_n2f(velocity.data(), Simulation::simulation->data->qvel + dqidx, 3);
  return velocity.data();
}

void Body::setVelocity(const float* velocity)
{
  if(rootBody != this)
    return;
  // TODO: Is this world or body coordinates?
  ASSERT(Simulation::simulation->model->body_jntnum[idx] == 1);
  const int jidx = Simulation::simulation->model->body_jntadr[idx];
  ASSERT(Simulation::simulation->model->jnt_type[jidx] == mjJNT_FREE);
  const int dqidx = Simulation::simulation->model->jnt_dofadr[jidx];
  mju_f2n(Simulation::simulation->data->qvel + dqidx, velocity, 3);
}

void Body::move(const float* pos)
{
  if(rootBody != this)
    return;
  ASSERT(Simulation::simulation->model->body_jntnum[idx] == 1);
  const int jidx = Simulation::simulation->model->body_jntadr[idx];
  ASSERT(Simulation::simulation->model->jnt_type[jidx] == mjJNT_FREE);
  const int qidx = Simulation::simulation->model->jnt_qposadr[jidx];
  mju_f2n(Simulation::simulation->data->qpos + qidx, pos, 3);

  // Unfortunately it seems that forward kinematics have to be done for the entire model again.
  mj_kinematics(Simulation::simulation->model, Simulation::simulation->data);

  Simulation::simulation->scene->lastTransformationUpdateStep = Simulation::simulation->simulationStep - 1; // enforce transformation update
}

void Body::move(const float* pos, const float (*rot)[3])
{
  // Set translation
  ASSERT(Simulation::simulation->model->body_jntnum[idx] == 1);
  const int jidx = Simulation::simulation->model->body_jntadr[idx];
  ASSERT(Simulation::simulation->model->jnt_type[jidx] == mjJNT_FREE);
  const int qidx = Simulation::simulation->model->jnt_qposadr[jidx];
  mju_f2n(Simulation::simulation->data->qpos + qidx, pos, 3);

  // Set rotation
  RotationMatrix targetRotation = (RotationMatrix() << rot[0][0], rot[1][0], rot[2][0],
                                   rot[0][1], rot[1][1], rot[2][1],
                                   rot[0][2], rot[1][2], rot[2][2]).finished();
  targetRotation.transposeInPlace();

  mjtNum buf[9];
  mju_f2n(buf, targetRotation.data(), 9);
  mju_mat2Quat(Simulation::simulation->data->qpos + qidx + 3, buf);

  // Unfortunately it seems that forward kinematics have to be done for the entire model again.
  mj_kinematics(Simulation::simulation->model, Simulation::simulation->data);

  Simulation::simulation->scene->lastTransformationUpdateStep = Simulation::simulation->simulationStep - 1; // enforce transformation update
}

void Body::resetDynamics()
{
  for(int j = 0; j < Simulation::simulation->model->body_jntnum[idx]; ++j)
  {
    const int jidx = Simulation::simulation->model->body_jntadr[idx];
    const int dqidx = Simulation::simulation->model->jnt_dofadr[jidx];
    // TODO: Is there any other chance to get number of DoFs?
    if(Simulation::simulation->model->jnt_type[jidx] == mjJNT_FREE)
      std::memset(Simulation::simulation->data->qvel + dqidx, 0, 6 * sizeof(mjtNum));
    else if(Simulation::simulation->model->jnt_type[jidx] == mjJNT_HINGE)
      std::memset(Simulation::simulation->data->qvel + dqidx, 0, 1 * sizeof(mjtNum));
    else if(Simulation::simulation->model->jnt_type[jidx] == mjJNT_SLIDE)
      std::memset(Simulation::simulation->data->qvel + dqidx, 0, 1 * sizeof(mjtNum));
    else
      ASSERT(false); // we don't support ball joints
  }
  for(Body* child : bodyChildren)
    child->resetDynamics();
}

void Body::enablePhysics(bool enable)
{
  // enable/disable dynamics
  // TODO MJC:
  // enable ? dBodyEnable(body) : dBodyDisable(body);
  // It's rather unfriendly to
  if(enable)
    --Simulation::simulation->model->ngravcomp;
  else
    ++Simulation::simulation->model->ngravcomp;
  Simulation::simulation->model->body_gravcomp[idx] = enable ? 0.f : 1.f;

  // TODO: we would rather keep the body out of the broadphase at all
  // enable/disable collisions with associated geoms
  Simulation::simulation->model->body_contype[idx] = Simulation::simulation->model->body_conaffinity[idx] = enable ? 1 : 0;

  for(Body* child : bodyChildren)
    child->enablePhysics(enable);
}

void Body::enableGravity(bool enable)
{
  Simulation::simulation->model->body_gravcomp[idx] = enable ? 0.f : 1.f; // TODO
  for(Body* child : bodyChildren)
    child->enableGravity(enable);
}
