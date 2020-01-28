/**
 * @file RectMass.cpp
 *
 * This file implements an axis-aligned rectangle mass class.
 *
 * @author Arne Hasselbring
 */

#include "RectMass.h"

void RectMass::setMass()
{
  mass.mass = value;
  mass.center.SetZero();
  mass.I = (1.f / 12.f) * value * (height * height + width * width);
}
