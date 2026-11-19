/**
 * @file Graphics/Backdrop.h
 *
 * This file declares the backdrop of a scene: a procedural room around the scene (walls with bands, a ceiling with lamps and
 * a floor) that is seen in the background and that lights the scene (image-based lighting).
 *
 * @author Arne Hasselbring
 */

#pragma once

#include "Parser/ElementCore3.h"
#include "Tools/Math/Eigen.h"
#include <vector>

class Backdrop;

/**
 * @class Band
 * A horizontal band on the walls of a backdrop (for example the stands or the crowd).
 */
class Band : public ElementCore3
{
public:
  float minHeight = 0.f; /**< The height of the lower edge of the band in meters. */
  float maxHeight = 1.f; /**< The height of the upper edge of the band in meters. */
  float color[3] = {0.5f, 0.5f, 0.5f}; /**< The color (sRGB, as in surfaces). */
  float variation = 0.f; /**< How much the brightness changes (0: not at all, 1: from black to twice as bright). */
  float scale = 0.5f; /**< The size of the changes in meters. */

private:
  void addParent(Element& element) override;
};

/**
 * @class Backdrop
 * The backdrop element of an environment.
 */
class Backdrop : public ElementCore3
{
public:
  Vector3f size = Vector3f(40.f, 30.f, 10.f); /**< The size of the room in meters (centered in x and y, the floor is at z = 0). */
  float floorColor[3] = {0.5f, 0.5f, 0.5f}; /**< The color of the floor (sRGB). */
  float wallColor[3] = {0.7f, 0.7f, 0.7f}; /**< The color of the walls (sRGB). */
  float ceilingColor[3] = {0.6f, 0.6f, 0.6f}; /**< The color of the ceiling (sRGB). */
  float intensity = 1.f; /**< The factor for the brightness of the floor, the walls and the ceiling. */
  float lampColor[3] = {1.f, 1.f, 1.f}; /**< The color of the lamps in the ceiling (sRGB). */
  float lampIntensity = 0.f; /**< The brightness of the lamps (0: there are none). */
  int lampRows = 3; /**< The number of rows of lamps (in the direction of the y axis). */
  int lampColumns = 5; /**< The number of columns of lamps (in the direction of the x axis). */
  float lampSize = 1.f; /**< The side length of the (square) lamps in meters. */
  int resolution = 256; /**< The size of the sides of the cube map of the backdrop in pixels. */
  std::vector<const Band*> bands; /**< The bands on the walls (later ones are drawn over earlier ones). */

  /**
   * Computes the radiance that comes from a direction at the center of the room, 0.5 m above the floor.
   * @param direction The direction (a unit vector).
   * @return The radiance (linear RGB).
   */
  Vector3f radiance(const Vector3f& direction) const;

private:
  void addParent(Element& element) override;
};
