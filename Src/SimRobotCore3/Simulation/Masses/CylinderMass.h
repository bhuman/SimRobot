/**
 * @file Simulation/Masses/CylinderMass.h
 * Declaration of class CylinderMass
 * @author Arne Hasselbring
 */

#pragma once

#include "Mass.h"

/**
 * @class CylinderMass
 * The mass of a cylinder
 */
class CylinderMass : public Mass
{
public:
  float value; /**< The total mass of the cylinder */
  float height; /**< The height of the cylinder */
  float radius; /**< The radius of the cylinder */

private:
  /** Creates the mass (not including children, \c translation or \c rotation) */
  void assembleMass() override;
};
