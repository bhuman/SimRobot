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
   * Directional lights use an orthographic projection. Point and spot lights use a single perspective projection
   * that looks from the light's position at the center of the bounding box, i.e. they only cast shadows within that frustum.
   * @param light The light (which must have a shadow map)
   * @return The transformations or nothing if they could not be computed. This is the case if a point or spot light is not outside of its bounding box.
   */
  static std::optional<LightSpace> create(const Light& light);
};
