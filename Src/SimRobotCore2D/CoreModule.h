/**
 * @file CoreModule.h
 *
 * This file declares the main class of the SimRobot 2D core.
 *
 * @author Arne Hasselbring
 */

#pragma once

#include "Simulation/Simulation.h"
#include "SimRobot.h"
#include <QIcon>

class CoreModule : public SimRobot::Module, public Simulation
{
public:
  /**
   * Constructor.
   * @param application The application which loaded this module.
   */
  explicit CoreModule(SimRobot::Application& application);

  static SimRobot::Application* application; /**< The instance of SimRobot which loaded this module. */
  static CoreModule* module; /**< The instance of this module. */

  QIcon sceneIcon; /**< An icon for scenes. */
  QIcon objectIcon; /**< An icon for objects in general. */

private:
  /**
   * Loads the scene and initializes the simulation.
   * @return Whether there were no errors during initialization.
   */
  bool compile() override;

  /** Advances the simulation by one step. */
  void update() override;
};
