/**
 * @file Simulation/Masses/CapsuleMass.cpp
 * Implementation of class CapsuleMass
 * @author Arne Hasselbring
 */

#include "CapsuleMass.h"
#include <ode/mass.h>

void CapsuleMass::assembleMass()
{
  dMassSetCapsuleTotal(&mass, value, 3, radius, height - radius - radius);
}
