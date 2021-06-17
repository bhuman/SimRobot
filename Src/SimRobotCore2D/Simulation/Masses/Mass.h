/**
 * @file Mass.h
 *
 * This file declares a base class for masses.
 *
 * @author Arne Hasselbring
 */

#pragma once

#include "Simulation/SimObject.h"
#include "SimRobotCore2D.h"
#include <box2d/b2_shape.h>

class Mass : public SimObject, public SimRobotCore2D::Mass
{
public:
  /**
   * Adds this mass to another mass (updating its CoM and inertia). All masses being added together must be in the same coordinate system.
   * @param massData The mass data to which to add the own mass.
   */
  void addMassData(b2MassData& massData);

protected:
  /** Sets mass data for the concrete mass. */
  virtual void setMass();

  /**
   * Returns the full path to the object in the scene graph.
   * @return The full path ...
   */
  [[nodiscard]] const QString& getFullName() const override;

  /**
   * Returns an icon to visualize the object in the scene graph.
   * @return An icon ...
   */
  [[nodiscard]] const QIcon* getIcon() const override;

  /**
   * Creates a widget for this object.
   * @return The new widget instance.
   */
  SimRobot::Widget* createWidget() override;

  /**
   * Creates a painter for this object.
   * @return The new painter instance.
   */
  SimRobotCore2D::Painter* createPainter() override;

  b2MassData mass; /**< The mass data. */

private:
  /** Initializes the mass and its children. */
  void createMass();

  bool created = false; /**< Whether the mass has been initialized. */
};
