/**
 * @file Graphics/Sky.cpp
 *
 * This file implements the sky of a scene. The positions of the sun and the moon are computed with low precision formulas
 * (an error of less than a degree), which is enough for what is seen.
 *
 * @author Arne Hasselbring
 */

#include "Sky.h"
#include "Graphics/LightSpace.h"
#include "Platform/Assert.h"
#include "Graphics/Environment.h"
#include "Simulation/Scene.h"
#include "Simulation/Simulation.h"
#include <algorithm>
#include <chrono>
#include <cmath>

namespace
{
  using Vector3d = Eigen::Vector3d;
  using Matrix3d = Eigen::Matrix3d;

  constexpr double pi = 3.14159265358979323846;

  double rad(double degrees)
  {
    return degrees * (pi / 180.0);
  }

  float smoothstep(float edge0, float edge1, float x)
  {
    const float t = std::clamp((x - edge0) / (edge1 - edge0), 0.f, 1.f);
    return t * t * (3.f - 2.f * t);
  }

  /** The Julian day number of the start of a day in the Gregorian calendar (at 0 h UT). */
  double julianDay(int year, int month, int day)
  {
    if(month <= 2)
    {
      --year;
      month += 12;
    }
    const int a = year / 100;
    const int b = 2 - a + a / 4;
    return std::floor(365.25 * (year + 4716)) + std::floor(30.6001 * (month + 1)) + day + b - 1524.5;
  }

  /** A position in the sky in equatorial coordinates (in radians). */
  struct Equatorial
  {
    double rightAscension;
    double declination;
  };

  /**
   * The position of the sun.
   * @param n The days since J2000.
   */
  Equatorial sunPosition(double n)
  {
    const double l = 280.460 + 0.9856474 * n;
    const double g = rad(357.528 + 0.9856003 * n);
    const double lambda = rad(l + 1.915 * std::sin(g) + 0.020 * std::sin(2.0 * g));
    const double epsilon = rad(23.439 - 0.0000004 * n);
    return {std::atan2(std::cos(epsilon) * std::sin(lambda), std::cos(lambda)), std::asin(std::sin(epsilon) * std::sin(lambda))};
  }

  /**
   * The position of the moon (a simplification of the method of Paul Schlyter, without the parallax).
   * @param d The days since 1999-12-31 0:00.
   */
  Equatorial moonPosition(double d)
  {
    const auto rev = [](double x) {return x - std::floor(x / 360.0) * 360.0;};
    const double n = rev(125.1228 - 0.0529538083 * d);
    const double i = rad(5.1454);
    const double w = rev(318.0634 + 0.1643573223 * d);
    const double a = 60.2666;
    const double e = 0.054900;
    const double m = rev(115.3654 + 13.0649929509 * d);
    // The mean anomaly and mean longitude of the sun for the perturbations.
    const double ms = rev(356.0470 + 0.9856002585 * d);
    const double ls = rev(282.9404 + 4.70935E-5 * d + ms);
    const double lm = rev(n + w + m);
    const double dm = rev(lm - ls);
    const double f = rev(lm - n);

    // Kepler's equation.
    double eccentricAnomaly = rad(m) + e * std::sin(rad(m)) * (1.0 + e * std::cos(rad(m)));
    for(int k = 0; k < 5; ++k)
      eccentricAnomaly -= (eccentricAnomaly - e * std::sin(eccentricAnomaly) - rad(m)) / (1.0 - e * std::cos(eccentricAnomaly));
    const double xv = a * (std::cos(eccentricAnomaly) - e);
    const double yv = a * std::sqrt(1.0 - e * e) * std::sin(eccentricAnomaly);
    const double v = std::atan2(yv, xv);
    const double r = std::sqrt(xv * xv + yv * yv);
    const double vw = v + rad(w);
    const double xh = r * (std::cos(rad(n)) * std::cos(vw) - std::sin(rad(n)) * std::sin(vw) * std::cos(i));
    const double yh = r * (std::sin(rad(n)) * std::cos(vw) + std::cos(rad(n)) * std::sin(vw) * std::cos(i));
    const double zh = r * std::sin(vw) * std::sin(i);
    double longitude = std::atan2(yh, xh) * (180.0 / pi);
    double latitude = std::atan2(zh, std::sqrt(xh * xh + yh * yh)) * (180.0 / pi);
    longitude += -1.274 * std::sin(rad(m - 2.0 * dm)) + 0.658 * std::sin(rad(2.0 * dm)) - 0.186 * std::sin(rad(ms)) - 0.059 * std::sin(rad(2.0 * m - 2.0 * dm))
                 - 0.057 * std::sin(rad(m - 2.0 * dm + ms)) + 0.053 * std::sin(rad(m + 2.0 * dm)) + 0.046 * std::sin(rad(2.0 * dm - ms)) + 0.041 * std::sin(rad(m - ms))
                 - 0.035 * std::sin(rad(dm)) - 0.031 * std::sin(rad(m + ms)) - 0.015 * std::sin(rad(2.0 * f - 2.0 * dm)) + 0.011 * std::sin(rad(m - 4.0 * dm));
    latitude += -0.173 * std::sin(rad(f - 2.0 * dm)) - 0.055 * std::sin(rad(m - f - 2.0 * dm)) - 0.046 * std::sin(rad(m + f - 2.0 * dm))
                + 0.033 * std::sin(rad(f + 2.0 * dm)) + 0.017 * std::sin(rad(2.0 * m + f));

    // From ecliptic to equatorial coordinates.
    const double epsilon = rad(23.4393 - 3.563E-7 * d);
    const double x = std::cos(rad(longitude)) * std::cos(rad(latitude));
    const double y = std::sin(rad(longitude)) * std::cos(rad(latitude));
    const double z = std::sin(rad(latitude));
    return {std::atan2(y * std::cos(epsilon) - z * std::sin(epsilon), x), std::asin(y * std::sin(epsilon) + z * std::cos(epsilon))};
  }

  /**
   * Converts a position in the sky to a direction in the horizontal frame (east, north, up).
   * @param position The position.
   * @param localSiderealTime The local sidereal time as an angle in radians.
   * @param latitude The latitude in radians.
   */
  Vector3d horizontal(const Equatorial& position, double localSiderealTime, double latitude)
  {
    const double hourAngle = localSiderealTime - position.rightAscension;
    return Vector3d(-std::cos(position.declination) * std::sin(hourAngle),
                    std::sin(position.declination) * std::cos(latitude) - std::cos(position.declination) * std::cos(hourAngle) * std::sin(latitude),
                    std::sin(position.declination) * std::sin(latitude) + std::cos(position.declination) * std::cos(hourAngle) * std::cos(latitude));
  }

  float luminance(const Vector3f& color)
  {
    return 0.2126f * color.x() + 0.7152f * color.y() + 0.0722f * color.z();
  }
}

Sky::Sky()
{
  // The light of the sky is dynamic, so these values are replaced as soon as the sky is computed.
  color[0] = color[1] = color[2] = 0.f;
}

void Sky::addParent(Element& element)
{
  DirLight::addParent(element);
  environment = dynamic_cast<Environment*>(&element);
  ASSERT(environment);
  environment->sky = this;
}

void Sky::update(GraphicsContext& graphicsContext)
{
  // The time as a Julian date (in UT) and a time that animates things in the sky.
  double julianDate;
  double animationTime;
  if(clock == wallClock)
  {
    const double seconds = std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count();
    julianDate = 2440587.5 + seconds / 86400.0;
    animationTime = std::fmod(seconds, 3600.0);
  }
  else
  {
    const double simulationTime = Simulation::simulation->simulationStep * static_cast<double>(Simulation::simulation->scene->stepLength);
    julianDate = julianDay(year, month, day) + (startTime - longitude / 15.0 + simulationTime * timeScale / 3600.0) / 24.0;
    animationTime = simulationTime;
  }
  const double key = julianDate + animationTime * 1e-9;
  if(initialized && key == lastTime)
    return;
  initialized = true;
  lastTime = key;

  // The positions of the sun and the moon.
  const double n = julianDate - 2451545.0;
  const double siderealTime = rad(std::fmod(280.46061837 + 360.98564736629 * n, 360.0) + longitude);
  const double latitudeRad = rad(latitude);
  const Vector3d sunHorizontal = horizontal(sunPosition(n), siderealTime, latitudeRad);
  const Vector3d moonHorizontal = horizontal(moonPosition(julianDate - 2451543.5), siderealTime, latitudeRad);

  // From the horizontal frame to the scene.
  const Vector3d north(std::cos(rad(northAngle)), std::sin(rad(northAngle)), 0.0);
  const Vector3d east = north.cross(Vector3d::UnitZ());
  Matrix3d horizontalToScene;
  horizontalToScene << east, north, Vector3d::UnitZ();
  const Vector3f sunDirection = (horizontalToScene * sunHorizontal).cast<float>().normalized();
  const Vector3f moonDirection = (horizontalToScene * moonHorizontal).cast<float>().normalized();

  GraphicsContext::SkyState state;
  state.sunDirection = sunDirection;
  state.moonDirection = moonDirection;
  state.moonRadius = static_cast<float>(rad(moonRadius));
  state.time = static_cast<float>(animationTime);
  state.clouds = clouds;

  // The season (of the hemisphere) determines the foliage: the sun is highest at 0.25 of the year (the solstice in June).
  // The tropics don't have a winter, so there the trees stay green.
  {
    const double yearFraction = std::fmod(n + 0.5 - 79.0, 365.2422) / 365.2422 + (latitude < 0.0 ? 0.5 : 0.0);
    const double summer = std::sin(2.0 * pi * yearFraction); // 1 at midsummer, -1 at midwinter
    const double autumnPhase = std::cos(2.0 * pi * yearFraction); // negative after the summer
    const float seasonal = smoothstep(15.f, 40.f, static_cast<float>(std::abs(latitude)));
    state.leaves = 1.f - seasonal * (1.f - smoothstep(-0.97f, -0.6f, static_cast<float>(summer)));
    state.autumn = seasonal * (1.f - smoothstep(-0.55f, 0.35f, static_cast<float>(summer))) * (autumnPhase < 0.0 ? 1.f : 0.f);
  }

  // The brightness of the sky depends on the elevation of the sun (as its sine).
  const float s = sunDirection.z();
  const float sunVisible = smoothstep(-0.025f, 0.04f, s);
  state.day = smoothstep(-0.12f, 0.30f, s);
  state.night = 1.f - smoothstep(-0.30f, -0.06f, s);
  state.twilight = std::exp(-std::pow((s + 0.02f) / 0.10f, 2.f));

  // The sunlight is reddened by the air that it passes: the air mass (Kasten and Young) times the extinction per wavelength.
  const float elevation = static_cast<float>(std::asin(std::clamp(s, -1.f, 1.f)) * (180.0 / pi));
  const float airMass = 1.f / (std::sin(rad(std::max(elevation, 0.f))) + 0.50572f * std::pow(std::max(elevation, 0.f) + 6.07995f, -1.6364f));
  const Vector3f transmittance(std::exp(-airMass * 0.045f), std::exp(-airMass * 0.085f), std::exp(-airMass * 0.17f));
  state.sunColor = transmittance;
  // Clouds let only a part of the sunlight through (the rest of the light is scattered into the sky).
  const float cloudShade = 1.f - 0.85f * smoothstep(0.3f, 1.f, clouds);
  const Vector3f sunLight = sunIntensity * sunVisible * cloudShade * transmittance;

  // The moon: the illuminated part follows from the angle to the sun.
  const float moonPhase = 0.5f * (1.f - sunDirection.dot(moonDirection));
  const Vector3f moonLight = sunIntensity * 0.012f * moonPhase * smoothstep(-0.02f, 0.08f, moonDirection.z()) * Vector3f(0.75f, 0.85f, 1.f);

  // The light of the sky is the one of the sun or the moon, whichever is brighter.
  if(luminance(sunLight) >= luminance(moonLight))
  {
    state.lightColor = sunLight;
    state.lightDirection = -sunDirection;
  }
  else
  {
    state.lightColor = moonLight;
    state.lightDirection = -moonDirection;
  }

  // The ambient light comes from the sky (blue, red at sunrise and sunset, dark and bluish at night) and the ground.
  const float moonAmbient = moonPhase * smoothstep(-0.02f, 0.2f, moonDirection.z());
  state.ambientSky = state.day * Vector3f(0.45f, 0.55f, 0.70f) + 0.6f * state.twilight * Vector3f(0.30f, 0.10f, 0.f) + Vector3f(0.010f, 0.014f, 0.028f) + moonAmbient * Vector3f(0.020f, 0.024f, 0.034f);
  // Below clouds, a part of the sunlight arrives as scattered light from the whole sky, which is gray.
  const float overcast = smoothstep(0.3f, 1.f, clouds);
  state.ambientSky += overcast * state.day * sunIntensity * 0.06f * sunVisible * Vector3f(0.9f, 0.95f, 1.f);
  state.ambientSky = (1.f - 0.5f * overcast) * state.ambientSky + 0.5f * overcast * Vector3f::Constant(state.ambientSky.dot(Vector3f(0.3f, 0.6f, 0.1f)));
  state.ambientGround = state.day * Vector3f(0.22f, 0.30f, 0.14f) + Vector3f(0.003f, 0.004f, 0.002f);

  // Cameras and eyes adapt to the darkness, so the scene is exposed longer at night.
  state.exposure = environment->exposure * std::pow(nightExposure, 1.f - smoothstep(-0.20f, 0.10f, s));

  // Polar lights are in the sky in the direction of the pole (and only visible at night).
  state.aurora = aurora ? *aurora : smoothstep(55.f, 67.f, static_cast<float>(std::abs(latitude)));
  state.auroraDirection = (horizontalToScene * Vector3d(0.0, latitude >= 0.0 ? 1.0 : -1.0, 0.0)).cast<float>();

  // The stars are fixed in the frame of the equator and rotate with the sidereal time: This is the matrix from the scene to that frame.
  Matrix3d equatorialAxes; // The east, north and up axes of the horizon in equatorial coordinates (as columns).
  equatorialAxes << Vector3d(-std::sin(siderealTime), std::cos(siderealTime), 0.0),
                    Vector3d(-std::sin(latitudeRad) * std::cos(siderealTime), -std::sin(latitudeRad) * std::sin(siderealTime), std::cos(latitudeRad)),
                    Vector3d(std::cos(latitudeRad) * std::cos(siderealTime), std::cos(latitudeRad) * std::sin(siderealTime), std::sin(latitudeRad));
  state.stars = (equatorialAxes * horizontalToScene.transpose()).cast<float>();

  // The shadow map follows the direction of the light.
  if(shadowMap)
  {
    std::copy(state.lightDirection.data(), state.lightDirection.data() + 3, direction);
    if(const std::optional<LightSpace> lightSpace = LightSpace::create(*this))
    {
      state.lightView = lightSpace->view;
      state.lightProjection = lightSpace->projection;
    }
  }

  graphicsContext.setSky(state);
}
