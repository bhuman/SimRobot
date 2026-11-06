/**
 * @file Simulation/Masses/CylinderMass.cpp
 * Implementation of class CylinderMass
 * @author Arne Hasselbring
 */

#include "CylinderMass.h"

Mass::Properties CylinderMass::assembleMass() const
{
  const float transverse = value * (0.25f * radius * radius + height * height / 12.f);
  const Vector3f diagonal(transverse, transverse, 0.5f * value * radius * radius);
  return {.mass = value, .inertia = Matrix3f(diagonal.asDiagonal())};
}
