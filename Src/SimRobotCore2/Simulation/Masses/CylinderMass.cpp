/**
 * @file Simulation/Masses/CylinderMass.cpp
 * Implementation of class CylinderMass
 * @author Arne Hasselbring
 */

#include "CylinderMass.h"

void CylinderMass::assembleMass()
{
  mass = value;
  inertia[0] = inertia[1] = value * (0.25f * radius * radius + height * height / 12.f);
  inertia[2] = 0.5f * value * radius * radius;
  inertia[3] = inertia[4] = inertia[5] = 0.f;
}
