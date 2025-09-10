/**
 * @file Simulation/Masses/SphereMass.cpp
 * Implementation of class SphereMass
 * @author Colin Graf
 */

#include "SphereMass.h"

void SphereMass::assembleMass()
{
  mass = value;
  inertia[0] = inertia[1] = inertia[2] = 0.4f * value * radius * radius;
  inertia[3] = inertia[4] = inertia[5] = 0.f;
}
