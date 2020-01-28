/**
 * @file PointMass.h
 *
 * This file declares a point mass class.
 *
 * @author Arne Hasselbring
 */

#pragma once

#include "Simulation/Masses/Mass.h"

class PointMass : public Mass
{
public:
  float value = 0.f; /**< The mass value [kg]. */

protected:
  /** Sets mass data for the point mass. */
  void setMass() override;
};
