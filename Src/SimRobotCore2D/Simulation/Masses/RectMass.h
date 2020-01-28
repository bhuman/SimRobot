/**
 * @file RectMass.h
 *
 * This file declares an axis-aligned rectangle mass class.
 *
 * @author Arne Hasselbring
 */

#pragma once

#include "Simulation/Masses/Mass.h"

class RectMass : public Mass
{
public:
  float value = 0.f; /**< The mass value [kg]. */
  float width = 0.f; /**< The width of the rectangle (i.e. length along the x-axis). */
  float height = 0.f; /**< The height of the rectangle (i.e. length along the y-axis). */

protected:
  /** Sets mass data for the rectangle mass. */
  void setMass() override;
};
