/**
 * @file Simulation/Masses/CylinderMass.cpp
 * Implementation of class CylinderMass
 * @author Arne Hasselbring
 */

#include "CylinderMass.h"
#include <ode/mass.h>

void CylinderMass::assembleMass()
{
  dMassSetCylinderTotal(&mass, value, 3, radius, height);
}
