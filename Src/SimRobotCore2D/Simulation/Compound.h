/**
 * @file Compound.h
 *
 * This file declares a class for compounds (i.e. static bodies).
 *
 * @author Arne Hasselbring
 */

#pragma once

#include "Simulation/PhysicalObject.h"
#include "SimRobotCore2D.h"

class Compound : public PhysicalObject, public SimRobotCore2D::Compound
{
protected:
  /** Initializes the physical properties of the compound. */
  void createPhysics() override;

  /**
   * Draws the physical properties of the compound.
   * @param painter The drawing helper.
   */
  void drawPhysics(QPainter& painter) const override;

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

  /**
   * Returns the parent body of the physical object.
   * @return The parent body.
   */
  SimRobotCore2D::Body* getParentBody() const override;
};
