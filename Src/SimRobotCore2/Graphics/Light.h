/**
 * @file Light.h
 *
 * This file declares scene light elements.
 *
 * @author Colin Graf
 * @author Arne Hasselbring
 */

#pragma once

#include "Parser/ElementCore2.h"

/**
 * @class Light
 * A scene light definition
 */
class Light : public ElementCore2
{
public:
  float diffuseColor[4];
  float ambientColor[4];
  float specularColor[4];

  /** Default constructor */
  Light();

  /**
   * Registers an element as parent
   * @param element The element to register
   */
  void addParent(Element& element) override;
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
