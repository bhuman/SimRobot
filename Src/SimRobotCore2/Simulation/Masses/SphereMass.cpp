/**
 * @file Simulation/Masses/SphereMass.cpp
 * Implementation of class SphereMass
 * @author Colin Graf
 */

#include "SphereMass.h"
#include <ode/mass.h>

void SphereMass::assembleMass()
{
  dMassSetSphereTotal(&mass, value, radius);
}
