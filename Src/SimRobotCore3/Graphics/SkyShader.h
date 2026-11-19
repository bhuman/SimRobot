/**
 * @file Graphics/SkyShader.h
 *
 * This file contains the code of the shader that draws the sky (a color for each viewing direction: a gradient, the sun, the
 * moon, clouds, stars and polar lights) and of the ground and the line of trees at the horizon, which is also used by the
 * shader that draws the backdrop. It is the same for all graphics APIs: it is written in the Metal Shading Language, and the
 * header of the GLSL version defines the type names and functions that differ. The values that it needs are 19 vectors:
 *   0: direction to the sun (xyz), day factor (w); for a backdrop: half of the size of the room in x and y (xy), its height (z)
 *      and the height of the point from which its cube map was computed (w)
 *   1: direction to the moon (xyz), angular radius of the moon in radians (w)
 *   2: color of the sunlight (xyz), strength of the twilight glow (w)
 *   3: night factor (x), visibility of polar lights (y), time in seconds (z), exposure (w)
 *   4: horizontal direction of polar lights (xyz)
 *   5 to 7: the rows of the rotation matrix from directions in the scene to directions in the frame of the stars
 *   8 to 10: the rows of the rotation matrix from view space to the scene
 *   11: the elements (0, 0) and (1, 1) of the projection matrix (xy)
 *   12: the color of the ground (linear, xyz) and its height (w)
 *   13: the light that a horizontal surface of the ground gets (xyz, as radiance for an albedo of 1) and the size of its tiles (w)
 *   14: the position of the camera (xyz) and whether there is a ground (w, 0 or 1)
 *   15: the distance of the line of trees (x, 0 if there is none), its mean height (y), the coverage with clouds (z) and the
 *       width of the joints between the tiles of the ground relative to the size of the tiles (w)
 *   16: the color of the light of the sky divided by pi (xyz), how much foliage the trees have (w, 0 in the winter)
 *   17: the direction to the light of the sky (xyz), how autumnal the foliage is (w)
 *   18: the color of the ambient light from above (xyz)
 *
 * @author Arne Hasselbring
 */

#pragma once

#include "Graphics/StarCatalog.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <string>

/** The code of an array of vectors in both shading languages (one element for each star of the catalog). */
namespace SkyShaderPrivate
{
  template<typename F> std::string array(bool glsl, const char* type, const char* name, F value)
  {
    constexpr std::size_t count = sizeof(StarCatalog::stars) / sizeof(StarCatalog::stars[0]);
    std::string code = glsl ? std::string("const ") + type + " " + name + "[" + std::to_string(count) + "] = " + type + "[" + std::to_string(count) + "](" :
                       std::string("constant ") + type + " " + name + "[" + std::to_string(count) + "] = {";
    for(std::size_t i = 0; i < count; ++i)
      code += (i ? ", " : "") + value(StarCatalog::stars[i]);
    return code + (glsl ? ");\n" : "};\n");
  }
}

static const char* skyShaderSharedCode = R"sky(
struct SkyParams
{
  float4 v[19];
};

float skyHash(float3 p)
{
  p = fract(p * 0.1031);
  p += dot(p, p.zyx + 31.32);
  return fract((p.x + p.y) * p.z);
}

float skyNoise(float2 p)
{
  float2 i = floor(p);
  float2 f = fract(p);
  f = f * f * (3.0 - 2.0 * f);
  float a = skyHash(float3(i, 0.0));
  float b = skyHash(float3(i + float2(1.0, 0.0), 0.0));
  float c = skyHash(float3(i + float2(0.0, 1.0), 0.0));
  float d = skyHash(float3(i + float2(1.0, 1.0), 0.0));
  return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

float skyFbm(float2 p, int octaves)
{
  float sum = 0.0;
  float amplitude = 0.5;
  for(int i = 0; i < octaves; ++i)
  {
    sum += amplitude * skyNoise(p);
    p = p * 2.03 + float2(17.3, 9.1);
    amplitude *= 0.5;
  }
  return sum;
}

// The stars: a point at a random position in some of the cells of a grid that covers the sphere (for the faint ones).
float3 skyStars(float3 direction)
{
  float3 p = direction * 55.0;
  float3 cell = floor(p);
  float3 local = fract(p) - 0.5;
  float3 offset = (float3(skyHash(cell + 1.7), skyHash(cell + 3.1), skyHash(cell + 5.9)) - 0.5) * 0.5;
  float present = step(0.88, skyHash(cell));
  float brightness = pow(skyHash(cell + 9.3), 3.0);
  float size = 0.11 + 0.05 * brightness;
  float3 tint = mix(float3(1.0, 0.82, 0.65), float3(0.7, 0.8, 1.0), skyHash(cell + 2.2));
  return tint * present * (0.012 + 0.06 * brightness) * (1.0 - smoothstep(0.0, size, length(local - offset)));
}

// The color of a star from its color index B-V.
float3 skyStarTint(float colorIndex)
{
  float t = clamp(colorIndex, -0.3, 1.8);
  return t < 0.6 ? mix(float3(0.65, 0.78, 1.0), float3(1.0, 0.96, 0.88), (t + 0.3) / 0.9) : mix(float3(1.0, 0.96, 0.88), float3(1.0, 0.6, 0.35), (t - 0.6) / 1.2);
}

// The stars of the catalog: Gaussians whose size is that of a pixel for the bright ones, so that they do not flicker.
float3 skyCatalogStars(float3 direction, float pixel)
{
  float sigma = max(0.0009, 0.65 * pixel);
  float sigma2 = sigma * sigma;
  float3 sum = float3(0.0);
  for(int i = 0; i < skyStarCount; ++i)
  {
    float3 difference = direction - skyStarCatalog[i].xyz;
    float x = 0.5 * dot(difference, difference); // 1 - cos(angle)
    if(x < 60.0 * sigma2)
    {
      float flux = skyStarCatalog[i].w;
      float core = exp(-x / sigma2) * (0.0000008 / sigma2);
      float halo = flux > 0.8 ? 0.01 * exp(-x / (40.0 * sigma2)) : 0.0;
      sum += skyStarTint(skyStarColors[i]) * flux * (core + halo);
    }
  }
  return sum;
}

// The Milky Way: a band around the galactic plane that is brighter towards the center of the galaxy and has dark lanes.
float3 skyMilkyWay(float3 direction)
{
  const float3 pole = float3(-0.8677, -0.1981, 0.4560);
  const float3 center = float3(-0.0565, -0.8737, -0.4838);
  float3 second = cross(pole, center);
  float latitude = asin(clamp(dot(direction, pole), -1.0, 1.0));
  float longitude = atan2(dot(direction, second), dot(direction, center));
  float band = exp(-pow(latitude / 0.16, 2.0));
  float clouds = 0.35 + 0.9 * skyFbm(float2(longitude * 7.0, latitude * 22.0), 4);
  float dark = smoothstep(0.35, 0.6, skyFbm(float2(longitude * 12.0 + 3.0, latitude * 40.0), 3)) * exp(-pow(latitude / 0.06, 2.0));
  float brightness = 0.45 + 0.55 * max(dot(direction, center), 0.0);
  float3 color = mix(float3(0.5, 0.58, 0.8), float3(0.95, 0.85, 0.7), exp(-pow(latitude / 0.07, 2.0)) * max(dot(direction, center), 0.0));
  return color * band * clouds * brightness * (1.0 - 0.7 * dark) * 0.010;
}

// The gradient from the zenith to the horizon, the glow of the sun, the haze around it and its disc.
float3 skyBase(float3 direction, SkyParams p)
{
  float3 sunDirection = p.v[0].xyz;
  float day = p.v[0].w;
  float3 sunColor = p.v[2].xyz;
  float twilight = p.v[2].w;
  float up = clamp(direction.z, -1.0, 1.0);
  float3 zenith = mix(float3(0.001, 0.002, 0.006), float3(0.45, 0.85, 1.9), day);
  float3 horizon = mix(float3(0.003, 0.005, 0.012), float3(1.25, 1.55, 2.0), day);
  float3 color = mix(zenith, horizon, pow(1.0 - max(up, 0.0), 2.5));
  float cosSun = dot(direction, sunDirection);
  // Below a closed layer of clouds, the sky is gray and the sun is not seen.
  float overcast = smoothstep(0.5, 1.0, p.v[15].z);
  color += (1.0 - overcast) * float3(1.7, 0.55, 0.15) * twilight * pow(max(0.5 + 0.5 * cosSun, 0.0), 8.0) * pow(1.0 - max(up, 0.0), 4.0) * 1.5;
  float sunAngle = acos(clamp(cosSun, -1.0, 1.0));
  color += (1.0 - overcast) * sunColor * day * 0.6 / (1.0 + 500.0 * sunAngle * sunAngle);
  color += (1.0 - overcast) * sunColor * 40.0 * (1.0 - smoothstep(0.012, 0.018, sunAngle)) * smoothstep(-0.02, 0.02, sunDirection.z);
  float gray = dot(color, float3(0.3, 0.6, 0.1));
  return mix(color, float3(gray * 0.7), 0.8 * overcast);
}

// The clouds: x is how opaque they are, y how brightly they are lit by the sun.
float2 skyClouds(float3 direction, SkyParams p)
{
  float coverage = p.v[15].z;
  float time = p.v[3].z;
  float up = max(direction.z, 0.0);
  // The clouds are in a plane above the viewer, which is moved by the wind.
  float2 uv = direction.xy / (up + 0.1) * 2.0 + float2(time * 0.0030, time * 0.0011);
  float2 towardsSun = p.v[0].xy / (max(p.v[0].z, 0.0) + 0.35);
  // Above a coverage of about one half, the clouds close up: at 1 the sky is a layer of gray clouds with dark and light areas.
  float overcast = smoothstep(0.5, 1.0, coverage);
  float threshold = mix(0.78, 0.34, coverage) - 0.36 * overcast;
  float base = skyFbm(uv, 5);
  float density = smoothstep(threshold, threshold + 0.16, base);
  float behind = smoothstep(threshold, threshold + 0.16, skyFbm(uv + towardsSun * 0.08, 5));
  float thickness = smoothstep(threshold + 0.05, threshold + 0.35, base);
  float lit = clamp(1.0 + 2.0 * (density - behind), 0.15, 1.0) * (1.0 - 0.5 * thickness);
  lit = mix(lit, 0.1 + 0.7 * (1.0 - smoothstep(0.1, 0.9, base)), overcast);
  return float2(mix(density, 1.0, 0.85 * overcast) * smoothstep(0.0, 0.12, up), lit);
}

// What is needed to draw the ground at a viewing direction (which uses derivatives, so it must not be in a divergent branch).
struct GroundSample
{
  float2 tile; // the position in the unit of the tiles
  float2 footprint; // how much it changes between two pixels
  float t; // the distance from the camera
};

GroundSample groundSample(float3 direction, SkyParams p)
{
  GroundSample result;
  float3 cameraPosition = p.v[14].xyz;
  result.t = (p.v[12].w - cameraPosition.z) / min(direction.z, -1e-4);
  float2 position = cameraPosition.xy + direction.xy * result.t;
  result.tile = position / max(p.v[13].w, 1e-3);
  result.footprint = fwidth(result.tile);
  return result;
}

// The color of the ground (the radiance, without haze).
float3 groundColor(GroundSample g, SkyParams p)
{
  float3 albedo = p.v[12].xyz;
  if(p.v[13].w > 0.0)
  {
    // The joints between the tiles, which fade into their mean as they get smaller than a pixel.
    float jointWidth = p.v[15].w;
    float2 toJoint = abs(fract(g.tile + 0.5) - 0.5);
    float2 filterWidth = g.footprint * 0.75;
    float2 coverage = 1.0 - smoothstep(0.5 * jointWidth - filterWidth, 0.5 * jointWidth + filterWidth, toJoint);
    coverage = mix(coverage, float2(jointWidth), smoothstep(float2(0.25), float2(0.5), filterWidth));
    albedo *= 1.0 - 0.45 * (1.0 - (1.0 - coverage.x) * (1.0 - coverage.y));
  }
  return albedo * p.v[13].xyz;
}

float3 skyColor(float3 direction, SkyParams p)
{
  float3 sunDirection = p.v[0].xyz;
  float day = p.v[0].w;
  float3 moonDirection = p.v[1].xyz;
  float moonRadius = p.v[1].w;
  float3 sunColor = p.v[2].xyz;
  float twilight = p.v[2].w;
  float night = p.v[3].x;
  float aurora = p.v[3].y;
  float time = p.v[3].z;
  float3 auroraDirection = p.v[4].xyz;
  float up = clamp(direction.z, -1.0, 1.0);
  bool hasGround = p.v[14].w > 0.5;
  float3 cameraPosition = p.v[14].xyz;

  // These use derivatives, so they are computed for all pixels.
  float pixel = length(fwidth(direction));
  GroundSample ground = groundSample(direction, p);
  float treeDistance = p.v[15].x;
  float3 treeHit = float3(0.0);
  float treeT = 0.0;
  float treeFootprint = 0.0;
  if(treeDistance > 0.0)
  {
    // The line of trees is the wall of a cylinder around the origin (the camera is inside).
    float a = max(dot(direction.xy, direction.xy), 1e-6);
    float b = dot(cameraPosition.xy, direction.xy);
    float c = dot(cameraPosition.xy, cameraPosition.xy) - treeDistance * treeDistance;
    treeT = (-b + sqrt(max(b * b - a * c, 0.0))) / a;
    treeHit = cameraPosition + direction * treeT;
    treeFootprint = max(fwidth(treeHit.z), 1e-3);
  }

  float3 horizon = mix(float3(0.003, 0.005, 0.012), float3(1.25, 1.55, 2.0), day);
  float3 color = skyBase(direction, p);

  if(up >= 0.0 || !hasGround)
  {
    // The moon: a sphere that is lit by the sun.
    float moonAngle = acos(clamp(dot(direction, moonDirection), -1.0, 1.0));
    if(moonAngle < moonRadius * 1.1)
    {
      float3 helper = abs(moonDirection.z) > 0.99 ? float3(1.0, 0.0, 0.0) : float3(0.0, 0.0, 1.0);
      float3 right = normalize(cross(moonDirection, helper));
      float3 upward = cross(right, moonDirection);
      float2 q = float2(dot(direction, right), dot(direction, upward)) / moonRadius;
      float r2 = dot(q, q);
      float3 normal = float3(q, sqrt(max(1.0 - r2, 0.0)));
      float3 toSun = float3(dot(sunDirection, right), dot(sunDirection, upward), -dot(sunDirection, moonDirection));
      float lit = max(dot(normal, normalize(toSun)), 0.0);
      float surface = 0.7 + 0.3 * skyNoise(q * 3.0 + 7.0) + 0.15 * skyNoise(q * 9.0);
      float3 moon = float3(1.0, 0.96, 0.88) * (0.9 * lit + 0.03) * surface * 6.0;
      float edge = 1.0 - smoothstep(0.9, 1.0, sqrt(r2));
      color = mix(color, moon, edge * (1.0 - 0.85 * day));
    }

    // The stars and the polar lights are only visible at night above the horizon.
    float visible = night * smoothstep(-0.02, 0.06, up);
    if(visible > 0.0)
    {
      // The rows of the rotation matrix are given.
      float3 starDirection = float3(dot(p.v[5].xyz, direction), dot(p.v[6].xyz, direction), dot(p.v[7].xyz, direction));
      float extinction = 1.0 - 0.6 * smoothstep(0.0, 0.5, 0.5 - up);
      color += visible * (skyStars(starDirection) + skyCatalogStars(starDirection, pixel) + skyMilkyWay(starDirection)) * extinction;
      if(aurora > 0.0)
      {
        float2 horizontal = direction.xy;
        float azimuth = atan2(dot(horizontal, float2(-auroraDirection.y, auroraDirection.x)), dot(horizontal, auroraDirection.xy));
        float elevation = asin(max(up, 0.0));
        float band = exp(-azimuth * azimuth);
        float ray = 0.6 * skyNoise(float2(azimuth * 10.0 + time * 0.03, time * 0.05)) + 0.6 * skyNoise(float2(azimuth * 60.0, elevation * 3.0 + time * 0.15));
        float curtain = smoothstep(0.55, 1.0, ray);
        float profile = smoothstep(0.03, 0.15, elevation) * exp(-pow((elevation - 0.35) / 0.28, 2.0));
        float3 auroraColor = mix(float3(0.15, 1.0, 0.35), float3(0.7, 0.2, 0.8), smoothstep(0.35, 0.8, elevation));
        color += auroraColor * curtain * profile * band * aurora * visible * 0.12;
      }
    }

    // The clouds hide everything behind them: the sun lights them (reddish when it is low), at night there is only a little light.
    if(p.v[15].z > 0.0 && up > 0.0)
    {
      float2 cloud = skyClouds(direction, p);
      if(cloud.x > 0.0)
      {
        float sunAbove = smoothstep(-0.12, 0.08, sunDirection.z);
        float moonPhase = 0.5 * (1.0 - dot(sunDirection, moonDirection));
        float3 ambient = float3(0.55, 0.63, 0.8) * day + float3(0.0015, 0.002, 0.004) + 0.5 * twilight * float3(0.25, 0.1, 0.05)
                       + float3(0.75, 0.85, 1.0) * 0.05 * moonPhase * smoothstep(0.0, 0.15, moonDirection.z);
        float overcast = smoothstep(0.5, 1.0, p.v[15].z);
        float3 cloudColor = mix(ambient * 0.8, ambient + sunColor * 2.6 * sunAbove * (1.0 - 0.85 * overcast), cloud.y);
        // Thick clouds are gray (the sunlight is scattered in them) and dark.
        cloudColor = mix(cloudColor, float3(dot(cloudColor, float3(0.3, 0.6, 0.1))) * mix(1.0, 0.65, overcast), 0.75 * overcast);
        color = mix(color, cloudColor, cloud.x * mix(0.93, 1.0, overcast));
      }
    }
  }
  if(!hasGround)
  {
    // Below the horizon.
    return mix(horizon * 0.25, color, smoothstep(-0.12, 0.0, up));
  }

  if(up < 0.0)
  {
    // The ground fades into the haze at the horizon.
    float2 towardsHorizon = direction.xy / max(length(direction.xy), 1e-4);
    color = mix(groundColor(ground, p), skyBase(float3(towardsHorizon, 0.0), p), 1.0 - exp(-ground.t / 2500.0));
  }

  if(treeDistance > 0.0)
  {
    // The crowns of the trees: rounded, each with its own height and width (two rows of them, the nearer one is lower).
    float arc = atan2(treeHit.y, treeHit.x) * treeDistance;
    float groundHeight = p.v[12].w;
    float cellA = floor(arc / 7.0);
    float localA = fract(arc / 7.0) - 0.5;
    float randomA = skyHash(float3(cellA, 4.1, 2.7));
    float cellB = floor(arc / 4.3 + 0.5);
    float localB = fract(arc / 4.3 + 0.5) - 0.5;
    float randomB = skyHash(float3(cellB, 8.3, 1.9));
    float crownA = p.v[15].y * (0.5 + 0.7 * randomA) * (0.7 + 0.3 * sqrt(max(1.0 - 4.0 * localA * localA, 0.0)));
    float crownB = p.v[15].y * (0.35 + 0.5 * randomB) * (0.7 + 0.3 * sqrt(max(1.0 - 4.0 * localB * localB, 0.0)));
    // The edge of a crown is made of clumps of leaves.
    float ragged = 1.1 * (skyNoise(float2(arc * 0.9, 3.3)) - 0.5) + 0.5 * (skyNoise(float2(arc * 3.1, 5.9)) - 0.5);
    float top = groundHeight + max(crownA, crownB) + ragged;
    float coverage = smoothstep(-treeFootprint, treeFootprint, top - treeHit.z) * smoothstep(-treeFootprint, treeFootprint, treeHit.z - groundHeight);
    if(coverage > 0.0)
    {
      float depthShade = crownA >= crownB ? 1.0 : 0.7; // the lower row is in front of the higher one
      float leaves = 0.25 + 1.1 * skyNoise(float2(arc * 1.1, treeHit.z * 1.1)) + 0.7 * skyNoise(float2(arc * 3.7, treeHit.z * 3.7)) + 0.4 * skyNoise(float2(arc * 11.0, treeHit.z * 11.0));
      float shadow = 0.35 + 0.65 * smoothstep(0.0, 0.5 * p.v[15].y, treeHit.z - groundHeight);
      // Each crown has its own shade of green, and a few trees have turned yellow.
      float random = crownA >= crownB ? randomA : randomB;
      float3 foliage = mix(float3(0.035, 0.085, 0.020), float3(0.080, 0.130, 0.040), random);
      foliage = mix(foliage, float3(0.16, 0.13, 0.03), step(0.9, random) * 0.6);
      // In the autumn the leaves turn yellow, orange and red (each tree at its own time), in the winter they are gone and only the dark branches remain.
      float leafAmount = p.v[16].w;
      float autumn = clamp(p.v[17].w * 1.6 - 0.6 * random, 0.0, 1.0);
      float3 autumnColor = mix(float3(0.28, 0.20, 0.02), float3(0.30, 0.07, 0.02), skyHash(float3(random * 40.0, 1.3, 7.7)));
      foliage = mix(foliage, autumnColor, autumn);
      foliage *= leaves * shadow * depthShade;
      float bare = 1.0 - smoothstep(0.1, 0.6, leafAmount * (0.6 + 0.8 * leaves - 0.4 * random));
      foliage = mix(foliage, float3(0.035, 0.028, 0.024) * (0.6 + leaves) * shadow, bare);
      coverage *= mix(1.0, 0.3 + 0.3 * smoothstep(0.7, 1.4, leaves), bare);
      float3 normal = float3(-treeHit.xy / treeDistance, 0.0);
      float3 light = p.v[16].xyz * max(dot(normal, p.v[17].xyz), 0.0) + p.v[18].xyz * 0.6;
      float3 treeColor = mix(foliage * light, horizon, 1.0 - exp(-treeT / 1200.0));
      color = mix(color, treeColor, coverage);
    }
  }
  return color;
}
)sky";

/**
 * Returns the code of the sky shader (the catalog of the brightest stars and the shared code).
 * @param glsl Whether the code is for OpenGL (otherwise for Metal).
 * @return The source code, which expects \c float2, \c float3 and \c float4 and the functions that are specific to a language to be defined.
 */
inline std::string skyShaderCode(bool glsl)
{
  constexpr std::size_t count = sizeof(StarCatalog::stars) / sizeof(StarCatalog::stars[0]);
  const auto format = [](const char* pattern, auto... values)
  {
    char buffer[128];
    std::snprintf(buffer, sizeof(buffer), pattern, values...);
    return std::string(buffer);
  };
  std::string code = std::string(glsl ? "const int " : "constant int ") + "skyStarCount = " + std::to_string(count) + ";\n";
  code += SkyShaderPrivate::array(glsl, "float4", "skyStarCatalog", [&](const StarCatalog::Star& star)
  {
    double x, y, z;
    star.direction(x, y, z);
    const float flux = std::min(2.2f * std::pow(10.f, -0.4f * star.magnitude), 6.f);
    return format("float4(%.6f, %.6f, %.6f, %.5f)", x, y, z, static_cast<double>(flux));
  });
  code += SkyShaderPrivate::array(glsl, "float", "skyStarColors", [&](const StarCatalog::Star& star)
  {
    return format("%.2f", static_cast<double>(star.colorIndex));
  });
  return code + skyShaderSharedCode;
}
