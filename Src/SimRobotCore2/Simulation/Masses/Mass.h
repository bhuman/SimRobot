/**
 * @file Simulation/Masses/Mass.h
 * Declaration of class Mass
 * @author Colin Graf
 */

#pragma once

#include "Simulation/SimObject.h"
#include "Simulation/PhysicalObject.h"
#include <ode/mass.h>

/**
 * @class Mass
 * Abstract class for masses of physical objects
 */
class Mass : public SimObject, public SimRobotCore2::Mass
{
public:
  /**
  * Creates the mass of a physical object (including children and not including \c translation and \c rotation)
  * @return The mass
  */
  const dMass& createMass();

protected:
  dMass mass;
  bool created = false;

  /** Creates the mass (not including children, \c translation or \c rotation) */
  virtual void assembleMass();

private:
  // API
  const QString& getFullName() const override {return SimObject::getFullName();}
  SimRobot::Widget* createWidget() override {return SimObject::createWidget();}
  const QIcon* getIcon() const override {return SimObject::getIcon();}
  SimRobotCore2::Renderer* createRenderer() override {return SimObject::createRenderer();}
};
