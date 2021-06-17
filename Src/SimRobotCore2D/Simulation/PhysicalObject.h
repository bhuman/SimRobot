/**
 * @file PhysicalObject.h
 *
 * This file declares a base class for objects with physical properties.
 *
 * @author Arne Hasselbring
 */

#pragma once

#include "Simulation/SimObject.h"
#include <box2d/b2_math.h>
#include <list>

class Body;
class QPainter;

class PhysicalObject : public SimObject
{
public:
  /** Initializes the physical properties of the object. */
  virtual void createPhysics();

  /**
   * Draws the physical properties of the object.
   * @param painter The drawing helper.
   */
  virtual void drawPhysics(QPainter& painter) const;

  PhysicalObject* parent = nullptr; /**< The parent physical object. */
  Body* parentBody = nullptr; /**< The parent body. */
  std::list<PhysicalObject*> physicalChildren; /**< The physical object children. */
  std::list<PhysicalObject*> physicalDrawings; /**< The physical object children which should be drawn below this object (because bodies have global poses). */
  b2Transform pose; /**< The global pose of this object (which is only updated for bodies). */

protected:
  /**
   * Registers another physical object as parent of this element.
   * @param element The element to register.
   */
  void addParent(Element& element) override;
};
