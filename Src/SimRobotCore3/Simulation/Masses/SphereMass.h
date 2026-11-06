/**
 * @file Simulation/Masses/SphereMass.h
 * Declaration of class SphereMass
 * @author Colin Graf
 */

#pragma once

#include "Mass.h"

/**
 * @class SphereMass
 * The mass of a sphere
 */
class SphereMass : public Mass
{
public:
  float value; /**< The total mass of the sphere */
  float radius; /**< The radius of the sphere */

private:
  Properties assembleMass() const override;
};
