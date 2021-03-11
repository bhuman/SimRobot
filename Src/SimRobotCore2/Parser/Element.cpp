/**
 * @file Simulation/Element.cpp
 * Implementation of class Element
 * @author Colin Graf
 */

#include "Element.h"
#include "Simulation/Simulation.h"

Element::Element()
{
  Simulation::simulation->elements.push_back(this);
}
