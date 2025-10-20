/**
 * @file Simulation/Masses/InertiaMatrixMass.cpp
 * Implementation of class InertiaMatrixMass
 * @author Colin Graf
 */

#include "InertiaMatrixMass.h"

void InertiaMatrixMass::assembleMass()
{
  mass = value;
  com = Vector3f(x, y, z);
  inertia[0] = ixx;
  inertia[1] = iyy;
  inertia[2] = izz;
  inertia[3] = ixy;
  inertia[4] = ixz;
  inertia[5] = iyz;
}
