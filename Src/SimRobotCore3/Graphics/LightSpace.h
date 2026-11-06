/**
 * @file Graphics/LightSpace.h
 *
 * This file declares a function that computes the transformations used to render the shadow map of a light.
 * It is independent of the graphics API (but assumes the OpenGL conventions for clip space).
 *
 * @author Arne Hasselbring
 */

#pragma once

#include "Tools/Math/Eigen.h"
#include <optional>

class Light;

/**
 * The transformations from world coordinates to the clip space of a light.
 */
struct LightSpace
{
  Matrix4f view; /**< The view matrix (= inverse pose) of the light. */
  Matrix4f projection; /**< The projection matrix of the light. */

  /**
   * Computes the transformations for the shadow map of a light such that its bounding box (as specified by \c Light::shadowMap) is covered.
   * The light space is rotated around the direction of the light such that the area of the shadow map that is needed is as small as possible.
   * - Directional lights use an orthographic projection.
   * - Point and spot lights use a single perspective projection, i.e. they only cast shadows within its frustum.
   *   - A spot light looks along the direction of its cone (if it has one), and the frustum is the smallest one around the
   *     cone and the bounding box. The light may be inside of the bounding box.
   *   - A point light (and a spot light without a cone) looks along the axis of (approximately) the smallest cone around the
   *     bounding box. It must be outside of the bounding box. Parts of the bounding box that are more than 80 degrees away
   *     from that axis are not covered.
   * @param light The light (which must have a shadow map)
   * @return The transformations or nothing if they could not be computed. This is the case if a point light is inside of
   *         its bounding box, if the bounding box is behind the light, or if a spot light does not light the bounding box.
   */
  static std::optional<LightSpace> create(const Light& light);
};
