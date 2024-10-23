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
#include <ode/objects.h>

void Hinge::createPhysics(GraphicsContext& graphicsContext)
{
  ASSERT(axis);

  //
  axis->create();

  if(axis->deflection && axis->deflection->offset != 0.f)
    poseInWorld.rotate(Rotation::AngleAxis::unpack(Vector3f(axis->x, axis->y, axis->z) * axis->deflection->offset));

  //
  Joint::createPhysics(graphicsContext);

  // find bodies to connect
  Body* parentBody = dynamic_cast<Body*>(parent);
  ASSERT(!parentBody || parentBody->body);
  ASSERT(!children.empty());
  Body* childBody = dynamic_cast<Body*>(children.front());
  ASSERT(childBody);
  ASSERT(childBody->body);

  // create joint
  joint = dJointCreateHinge(Simulation::simulation->physicalWorld, 0);
  dJointAttach(joint, childBody->body, parentBody ? parentBody->body : 0);
  //set hinge joint parameter
  dJointSetHingeAnchor(joint, poseInWorld.translation.x(), poseInWorld.translation.y(), poseInWorld.translation.z());
  const Vector3f globalAxis = poseInWorld.rotation * Vector3f(axis->x, axis->y, axis->z);
  dJointSetHingeAxis(joint, globalAxis.x(), globalAxis.y(), globalAxis.z());
  if(axis->cfm != -1.f)
    dJointSetHingeParam(joint, dParamCFM, axis->cfm);

  if(axis->deflection)
  {
    const Axis::Deflection& deflection = *axis->deflection;

    // Only set stops if requested
    if(deflection.setStops)
    {
      float minHingeLimit = deflection.min;
      float maxHingeLimit = deflection.max;
      if(minHingeLimit > maxHingeLimit)
        minHingeLimit = maxHingeLimit;

      dJointSetHingeParam(joint, dParamLoStop, minHingeLimit - deflection.offset);
      dJointSetHingeParam(joint, dParamHiStop, maxHingeLimit - deflection.offset);
    }

    if(deflection.stopCFM != -1.f)
      dJointSetHingeParam(joint, dParamStopCFM, deflection.stopCFM);
    if(deflection.stopERP != -1.f)
      dJointSetHingeParam(joint, dParamStopERP, deflection.stopERP);
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
