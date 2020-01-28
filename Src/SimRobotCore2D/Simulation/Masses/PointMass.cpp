/**
 * @file PointMass.cpp
 *
 * This file implements a point mass class.
 *
 * @author Arne Hasselbring
 */

#include "PointMass.h"

void PointMass::setMass()
{
  mass.mass = value;
  mass.center.SetZero();
  mass.I = 0.f;
}
