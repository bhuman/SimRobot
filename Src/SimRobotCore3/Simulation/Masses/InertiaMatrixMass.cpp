/**
 * @file Simulation/Masses/InertiaMatrixMass.cpp
 * Implementation of class InertiaMatrixMass
 * @author Colin Graf
 */

#include "InertiaMatrixMass.h"

Mass::Properties InertiaMatrixMass::assembleMass() const
{
  Properties properties;
  properties.mass = value;
  properties.com = Vector3f(x, y, z);
  properties.inertia << ixx, ixy, ixz,
                        ixy, iyy, iyz,
                        ixz, iyz, izz;
  return properties;
}
