/**
 * @file Element.cpp
 *
 * This file implements a base class for all simulation elements.
 *
 * @author Colin Graf
 */

#include "Element.h"
#include "Simulation/Simulation.h"

Element::Element()
{
  Simulation::simulation->elements.push_back(this);
}
