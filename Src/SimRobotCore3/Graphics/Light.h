/**
 * @file Light.h
 *
 * This file declares scene light elements.
 *
 * @author Colin Graf
 * @author Arne Hasselbring
 */

#pragma once

#include "Parser/ElementCore3.h"
#include "Tools/Math/Eigen.h"
#include <optional>

/**
 * @class Light
 * A scene light definition
 */
class Light : public ElementCore3
{
public:
  /**
   * The parameters of the shadow map of a light (enabled by a \c ShadowMap child element).
   * Only objects within the axis-aligned bounding box cast and receive shadows.
   */
  struct ShadowMap
  {
    unsigned int width = 2048; /**< The width of the shadow map in pixels. */
    unsigned int height = 2048; /**< The height of the shadow map in pixels. */
    Vector3f aabbCenter = Vector3f::Zero(); /**< The center of the bounding box (in world coordinates). */
    Vector3f aabbExtent = Vector3f::Zero(); /**< The size of the bounding box in x, y and z direction. */
    bool autoFit = false; /**< Whether the bounding box is fitted to the scene (when the shadow maps are rendered the first time) instead of being given. */
    bool fitted = false; /**< Whether the bounding box has been fitted (only if \c autoFit). */
  };

  float color[3];
  std::optional<ShadowMap> shadowMap; /**< The shadow map if the light casts shadows. */

  /** Default constructor */
  Light();

  /**
   * Registers an element as parent
   * @param element The element to register
   */
  void addParent(Element& element) override;

  /**
   * Returns whether the color and direction of the light change while the scene is rendered (see \c Sky).
   * @return Whether the light is dynamic.
   */
  virtual bool isDynamic() const {return false;}
};
/**
 * @class DirLight
 * A directional light definition
 */
class DirLight : public Light
{
public:
  float direction[3];

  /** Default constructor */
  DirLight();
};

/**
 * @class PointLight
 * A point light definition
 */
class PointLight : public Light
{
public:
  float position[3];
  float constantAttenuation = 1.f;
  float linearAttenuation = 0.f;
  float quadraticAttenuation = 0.f;

  /** Default constructor */
  PointLight();
};

/**
 * @class SpotLight
 * A spot light definition
 */
class SpotLight : public PointLight
{
public:
  float direction[3];
  float cutoff = 0.f;

  /** Default constructor */
  SpotLight();
};
