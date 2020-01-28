/**
 * @file DiskMass.cpp
 *
 * This file implements a disk mass class.
 *
 * @author Arne Hasselbring
 */

#include "DiskMass.h"

void DiskMass::setMass()
{
  mass.mass = value;
  mass.center.SetZero();
  mass.I = 0.5f * value * radius * radius;
}
