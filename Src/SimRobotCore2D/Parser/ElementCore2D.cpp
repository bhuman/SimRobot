/**
 * @file ElementCore2D.cpp
 *
 * This file implements a base class for all simulation elements.
 *
 * @author Colin Graf
 */

#include "ElementCore2D.h"
#include "Simulation/Simulation.h"

ElementCore2D::ElementCore2D()
{
  Simulation::simulation->elements.push_back(this);
}
