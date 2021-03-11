/**
 * @file Simulation/Axis.cpp
 * Implementation of class Axis
 * @author Colin Graf
 */

#include "Axis.h"
#include "Platform/Assert.h"
#include "Simulation/Actuators/Joint.h"
#include "Simulation/Motors/Motor.h"
#include <cmath>

Axis::~Axis()
{
  delete deflection;
  delete motor;
}

void Axis::create()
{
  // normalize axis
  const float len = std::sqrt(x * x + y * y + z * z);
  if(len == 0.f)
    x = 1.f;
  else
  {
    const float invLen = 1.f / len;
    x *= invLen;
    y *= invLen;
    z *= invLen;
  }
}

void Axis::addParent(Element& element)
{
  joint = dynamic_cast<Joint*>(&element);
  ASSERT(!joint->axis);
  joint->axis = this;
}
