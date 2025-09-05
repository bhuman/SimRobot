/**
 * @file Simulation/Masses/SphereMass.cpp
 * Implementation of class SphereMass
 * @author Colin Graf
 */

#include "SphereMass.h"

void SphereMass::assembleMass()
{
  mass = value;
  inertia[0] = 0.4f * value * radius*radius;
  inertia[1] = 0.4f * value * radius*radius;
  inertia[2] = 0.4f * value * radius*radius;
  inertia[3] = 0.f;
  inertia[4] = 0.f;
  inertia[5] = 0.f;
}
