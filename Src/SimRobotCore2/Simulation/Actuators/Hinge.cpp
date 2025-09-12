/**
 * @file Simulation/Actuators/Hinge.cpp
 * Implementation of class Hinge
 * @author <A href="mailto:tlaue@uni-bremen.de">Tim Laue</A>
 * @author <A href="mailto:kspiess@informatik.uni-bremen.de">Kai Spiess</A>
 * @author Colin Graf
 */

#include "Hinge.h"
#include "CoreModule.h"
#include "Simulation/Axis.h"
#include "Simulation/Body.h"
#include "Simulation/Motors/ServoMotor.h"
#include "Simulation/Motors/VelocityMotor.h"
#include "Simulation/Simulation.h"
#include "Platform/Assert.h"
#include "Tools/Math/Rotation.h"
#include <mujoco/mujoco.h>

void Hinge::createPhysics(GraphicsContext& graphicsContext)
{
  ASSERT(axis);

  axis->create();

  if(axis->deflection && axis->deflection->offset != 0.f)
    poseInWorld.rotate(Rotation::AngleAxis::unpack(Vector3f(axis->x, axis->y, axis->z) * axis->deflection->offset));

  Joint::createPhysics(graphicsContext);

  // find bodies to connect
  [[maybe_unused]] Body* parentBody = dynamic_cast<Body*>(parent);
  ASSERT(!parentBody || parentBody->body);
  ASSERT(!children.empty());
  Body* childBody = dynamic_cast<Body*>(children.front());
  ASSERT(childBody);
  ASSERT(childBody->body);

  // TODO: only if axis has a motor?
  jointName = Simulation::simulation->getName(mjOBJ_JOINT, "Hinge", &jointIndex);
  mjsJoint* joint = mjs_addJoint(childBody->body, nullptr);
  mjs_setName(joint->element, jointName);
  joint->type = mjJNT_HINGE;

  const Vector3f positionInChild = childBody->poseInWorld.inverse() * poseInWorld.translation;
  mju_f2n(joint->pos, positionInChild.data(), 3);
  const Vector3f axisInChild = childBody->poseInWorld.rotation.inverse() * poseInWorld.rotation * Vector3f(axis->x, axis->y, axis->z);
  mju_f2n(joint->axis, axisInChild.data(), 3);

  joint->stiffness = 1.f;

  // joint->springref = 1.f;

  //joint->damping = 0.9f;

  if(axis->deflection)
  {
    joint->limited = mjLIMITED_TRUE;
    joint->range[0] = axis->deflection->min;
    joint->range[1] = axis->deflection->max;
    joint->ref = axis->deflection->offset;
  }

  // create motor
  if(axis->motor)
  {
    axis->motor->create(this);
    if(!dynamic_cast<VelocityMotor*>(axis->motor) && axis->deflection) // Move setpoint to a position inside the deflection range
      axis->motor->setpoint = axis->deflection->offset;
  }
}

const QIcon* Hinge::getIcon() const
{
  return &CoreModule::module->hingeIcon;
}
