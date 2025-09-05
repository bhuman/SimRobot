/**
 * @file Simulation/Actuators/Slider.cpp
 * Implementation of class Slider
 * @author <A href="mailto:tlaue@uni-bremen.de">Tim Laue</A>
 * @author <A href="mailto:kspiess@informatik.uni-bremen.de">Kai Spiess</A>
 * @author Colin Graf
 * @author Thomas Röfer
 */

#include "Slider.h"
#include "CoreModule.h"
#include "Simulation/Axis.h"
#include "Simulation/Body.h"
#include "Simulation/Motors/ServoMotor.h"
#include "Simulation/Simulation.h"
#include "Platform/Assert.h"
#include <mujoco/mujoco.h>

void Slider::createPhysics(GraphicsContext& graphicsContext)
{
  ASSERT(axis);

  //
  axis->create();

  //
  Joint::createPhysics(graphicsContext);

  // find bodies to connect
  [[maybe_unused]] Body* parentBody = dynamic_cast<Body*>(parent);
  ASSERT(!parentBody || parentBody->body);
  ASSERT(!children.empty());
  Body* childBody = dynamic_cast<Body*>(children.front());
  ASSERT(childBody);
  ASSERT(childBody->body);

  mjsJoint* joint = mjs_addJoint(childBody->body, nullptr);
  joint->type = mjJNT_SLIDE;

  // I hope this is correct.
  const Vector3f positionInChild = childBody->poseInWorld.inverse() * poseInWorld.translation;
  joint->pos[0] = positionInChild.x();
  joint->pos[1] = positionInChild.y();
  joint->pos[2] = positionInChild.z();

  // TODO: maybe we also need to set the orientation?

  // This is in local coordinates now. (TODO: of parent or child body?)
  const Vector3f axisInChild = childBody->poseInWorld.inverse() * poseInWorld * Vector3f(axis->x, axis->y, axis->z);
  joint->axis[0] = axisInChild.x();
  joint->axis[1] = axisInChild.y();
  joint->axis[2] = axisInChild.z();

  joint->ref = 0.0;

  if(axis->deflection)
  {
    joint->limited = mjLIMITED_TRUE;
    joint->range[0] = axis->deflection->min;
    joint->range[1] = axis->deflection->max;
    joint->ref = axis->deflection->offset; // TODO: is this necessary?
  }

  /*
  if(axis->cfm != -1.f)
    dJointSetSliderParam(joint, dParamCFM, axis->cfm);

  if(axis->deflection)
  {
    const Axis::Deflection& deflection = *axis->deflection;
    float minSliderLimit = deflection.min;
    float maxSliderLimit = deflection.max;
    if(minSliderLimit > maxSliderLimit)
      minSliderLimit = maxSliderLimit;
    //Set physical limits to higher values (+10%) to avoid strange Slider effects.
    //Otherwise, sometimes the motor exceeds the limits.
    if(dynamic_cast<ServoMotor*>(axis->motor))
    {
      const float internalTolerance = (maxSliderLimit - minSliderLimit) * 0.1f;
      minSliderLimit -= internalTolerance;
      maxSliderLimit += internalTolerance;
    }
    dJointSetSliderParam(joint, dParamLoStop, minSliderLimit);
    dJointSetSliderParam(joint, dParamHiStop, maxSliderLimit);
    if(deflection.stopCFM != -1.f)
      dJointSetSliderParam(joint, dParamStopCFM, deflection.stopCFM);
    if(deflection.stopERP != -1.f)
      dJointSetSliderParam(joint, dParamStopERP, deflection.stopERP);
  }
  */

  // create motor
  if(axis->motor)
    axis->motor->create(this);
}

const QIcon* Slider::getIcon() const
{
  return &CoreModule::module->sliderIcon;
}
