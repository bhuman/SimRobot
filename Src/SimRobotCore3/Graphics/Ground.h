/**
 * @file Graphics/Ground.h
 *
 * This file declares the ground of a scene: an infinite horizontal plane with a tile pattern that is seen around the
 * scene (below the horizon), optionally with a line of trees at the horizon.
 *
 * @author Arne Hasselbring
 */

#pragma once

#include "Parser/ElementCore3.h"

/**
 * @class Ground
 * The ground element of an environment.
 */
class Ground : public ElementCore3
{
public:
  float color[3] = {0.5f, 0.5f, 0.5f}; /**< The color of the tiles (sRGB). */
  float height = 0.f; /**< The height of the plane in meters. */
  float tileSize = 1.f; /**< The side length of the (square) tiles in meters (0: no tiles). */
  float jointWidth = 0.01f; /**< The width of the joints between the tiles in meters. */
  float treeDistance = 0.f; /**< The distance of the line of trees from the origin in meters (0: there are none; ignored with a backdrop). */
  float treeHeight = 8.f; /**< The mean height of the trees in meters. */

private:
  void addParent(Element& element) override;
};
