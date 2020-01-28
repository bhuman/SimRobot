/**
 * @file DiskMass.h
 *
 * This file declares a disk mass class.
 *
 * @author Arne Hasselbring
 */

#pragma once

#include "Simulation/Masses/Mass.h"

class DiskMass : public Mass
{
public:
  float value = 0.f; /**< The mass value [kg]. */
  float radius = 0.f; /**< The radius of the disk. */

protected:
  /** Sets mass data for the disk mass. */
  void setMass() override;
};
