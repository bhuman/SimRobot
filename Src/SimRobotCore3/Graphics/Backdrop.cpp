/**
 * @file Graphics/Backdrop.cpp
 *
 * This file implements the backdrop of a scene.
 *
 * @author Arne Hasselbring
 */

#include "Backdrop.h"
#include "Graphics/Environment.h"
#include "Platform/Assert.h"
#include <algorithm>
#include <cmath>

namespace
{
  Vector3f toLinear(const float* srgb)
  {
    return Vector3f(std::pow(srgb[0], 2.2f), std::pow(srgb[1], 2.2f), std::pow(srgb[2], 2.2f));
  }

  float hash(float x, float y)
  {
    const float h = std::sin(x * 127.1f + y * 311.7f) * 43758.5453f;
    return h - std::floor(h);
  }

  /** Smooth noise in [0, 1], with two octaves. */
  float noise(float x, float y)
  {
    float result = 0.f;
    float amplitude = 0.65f;
    for(int i = 0; i < 2; ++i)
    {
      const float ix = std::floor(x), iy = std::floor(y);
      float fx = x - ix, fy = y - iy;
      fx = fx * fx * (3.f - 2.f * fx);
      fy = fy * fy * (3.f - 2.f * fy);
      const float a = hash(ix, iy), b = hash(ix + 1.f, iy), c = hash(ix, iy + 1.f), d = hash(ix + 1.f, iy + 1.f);
      result += amplitude * (a + (b - a) * fx + (c - a) * fy + (a - b - c + d) * fx * fy);
      x *= 2.7f;
      y *= 2.7f;
      amplitude *= 0.35f / 0.65f;
    }
    return result;
  }
}

void Band::addParent(Element& element)
{
  Backdrop* backdrop = dynamic_cast<Backdrop*>(&element);
  ASSERT(backdrop);
  backdrop->bands.push_back(this);
}

void Backdrop::addParent(Element& element)
{
  Environment* environment = dynamic_cast<Environment*>(&element);
  ASSERT(environment);
  ASSERT(!environment->backdrop);
  environment->backdrop = this;
}

Vector3f Backdrop::radiance(const Vector3f& direction) const
{
  const float eyeHeight = std::min(0.5f, 0.5f * size.z());
  const Vector3f origin(0.f, 0.f, eyeHeight);
  const Vector3f half = 0.5f * size;

  // Which plane of the room is hit first?
  float distance = std::numeric_limits<float>::max();
  int plane = 0; // 0: floor, 1: ceiling, 2: wall in x direction, 3: wall in y direction
  const auto consider = [&](float t, int hit)
  {
    if(t > 0.f && t < distance)
    {
      distance = t;
      plane = hit;
    }
  };
  if(direction.z() < 0.f)
    consider(-origin.z() / direction.z(), 0);
  else if(direction.z() > 0.f)
    consider((size.z() - origin.z()) / direction.z(), 1);
  if(direction.x() != 0.f)
    consider(((direction.x() > 0.f ? half.x() : -half.x()) - origin.x()) / direction.x(), 2);
  if(direction.y() != 0.f)
    consider(((direction.y() > 0.f ? half.y() : -half.y()) - origin.y()) / direction.y(), 3);
  const Vector3f hit = origin + direction * distance;

  switch(plane)
  {
    case 0:
      return toLinear(floorColor) * intensity;
    case 1:
    {
      Vector3f color = toLinear(ceilingColor) * intensity;
      if(lampIntensity > 0.f && lampRows > 0 && lampColumns > 0)
      {
        // The lamps are in a grid that covers the inner part of the ceiling.
        const float softness = std::max(0.15f * lampSize, 1e-3f);
        const auto coverage = [&](float position, float extent, int count)
        {
          const float cell = 0.8f * extent / static_cast<float>(count);
          const float inside = position + 0.4f * extent;
          const float index = std::floor(inside / cell);
          if(index < 0.f || index >= static_cast<float>(count))
            return 0.f;
          const float local = inside - (index + 0.5f) * cell;
          const float edge = 0.5f * lampSize - std::abs(local);
          return std::clamp(edge / softness + 0.5f, 0.f, 1.f);
        };
        const float lamp = coverage(hit.x(), size.x(), lampColumns) * coverage(hit.y(), size.y(), lampRows);
        color += lamp * toLinear(lampColor) * lampIntensity;
      }
      return color;
    }
    default:
    {
      Vector3f color = toLinear(wallColor) * intensity;
      const float along = plane == 2 ? hit.y() + (direction.x() > 0.f ? 0.f : 1000.f) : hit.x() + (direction.y() > 0.f ? 0.f : 1000.f);
      for(const Band* band : bands)
        if(hit.z() >= band->minHeight && hit.z() <= band->maxHeight)
        {
          const float scale = std::max(band->scale, 1e-3f);
          const float factor = 1.f + band->variation * (2.f * noise(along / scale, hit.z() / scale) - 1.f);
          color = toLinear(band->color) * intensity * std::max(factor, 0.f);
        }
      return color;
    }
  }
}
