/**
 * @file Graphics/Sky.h
 *
 * This file declares the sky of a scene: the sun, the moon, the stars and the light that they cast, which change with the
 * time of day. The sky is also a (directional) light, namely the one of the sun or (at night) of the moon.
 *
 * @author Arne Hasselbring
 */

#pragma once

#include "Graphics/GraphicsContext.h"
#include "Graphics/Light.h"
#include "Tools/Math/Eigen.h"
#include <optional>

/**
 * @class Sky
 * The sky element of a scene
 */
class Environment;

class Sky : public DirLight
{
public:
  /** The source of the time that determines the positions of the sun and the moon. */
  enum Clock
  {
    simulationClock, /**< The time of the simulation (scaled by \c timeScale and added to the start time). */
    wallClock /**< The time of the clock of the computer. */
  };

  Clock clock = simulationClock; /**< The source of the time. */
  int year = 2026; /**< The year of the start date (simulation clock). */
  int month = 6; /**< The month of the start date (1-12). */
  int day = 21; /**< The day of the start date. */
  double startTime = 17.0; /**< The local mean solar time in hours at the start (simulation clock). */
  double timeScale = 0.0; /**< How many seconds pass in the sky per second of simulated time (simulation clock). */
  double latitude = 53.08; /**< The latitude in degrees (north is positive). */
  double longitude = 8.8; /**< The longitude in degrees (east is positive). */
  double northAngle = 90.0; /**< The direction of the north in the scene as an angle in degrees around the z axis, starting at the x axis. */
  float moonRadius = 1.5f; /**< The angular radius of the moon in degrees (the real moon is 0.26 degrees, but it looks larger). */
  std::optional<float> aurora; /**< The visibility of polar lights from 0 to 1 (by default, it depends on the latitude). */
  float clouds = 0.35f; /**< How much of the sky is covered by clouds from 0 to 1. */
  float sunIntensity = 5.5f; /**< The intensity of the sunlight (in the same units as the intensity of lights) if the sun is high. */
  float nightExposure = 6.f; /**< How much brighter than at day the scene is exposed at night. */

  /** Default constructor */
  Sky();

  /**
   * Registers an element as parent (the environment).
   * @param element The element to register
   */
  void addParent(Element& element) override;

  /** The light of the sky changes while the scene is rendered. */
  bool isDynamic() const override {return true;}

  /**
   * Computes the state of the sky for the current time and gives it to the graphics context.
   * Does nothing if the time did not change.
   * @param graphicsContext The graphics context
   */
  void update(GraphicsContext& graphicsContext);

private:
  Environment* environment = nullptr; /**< The environment that this sky is a part of. */
  bool initialized = false; /**< Whether the sky has been computed at least once. */
  double lastTime = 0.0; /**< The (Julian) date for which the sky was computed last. */
};
