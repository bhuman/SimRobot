/**
 * @file Simulation/Masses/SphereMass.cpp
 * Implementation of class SphereMass
 * @author Colin Graf
 */

#include "SphereMass.h"

Mass::Properties SphereMass::assembleMass() const
{
  const Vector3f diagonal = Vector3f::Constant(0.4f * value * radius * radius);
  return {.mass = value, .inertia = Matrix3f(diagonal.asDiagonal())};
}
