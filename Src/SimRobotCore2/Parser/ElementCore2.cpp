/**
 * @file Simulation/Element.cpp
 * Implementation of class Element
 * @author Colin Graf
 */

#include "ElementCore2.h"
#include "Simulation/Simulation.h"

ElementCore2::ElementCore2()
{
  Simulation::simulation->elements.push_back(this);
}
