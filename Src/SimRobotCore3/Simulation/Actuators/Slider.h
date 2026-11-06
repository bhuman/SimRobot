/**
 * @file Simulation/Actuators/Slider.h
 * Declaration of class Slider
 * @author Colin Graf
 * @author Thomas Röfer
 */

#pragma once

#include "Joint.h"

/**
 * @class Slider
 * A Slider joint that connects two bodies
 */
class Slider : public Joint
{
private:
  /**
   * Creates the physical objects used by MuJoCo.
   * These are a geometry object for collision detection and/or a body,
   * if the simulation object is movable.
   * @param graphicsContext The graphics context to create resources in
   */
  void createPhysics(GraphicsContext& graphicsContext) override;

  //API
  const QIcon* getIcon() const override;
};
