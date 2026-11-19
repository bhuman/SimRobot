/**
 * @file Graphics/EnvironmentMap.h
 *
 * This file declares the maps that light a scene with the radiance that comes from all directions (image-based lighting):
 * a cube map whose first level is the radiance and whose other levels are the radiance prefiltered with the specular lobe
 * of increasing roughness, and spherical harmonics of the irradiance for the diffuse light. They are computed on the CPU
 * from a procedural backdrop, so that all graphics APIs get the same data.
 *
 * @author Arne Hasselbring
 */

#pragma once

#include "Tools/Math/Eigen.h"
#include <array>
#include <cstdint>
#include <memory>
#include <vector>

class Backdrop;

/**
 * @struct EnvironmentMap
 * The cube map and the spherical harmonics of the radiance of an environment.
 */
struct EnvironmentMap
{
  unsigned int resolution = 0; /**< The size of the sides of the first level in pixels. */
  unsigned int levels = 0; /**< The number of levels (the roughness of level i is i / (levels - 1)). */

  /**
   * The pixels (RGBA, half floats) of each side (+x, -x, +y, -y, +z, -z; the order and orientation of OpenGL cube maps, but
   * with the directions in the coordinates of the scene) of each level (index: level * 6 + side). The first row has t = 0
   * (see the table of the OpenGL specification for cube map textures).
   */
  std::vector<std::vector<std::uint16_t>> sides;

  /**
   * The coefficients of the irradiance divided by pi in the basis of \c basis (so that the diffuse radiance of a surface
   * with an albedo is the albedo times the sum over the coefficients times the basis functions of the normal).
   */
  std::array<Vector3f, 9> irradiance;

  /**
   * Returns the size of the sides of a level.
   * @param level The level.
   * @return The size in pixels.
   */
  unsigned int size(unsigned int level) const;

  /**
   * Computes the maps of a backdrop.
   * @param backdrop The backdrop.
   * @return The maps.
   */
  static std::unique_ptr<EnvironmentMap> create(const Backdrop& backdrop);

  /**
   * The real spherical harmonics (up to the second band) that are used for the irradiance.
   * @param direction The direction (a unit vector).
   * @return The values of the nine basis functions.
   */
  static std::array<float, 9> basis(const Vector3f& direction);
};
