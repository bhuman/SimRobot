/**
 * @file Graphics/EnvironmentMap.cpp
 *
 * This file implements the computation of the maps of an environment.
 *
 * @author Arne Hasselbring
 */

#include "EnvironmentMap.h"
#include "Graphics/Backdrop.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <future>

namespace
{
  constexpr float pi = 3.14159265358979f;
  constexpr unsigned int minimumSize = 8; /**< The size of the sides of the smallest (roughest) level. */
  constexpr unsigned int numOfSamples = 64; /**< The number of samples of the specular lobe per pixel of a prefiltered level. */

  using Image = std::vector<Vector3f>; /**< The pixels of a side. */
  using Sides = std::array<Image, 6>;

  /**
   * Converts a float into a half float (rounding to nearest, large values are clamped to the largest half float).
   */
  std::uint16_t toHalf(float value)
  {
    if(!(value > 0.f))
      return 0;
    value = std::min(value, 65504.f);
    const std::uint32_t bits = std::bit_cast<std::uint32_t>(value);
    const int exponent = static_cast<int>((bits >> 23) & 0xff) - 127 + 15;
    std::uint32_t mantissa = bits & 0x7fffff;
    if(exponent <= 0)
    {
      // Denormals (or zero).
      if(exponent < -10)
        return 0;
      mantissa |= 0x800000;
      const int shift = 14 - exponent;
      std::uint32_t half = mantissa >> shift;
      if((mantissa >> (shift - 1)) & 1)
        ++half;
      return static_cast<std::uint16_t>(half);
    }
    std::uint32_t half = (static_cast<std::uint32_t>(exponent) << 10) | (mantissa >> 13);
    if(mantissa & 0x1000)
      ++half; // May carry into the exponent, which is correct.
    return static_cast<std::uint16_t>(std::min<std::uint32_t>(half, 0x7bff));
  }

  /**
   * The direction of the pixel (u, v) of a side of a cube map, using the table of the OpenGL specification.
   * @param side The side.
   * @param sc The coordinate s of the side, from -1 to 1.
   * @param tc The coordinate t of the side, from -1 to 1 (the rows go downwards in the data).
   */
  Vector3f direction(int side, float sc, float tc)
  {
    switch(side)
    {
      case 0: return Vector3f(1.f, -tc, -sc);
      case 1: return Vector3f(-1.f, -tc, sc);
      case 2: return Vector3f(sc, 1.f, tc);
      case 3: return Vector3f(sc, -1.f, -tc);
      case 4: return Vector3f(sc, -tc, 1.f);
      default: return Vector3f(-sc, -tc, -1.f);
    }
  }

  /** The inverse of \c direction: finds the side and the coordinates s and t (from 0 to 1). */
  int side(const Vector3f& d, float& s, float& t)
  {
    const Vector3f a = d.cwiseAbs();
    int result;
    float sc, tc, ma;
    if(a.x() >= a.y() && a.x() >= a.z())
    {
      ma = a.x();
      if(d.x() > 0.f) { result = 0; sc = -d.z(); tc = -d.y(); }
      else { result = 1; sc = d.z(); tc = -d.y(); }
    }
    else if(a.y() >= a.z())
    {
      ma = a.y();
      if(d.y() > 0.f) { result = 2; sc = d.x(); tc = d.z(); }
      else { result = 3; sc = d.x(); tc = -d.z(); }
    }
    else
    {
      ma = a.z();
      if(d.z() > 0.f) { result = 4; sc = d.x(); tc = -d.y(); }
      else { result = 5; sc = -d.x(); tc = -d.y(); }
    }
    s = 0.5f * (sc / ma + 1.f);
    t = 0.5f * (tc / ma + 1.f);
    return result;
  }

  /** Samples a level of a chain of cube maps bilinearly. */
  Vector3f sampleLevel(const std::vector<Sides>& chain, unsigned int size0, unsigned int level, const Vector3f& d)
  {
    float s, t;
    const int index = side(d, s, t);
    const int size = static_cast<int>(size0 >> level);
    const float x = std::clamp(s * static_cast<float>(size) - 0.5f, 0.f, static_cast<float>(size - 1));
    const float y = std::clamp(t * static_cast<float>(size) - 0.5f, 0.f, static_cast<float>(size - 1));
    const int x0 = static_cast<int>(x), y0 = static_cast<int>(y);
    const int x1 = std::min(x0 + 1, size - 1), y1 = std::min(y0 + 1, size - 1);
    const float fx = x - static_cast<float>(x0), fy = y - static_cast<float>(y0);
    const Image& image = chain[level][index];
    const Vector3f top = image[y0 * size + x0] * (1.f - fx) + image[y0 * size + x1] * fx;
    const Vector3f bottom = image[y1 * size + x0] * (1.f - fx) + image[y1 * size + x1] * fx;
    return top * (1.f - fy) + bottom * fy;
  }

  float radicalInverse(std::uint32_t bits)
  {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return static_cast<float>(bits) * 2.3283064365386963e-10f;
  }
}

unsigned int EnvironmentMap::size(unsigned int level) const
{
  return level == 0 ? resolution : std::max(resolution >> level, minimumSize);
}

std::array<float, 9> EnvironmentMap::basis(const Vector3f& d)
{
  return {0.282095f,
          0.488603f * d.y(), 0.488603f * d.z(), 0.488603f * d.x(),
          1.092548f * d.x() * d.y(), 1.092548f * d.y() * d.z(), 0.315392f * (3.f * d.z() * d.z() - 1.f), 1.092548f * d.x() * d.z(), 0.546274f * (d.x() * d.x() - d.y() * d.y())};
}

std::unique_ptr<EnvironmentMap> EnvironmentMap::create(const Backdrop& backdrop)
{
  auto map = std::make_unique<EnvironmentMap>();
  const unsigned int n0 = static_cast<unsigned int>(backdrop.resolution);
  map->resolution = n0;
  // The levels have the sizes n0, n0 / 2, n0 / 4, ... (down to the minimum), the roughness grows from 0 to 1.
  unsigned int levels = 1;
  while((n0 >> levels) >= minimumSize && levels < 8)
    ++levels;
  map->levels = std::max(levels, 2u);
  map->sides.resize(static_cast<std::size_t>(map->levels) * 6);

  // The radiance at the centers of the pixels of the first level (antialiased with four samples per pixel).
  std::vector<Sides> chain(1);
  {
    std::vector<std::future<void>> jobs;
    for(int s = 0; s < 6; ++s)
      jobs.push_back(std::async(std::launch::async, [&, s]
      {
        Image& image = chain[0][s];
        image.resize(static_cast<std::size_t>(n0) * n0);
        for(unsigned int y = 0; y < n0; ++y)
          for(unsigned int x = 0; x < n0; ++x)
          {
            Vector3f sum = Vector3f::Zero();
            for(int k = 0; k < 4; ++k)
            {
              const float sc = 2.f * (static_cast<float>(x) + 0.25f + 0.5f * static_cast<float>(k & 1)) / static_cast<float>(n0) - 1.f;
              const float tc = 2.f * (static_cast<float>(y) + 0.25f + 0.5f * static_cast<float>(k >> 1)) / static_cast<float>(n0) - 1.f;
              sum += backdrop.radiance(direction(s, sc, tc).normalized());
            }
            image[y * n0 + x] = sum * 0.25f;
          }
      }));
    for(auto& job : jobs)
      job.get();
  }

  // A chain of levels that are the average of the pixels of the previous one (the filter for the sampling of the lobe).
  for(unsigned int size = n0 / 2; size >= 4; size /= 2)
  {
    const unsigned int previous = size * 2;
    Sides& next = chain.emplace_back();
    const Sides& source = chain[chain.size() - 2];
    for(int s = 0; s < 6; ++s)
    {
      next[s].resize(static_cast<std::size_t>(size) * size);
      for(unsigned int y = 0; y < size; ++y)
        for(unsigned int x = 0; x < size; ++x)
          next[s][y * size + x] = 0.25f * (source[s][2 * y * previous + 2 * x] + source[s][2 * y * previous + 2 * x + 1] +
                                          source[s][(2 * y + 1) * previous + 2 * x] + source[s][(2 * y + 1) * previous + 2 * x + 1]);
    }
  }

  // Level 0 is the radiance itself.
  const auto store = [&](unsigned int level, int s, const Image& image)
  {
    std::vector<std::uint16_t>& data = map->sides[level * 6 + s];
    data.resize(image.size() * 4);
    for(std::size_t i = 0; i < image.size(); ++i)
    {
      data[i * 4] = toHalf(image[i].x());
      data[i * 4 + 1] = toHalf(image[i].y());
      data[i * 4 + 2] = toHalf(image[i].z());
      data[i * 4 + 3] = toHalf(1.f);
    }
  };
  for(int s = 0; s < 6; ++s)
    store(0, s, chain[0][s]);

  // The other levels are prefiltered with the specular lobe (GGX) of the roughness of the level. The normal, the view and the
  // reflection direction are assumed to be the same. The samples are taken from a level of the chain that depends on their density.
  const float solidAngleOfPixel = 4.f * pi / (6.f * static_cast<float>(n0) * static_cast<float>(n0));
  const unsigned int numOfChainLevels = static_cast<unsigned int>(chain.size());
  for(unsigned int level = 1; level < map->levels; ++level)
  {
    const unsigned int size = map->size(level);
    const float roughness = static_cast<float>(level) / static_cast<float>(map->levels - 1);
    const float alpha = std::max(roughness * roughness, 1e-3f);
    std::vector<std::future<void>> jobs;
    std::array<Image, 6> filtered;
    for(int s = 0; s < 6; ++s)
      jobs.push_back(std::async(std::launch::async, [&, s]
      {
        filtered[s].resize(static_cast<std::size_t>(size) * size);
        for(unsigned int y = 0; y < size; ++y)
          for(unsigned int x = 0; x < size; ++x)
          {
            const Vector3f n = direction(s, 2.f * (static_cast<float>(x) + 0.5f) / static_cast<float>(size) - 1.f, 2.f * (static_cast<float>(y) + 0.5f) / static_cast<float>(size) - 1.f).normalized();
            const Vector3f up = std::abs(n.z()) < 0.999f ? Vector3f::UnitZ() : Vector3f::UnitX();
            const Vector3f tangent = up.cross(n).normalized();
            const Vector3f bitangent = n.cross(tangent);
            Vector3f sum = Vector3f::Zero();
            float weight = 0.f;
            for(unsigned int i = 0; i < numOfSamples; ++i)
            {
              const float u1 = (static_cast<float>(i) + 0.5f) / static_cast<float>(numOfSamples);
              const float u2 = radicalInverse(i);
              // A half vector of the GGX distribution.
              const float cosTheta = std::sqrt((1.f - u1) / (1.f + (alpha * alpha - 1.f) * u1));
              const float sinTheta = std::sqrt(std::max(1.f - cosTheta * cosTheta, 0.f));
              const float phi = 2.f * pi * u2;
              const Vector3f h = tangent * (sinTheta * std::cos(phi)) + bitangent * (sinTheta * std::sin(phi)) + n * cosTheta;
              const Vector3f l = 2.f * n.dot(h) * h - n;
              const float nDotL = n.dot(l);
              if(nDotL <= 0.f)
                continue;
              const float denominator = cosTheta * cosTheta * (alpha * alpha - 1.f) + 1.f;
              const float distribution = alpha * alpha / (pi * denominator * denominator);
              const float pdf = distribution / 4.f; // D * (n . h) / (4 * (v . h)) with n = v
              const float solidAngleOfSample = 1.f / (static_cast<float>(numOfSamples) * pdf + 1e-4f);
              const float chainLevel = std::clamp(0.5f * std::log2(solidAngleOfSample / solidAngleOfPixel) + 1.f, 0.f, static_cast<float>(numOfChainLevels - 1));
              const unsigned int lower = static_cast<unsigned int>(chainLevel);
              const unsigned int upper = std::min(lower + 1, numOfChainLevels - 1);
              const float fraction = chainLevel - static_cast<float>(lower);
              const Vector3f radiance = sampleLevel(chain, n0, lower, l) * (1.f - fraction) + sampleLevel(chain, n0, upper, l) * fraction;
              sum += radiance * nDotL;
              weight += nDotL;
            }
            filtered[s][y * size + x] = weight > 0.f ? Vector3f(sum / weight) : Vector3f(chain[0][s][0]);
          }
      }));
    for(auto& job : jobs)
      job.get();
    for(int s = 0; s < 6; ++s)
      store(level, s, filtered[s]);
  }

  // The spherical harmonics of the radiance (from a small level of the chain) convolved with the cosine lobe.
  {
    const unsigned int chainLevel = std::min(numOfChainLevels - 1, static_cast<unsigned int>(std::max(0, static_cast<int>(std::log2(static_cast<float>(n0))) - 5))); // 32 x 32 pixels per side
    const unsigned int size = n0 >> chainLevel;
    std::array<Vector3f, 9> coefficients;
    coefficients.fill(Vector3f::Zero());
    for(int s = 0; s < 6; ++s)
      for(unsigned int y = 0; y < size; ++y)
        for(unsigned int x = 0; x < size; ++x)
        {
          const float sc = 2.f * (static_cast<float>(x) + 0.5f) / static_cast<float>(size) - 1.f;
          const float tc = 2.f * (static_cast<float>(y) + 0.5f) / static_cast<float>(size) - 1.f;
          // The solid angle of a pixel of a cube side.
          const float solidAngle = 4.f / (static_cast<float>(size) * static_cast<float>(size) * std::pow(1.f + sc * sc + tc * tc, 1.5f));
          const std::array<float, 9> b = basis(direction(s, sc, tc).normalized());
          for(int i = 0; i < 9; ++i)
            coefficients[i] += chain[chainLevel][s][y * size + x] * (b[i] * solidAngle);
        }
    // The convolution with the clamped cosine lobe (the irradiance) and the division by pi (for the Lambertian BRDF).
    constexpr float lobe[3] = {1.f, 2.f / 3.f, 0.25f};
    for(int i = 0; i < 9; ++i)
      map->irradiance[i] = coefficients[i] * lobe[i == 0 ? 0 : i < 4 ? 1 : 2];
  }
  return map;
}
