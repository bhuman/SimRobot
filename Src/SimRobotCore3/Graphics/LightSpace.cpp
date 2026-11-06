/**
 * @file Graphics/LightSpace.cpp
 *
 * This file implements a function that computes the transformations used to render the shadow map of a light.
 *
 * @author Arne Hasselbring
 */

#include "LightSpace.h"
#include "Graphics/Light.h"
#include "Platform/Assert.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <vector>

namespace
{
  constexpr float minDepth = 1e-3f; /**< Points closer to the plane through a light that is perpendicular to its direction cannot be projected. */
  constexpr float maxTangent = 5.67f; /**< The tangent of the largest angle (80 degrees) from the direction of a light that is covered by a perspective projection. */
  constexpr float minSpotCutoff = 0.01f; /**< The cutoff above which the cone of a spot light is used to fit the projection (below, it is almost a hemisphere). */
  constexpr int numOfRolls = 45; /**< The number of rotations around the direction of a light that are tried (in steps of 2 degrees). */

  /**
   * The rotation of a light's coordinate system. The light looks along its negative z-axis (like a camera in OpenGL).
   * @param forward The (normalized) direction of the light.
   * @param roll The rotation of the x- and y-axes around the z-axis.
   * @return The rotation (its columns are the axes of the light's coordinate system in world coordinates).
   */
  Matrix3f calcRotation(const Vector3f& forward, float roll)
  {
    const Vector3f z = -forward;
    const Vector3f up = std::abs(z.z()) > 0.99f ? Vector3f::UnitX() : Vector3f::UnitZ();
    const Vector3f x0 = up.cross(z).normalized();
    const Vector3f y0 = z.cross(x0);
    const float c = std::cos(roll), s = std::sin(roll);
    Matrix3f rotation;
    rotation << c * x0 + s * y0, -s * x0 + c * y0, z;
    return rotation;
  }

  /** A rectangle. */
  struct Bounds
  {
    Vector2f minimum = Vector2f::Constant(std::numeric_limits<float>::max()); /**< The lower left corner. */
    Vector2f maximum = Vector2f::Constant(std::numeric_limits<float>::lowest()); /**< The upper right corner. */

    float area() const {return (maximum.x() - minimum.x()) * (maximum.y() - minimum.y());}
  };

  /**
   * Finds the rotation around the axis of a light for which the bounding rectangle of some points has the smallest area.
   * @param points The points (in the coordinate system of a rotation of 0).
   * @param limit The coordinates of the rectangle are limited to [-limit, limit] (e.g. by the cone of a spot light).
   * @param roll The best rotation.
   * @param bounds The bounding rectangle for the best rotation.
   * @return Whether a rotation was found (otherwise, the points are outside of the limit).
   */
  bool fitRoll(const std::vector<Vector2f>& points, float limit, float& roll, Bounds& bounds)
  {
    float bestArea = std::numeric_limits<float>::max();
    for(int i = 0; i < numOfRolls; ++i)
    {
      const float angle = static_cast<float>(i) * (0.5f * std::numbers::pi_v<float> / static_cast<float>(numOfRolls));
      const float c = std::cos(angle), s = std::sin(angle);
      Bounds candidate;
      for(const Vector2f& point : points)
      {
        const Vector2f rotated(c * point.x() + s * point.y(), -s * point.x() + c * point.y());
        candidate.minimum = candidate.minimum.cwiseMin(rotated);
        candidate.maximum = candidate.maximum.cwiseMax(rotated);
      }
      candidate.minimum = candidate.minimum.cwiseMax(Vector2f::Constant(-limit));
      candidate.maximum = candidate.maximum.cwiseMin(Vector2f::Constant(limit));
      if(candidate.maximum.x() < candidate.minimum.x() || candidate.maximum.y() < candidate.minimum.y())
        continue;
      if(candidate.area() < bestArea)
      {
        bestArea = candidate.area();
        roll = angle;
        bounds = candidate;
      }
    }
    return bestArea < std::numeric_limits<float>::max();
  }

  /**
   * Approximates the axis of the smallest cone with its apex at the origin that contains all corners.
   * @param origin The apex.
   * @param corners The corners (in world coordinates).
   * @return The (normalized) axis of the cone. If the origin is not inside of the convex hull of the corners,
   *         the cone is narrower than 180 degrees (but it still can be close to it).
   */
  Vector3f calcEnclosingAxis(const Vector3f& origin, const Vector3f* corners)
  {
    Vector3f directions[8];
    Vector3f axis = Vector3f::Zero();
    for(int i = 0; i < 8; ++i)
    {
      directions[i] = (corners[i] - origin).normalized();
      axis += directions[i];
    }
    if(axis.squaredNorm() < 1e-6f)
      return Vector3f::UnitZ();
    axis.normalize();
    // Move the axis towards the corner that is farthest away from it with decreasing steps.
    for(int i = 0; i < 32; ++i)
    {
      int farthest = 0;
      for(int j = 1; j < 8; ++j)
        if(directions[j].dot(axis) < directions[farthest].dot(axis))
          farthest = j;
      axis = (axis + (directions[farthest] - axis) / static_cast<float>(i + 2)).normalized();
    }
    return axis;
  }
}

std::optional<LightSpace> LightSpace::create(const Light& light)
{
  ASSERT(light.shadowMap);
  const Light::ShadowMap& shadowMap = *light.shadowMap;

  // The corners of the bounding box in world coordinates.
  Vector3f corners[8];
  for(int i = 0; i < 8; ++i)
    corners[i] = shadowMap.aabbCenter + 0.5f * Vector3f((i & 1) ? shadowMap.aabbExtent.x() : -shadowMap.aabbExtent.x(),
                                                        (i & 2) ? shadowMap.aabbExtent.y() : -shadowMap.aabbExtent.y(),
                                                        (i & 4) ? shadowMap.aabbExtent.z() : -shadowMap.aabbExtent.z());

  const auto* dirLight = dynamic_cast<const DirLight*>(&light);
  const auto* pointLight = dynamic_cast<const PointLight*>(&light);
  ASSERT(dirLight || pointLight);
  const auto* spotLight = dynamic_cast<const SpotLight*>(&light);

  Vector3f origin, forward;
  float coneTangent = std::numeric_limits<float>::infinity(); // The tangent of the half angle of the cone of a spot light.
  if(dirLight)
  {
    origin = shadowMap.aabbCenter;
    forward = Vector3f(dirLight->direction);
    if(forward.squaredNorm() < 1e-12f)
      return std::nullopt;
    forward.normalize();
  }
  else
  {
    origin = Vector3f(pointLight->position);
    if(spotLight && spotLight->cutoff > minSpotCutoff && Vector3f(spotLight->direction).squaredNorm() > 1e-12f)
    {
      // Nothing outside of the cone is lit, so the light looks in the direction of the cone.
      forward = Vector3f(spotLight->direction).normalized();
      coneTangent = std::min(std::sqrt(1.f - spotLight->cutoff * spotLight->cutoff) / spotLight->cutoff, maxTangent);
    }
    else
    {
      // The light must look at the whole bounding box, which is only possible if it is outside of it.
      const Vector3f offset = (origin - shadowMap.aabbCenter).cwiseAbs();
      if((offset.array() < 0.5f * shadowMap.aabbExtent.array()).all())
        return std::nullopt;
      forward = calcEnclosingAxis(origin, corners);
    }
  }

  // The corners in the coordinate system of the light (without a roll).
  const Matrix3f rotation0 = calcRotation(forward, 0.f);
  Vector3f cornersInLight[8];
  for(int i = 0; i < 8; ++i)
    cornersInLight[i] = rotation0.transpose() * (corners[i] - origin);

  float roll = 0.f;
  LightSpace lightSpace;
  lightSpace.projection.setZero();
  if(dirLight)
  {
    // Find the rotation around the direction of the light for which the (orthographic) projection of the bounding box is smallest.
    std::vector<Vector2f> points;
    float near = std::numeric_limits<float>::max(), far = std::numeric_limits<float>::lowest();
    for(const Vector3f& corner : cornersInLight)
    {
      points.emplace_back(corner.x(), corner.y());
      near = std::min(near, -corner.z());
      far = std::max(far, -corner.z());
    }
    Bounds bounds;
    VERIFY(fitRoll(points, std::numeric_limits<float>::max(), roll, bounds));
    lightSpace.projection(0, 0) = 2.f / (bounds.maximum.x() - bounds.minimum.x());
    lightSpace.projection(1, 1) = 2.f / (bounds.maximum.y() - bounds.minimum.y());
    lightSpace.projection(2, 2) = -2.f / (far - near);
    lightSpace.projection(0, 3) = -(bounds.maximum.x() + bounds.minimum.x()) / (bounds.maximum.x() - bounds.minimum.x());
    lightSpace.projection(1, 3) = -(bounds.maximum.y() + bounds.minimum.y()) / (bounds.maximum.y() - bounds.minimum.y());
    lightSpace.projection(2, 3) = -(far + near) / (far - near);
    lightSpace.projection(3, 3) = 1.f;
  }
  else
  {
    // Determine which corners can be projected. The light is not able to cover everything, because the angle between the
    // directions to the corners and the direction of the light is limited. For a spot light, this only matters for
    // the extent of the bounding box, since its cone is narrower than that anyway.
    std::vector<Vector2f> points;
    bool allCovered = true;
    float minCoveredDepth = std::numeric_limits<float>::max(), maxDepth = 0.f;
    for(const Vector3f& corner : cornersInLight)
    {
      const float depth = -corner.z();
      maxDepth = std::max(maxDepth, depth);
      if(depth <= minDepth || corner.head<2>().norm() > maxTangent * depth)
      {
        allCovered = false;
        continue;
      }
      points.emplace_back(corner.x() / depth, corner.y() / depth);
      minCoveredDepth = std::min(minCoveredDepth, depth);
    }
    if(points.empty())
      return std::nullopt; // the bounding box is behind the light

    // The tangents of the borders of the frustum (at a distance of 1).
    Bounds bounds;
    if(!allCovered && std::isfinite(coneTangent))
    {
      // The bounding box cannot be used to limit the frustum, but the cone can.
      bounds.minimum = Vector2f::Constant(-coneTangent);
      bounds.maximum = Vector2f::Constant(coneTangent);
    }
    else if(!fitRoll(points, coneTangent, roll, bounds))
      return std::nullopt; // the bounding box is outside of the cone

    // A bit of margin avoids that the border of the bounding box is clipped.
    const Vector2f margin = (0.01f * (bounds.maximum - bounds.minimum)).cwiseMax(1e-3f);
    const Vector2f minimum = bounds.minimum - margin, maximum = bounds.maximum + margin;
    const float near = allCovered ? 0.99f * minCoveredDepth : std::max(0.01f * maxDepth, minDepth);
    const float far = 1.01f * maxDepth;
    if(!(far - near > 1e-3f))
      return std::nullopt;
    lightSpace.projection(0, 0) = 2.f / (maximum.x() - minimum.x());
    lightSpace.projection(0, 2) = (maximum.x() + minimum.x()) / (maximum.x() - minimum.x());
    lightSpace.projection(1, 1) = 2.f / (maximum.y() - minimum.y());
    lightSpace.projection(1, 2) = (maximum.y() + minimum.y()) / (maximum.y() - minimum.y());
    lightSpace.projection(2, 2) = -(far + near) / (far - near);
    lightSpace.projection(2, 3) = -2.f * far * near / (far - near);
    lightSpace.projection(3, 2) = -1.f;
  }

  const Matrix3f rotation = calcRotation(forward, roll);
  lightSpace.view.setIdentity();
  lightSpace.view.topLeftCorner<3, 3>() = rotation.transpose();
  lightSpace.view.topRightCorner<3, 1>() = -rotation.transpose() * origin;
  return lightSpace;
}
