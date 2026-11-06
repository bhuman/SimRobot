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

std::optional<LightSpace> LightSpace::create(const Light& light)
{
  ASSERT(light.shadowMap);
  const Light::ShadowMap& shadowMap = *light.shadowMap;
  LightSpace lightSpace;

  Vector3f origin, forward;
  const auto* dirLight = dynamic_cast<const DirLight*>(&light);
  if(dirLight)
  {
    origin = shadowMap.aabbCenter;
    forward = Vector3f(dirLight->direction);
  }
  else
  {
    const auto* pointLight = dynamic_cast<const PointLight*>(&light);
    ASSERT(pointLight);
    origin = Vector3f(pointLight->position);
    forward = shadowMap.aabbCenter - origin;
  }
  if(forward.squaredNorm() < 1e-12f)
    return std::nullopt;
  forward.normalize();

  // The light looks along its negative z-axis (like a camera in OpenGL).
  const Vector3f z = -forward;
  const Vector3f up = std::abs(z.z()) > 0.99f ? Vector3f::UnitX() : Vector3f::UnitZ();
  const Vector3f x = up.cross(z).normalized();
  const Vector3f y = z.cross(x);
  Matrix3f rotation;
  rotation << x, y, z;
  lightSpace.view.setIdentity();
  lightSpace.view.topLeftCorner<3, 3>() = rotation.transpose();
  lightSpace.view.topRightCorner<3, 1>() = -rotation.transpose() * origin;

  // The corners of the bounding box in the light's coordinate system.
  Vector3f corners[8];
  for(int i = 0; i < 8; ++i)
  {
    const Vector3f offset = 0.5f * Vector3f((i & 1) ? shadowMap.aabbExtent.x() : -shadowMap.aabbExtent.x(),
                                            (i & 2) ? shadowMap.aabbExtent.y() : -shadowMap.aabbExtent.y(),
                                            (i & 4) ? shadowMap.aabbExtent.z() : -shadowMap.aabbExtent.z());
    corners[i] = lightSpace.view.topLeftCorner<3, 3>() * (shadowMap.aabbCenter + offset) + lightSpace.view.topRightCorner<3, 1>();
  }

  lightSpace.projection.setZero();
  if(dirLight)
  {
    Vector3f minimum = corners[0], maximum = corners[0];
    for(int i = 1; i < 8; ++i)
    {
      minimum = minimum.cwiseMin(corners[i]);
      maximum = maximum.cwiseMax(corners[i]);
    }
    // The distances to the clipping planes are along the negative z-axis.
    const float near = -maximum.z();
    const float far = -minimum.z();
    lightSpace.projection(0, 0) = 2.f / (maximum.x() - minimum.x());
    lightSpace.projection(1, 1) = 2.f / (maximum.y() - minimum.y());
    lightSpace.projection(2, 2) = -2.f / (far - near);
    lightSpace.projection(0, 3) = -(maximum.x() + minimum.x()) / (maximum.x() - minimum.x());
    lightSpace.projection(1, 3) = -(maximum.y() + minimum.y()) / (maximum.y() - minimum.y());
    lightSpace.projection(2, 3) = -(far + near) / (far - near);
    lightSpace.projection(3, 3) = 1.f;
  }
  else
  {
    // Every corner must be in front of the light.
    float tangentX = 0.f, tangentY = 0.f;
    float near = std::numeric_limits<float>::max(), far = 0.f;
    for(const Vector3f& corner : corners)
    {
      const float depth = -corner.z();
      if(depth <= 1e-3f)
        return std::nullopt;
      tangentX = std::max(tangentX, std::abs(corner.x()) / depth);
      tangentY = std::max(tangentY, std::abs(corner.y()) / depth);
      near = std::min(near, depth);
      far = std::max(far, depth);
    }
    // A bit of margin avoids that the border of the bounding box is clipped.
    tangentX *= 1.01f;
    tangentY *= 1.01f;
    near *= 0.99f;
    far *= 1.01f;
    lightSpace.projection(0, 0) = 1.f / tangentX;
    lightSpace.projection(1, 1) = 1.f / tangentY;
    lightSpace.projection(2, 2) = -(far + near) / (far - near);
    lightSpace.projection(2, 3) = -2.f * far * near / (far - near);
    lightSpace.projection(3, 2) = -1.f;
  }
  return lightSpace;
}
