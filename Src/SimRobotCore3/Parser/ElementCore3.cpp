/**
 * @file ElementCore3.cpp
 * Implementation of class ElementCore3
 * @author Colin Graf
 */

#include "ElementCore3.h"
#include "Simulation/Simulation.h"

ElementCore3::ElementCore3()
{
  Simulation::simulation->elements.push_back(this);
}
