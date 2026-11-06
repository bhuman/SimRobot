/**
 * @file Simulation/Masses/CapsuleMass.cpp
 * Implementation of class CapsuleMass
 * @author Arne Hasselbring
 */

#include "CapsuleMass.h"
#include "Tools/Math/Constants.h"
#include <algorithm>

Mass::Properties CapsuleMass::assembleMass() const
{
  // A capsule is a cylinder along the z-axis with a hemisphere on each end. The mass is distributed uniformly.
  const float cylinderHeight = std::max(height - 2.f * radius, 0.f);
  const float cylinderVolume = Constants::pi * radius * radius * cylinderHeight;
  const float sphereVolume = 4.f / 3.f * Constants::pi * radius * radius * radius;
  const float cylinderMass = value * cylinderVolume / (cylinderVolume + sphereVolume);
  const float hemisphereMass = 0.5f * (value - cylinderMass);

  // The inertia of a hemisphere about its flat face is the same as for a sphere (2/5 m r^2), its center of mass is 3/8 r from the flat face.
  const float hemisphereCenterOffset = 0.375f * radius;
  const float hemisphereTransverse = 0.4f * hemisphereMass * radius * radius - hemisphereMass * hemisphereCenterOffset * hemisphereCenterOffset;
  const float distance = 0.5f * cylinderHeight + hemisphereCenterOffset;

  const float transverse = cylinderMass * (0.25f * radius * radius + cylinderHeight * cylinderHeight / 12.f) +
                           2.f * (hemisphereTransverse + hemisphereMass * distance * distance);
  const Vector3f diagonal(transverse, transverse, 0.5f * cylinderMass * radius * radius + 0.8f * hemisphereMass * radius * radius);
  return {.mass = value, .inertia = Matrix3f(diagonal.asDiagonal())};
}
