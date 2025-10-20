/**
 * @file Simulation/Masses/BoxMass.cpp
 * Implementation of class BoxMass
 * @author Colin Graf
 */

#include "BoxMass.h"

void BoxMass::assembleMass()
{
  mass = value;
  inertia[0] = value * (width * width + height * height) / 12.f;
  inertia[1] = value * (depth * depth + height * height) / 12.f;
  inertia[2] = value * (depth * depth + width * width) / 12.f;
  inertia[3] = inertia[4] = inertia[5] = 0.f;
}
