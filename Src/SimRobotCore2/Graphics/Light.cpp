/**
 * @file Light.h
 *
 * This file implements scene light elements.
 *
 * @author Colin Graf
 * @author Arne Hasselbring
 */

#include "Light.h"
#include "Platform/Assert.h"
#include "Simulation/Scene.h"

Light::Light()
{
  diffuseColor[0] = diffuseColor[1] = diffuseColor[2] = diffuseColor[3] = 1.f;
  ambientColor[0] = ambientColor[1] = ambientColor[2] = 0.f;
  ambientColor[3] = 1.f;
  specularColor[0] = specularColor[1] = specularColor[2] = specularColor[3] = 1.f;
}

void Light::addParent(Element& element)
{
  Scene* scene = dynamic_cast<Scene*>(&element);
  ASSERT(scene);
  scene->lights.push_back(this);
}

DirLight::DirLight()
{
  direction[0] = direction[1] = 0.f;
  direction[2] = 1.f;
}

PointLight::PointLight()
{
  position[0] = position[1] = position[2] = 0.f;
}

SpotLight::SpotLight()
{
  direction[0] = direction[1] = 0.f;
  direction[2] = 1.f;
}
