/**
 * @file Simulation/Masses/BoxMass.cpp
 * Implementation of class BoxMass
 * @author Colin Graf
 */

#include "BoxMass.h"

Mass::Properties BoxMass::assembleMass() const
{
  const Vector3f diagonal(value * (width * width + height * height) / 12.f,
                          value * (depth * depth + height * height) / 12.f,
                          value * (depth * depth + width * width) / 12.f);
  return {.mass = value, .inertia = Matrix3f(diagonal.asDiagonal())};
}
