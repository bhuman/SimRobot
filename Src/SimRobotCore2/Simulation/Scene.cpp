/**
 * @file Simulation/Scene.h
 * Implementation of class Scene
 * @author Colin Graf
 */

#include "Scene.h"
#include "CoreModule.h"
#include "Platform/Assert.h"
#include "Platform/OpenGL.h"
#include "Simulation/Actuators/Actuator.h"
#include "Simulation/Body.h"
#include "Simulation/Simulation.h"
#include "Tools/Math/Constants.h"

void Scene::updateTransformations()
{
  if(lastTransformationUpdateStep != Simulation::simulation->simulationStep)
  {
    for(Body* body : bodies)
      body->updateTransformation();
    lastTransformationUpdateStep = Simulation::simulation->simulationStep;
  }
}

void Scene::updateActuators()
{
  for(Actuator::Port* actuator : actuators)
    actuator->act();
}

void Scene::createGraphics(bool isShared)
{
  float* clearColor = Simulation::simulation->scene->color;
  glClearColor(clearColor[0], clearColor[1], clearColor[2], clearColor[3]);

  // enable depth test
  glClearDepth(1.0f);
  glDepthFunc(GL_LEQUAL);
  glEnable(GL_DEPTH_TEST);

  // avoid rendering the backside of surfaces
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glFrontFace(GL_CCW);

  //
  glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

  // setup lights
  glEnable(GL_COLOR_MATERIAL);
  GLfloat data[4];
  data[0] = 0.2f; data[1] = 0.2f; data[2] = 0.2f; data[3] = 1.f;
  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, data);
  //glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE);
  glLightModelf(GL_LIGHT_MODEL_LOCAL_VIEWER, GL_TRUE);

  int i = 0;
  for(Light* light : lights)
  {
    glLightfv(GL_LIGHT0 + i, GL_AMBIENT, light->ambientColor);
    glLightfv(GL_LIGHT0 + i, GL_DIFFUSE, light->diffuseColor);
    glLightfv(GL_LIGHT0 + i, GL_SPECULAR, light->specularColor);
    glLightfv(GL_LIGHT0 + i, GL_POSITION, light->position);
    glLightfv(GL_LIGHT0 + i, GL_CONSTANT_ATTENUATION, &light->constantAttenuation);
    glLightfv(GL_LIGHT0 + i, GL_LINEAR_ATTENUATION, &light->linearAttenuation);
    glLightfv(GL_LIGHT0 + i, GL_QUADRATIC_ATTENUATION, &light->quadraticAttenuation);
    float spotCutoff = light->spotCutoff * (180.f / pi);
    glLightfv(GL_LIGHT0 + i, GL_SPOT_CUTOFF, &spotCutoff);
    glLightfv(GL_LIGHT0 + i, GL_SPOT_DIRECTION, light->spotDirection.data());
    glLightfv(GL_LIGHT0 + i, GL_SPOT_EXPONENT, &light->spotExponent);
    glEnable(GL_LIGHT0 + i);
    ++i;
  }

  // load display lists and textures
  if(!isShared)
  {
    for(Body* body : bodies)
      body->createGraphics();
    GraphicalObject::createGraphics();

    if(!textures.empty())
    {
      for(auto& texture : textures)
        texture.second.createGraphics();
      glBindTexture(GL_TEXTURE_2D, 0);
    }
  }
}

void Scene::drawAppearances(SurfaceColor color, bool drawControllerDrawings) const
{
  for(const Body* body : bodies)
    body->drawAppearances(color, drawControllerDrawings);
  GraphicalObject::drawAppearances(color, drawControllerDrawings);
}

void Scene::drawPhysics(unsigned int flags) const
{
  for(const Body* body : bodies)
    body->drawPhysics(flags);
  ::PhysicalObject::drawPhysics(flags);
}

const QIcon* Scene::getIcon() const
{
  return &CoreModule::module->sceneIcon;
}

unsigned int Scene::getStep() const
{
  return Simulation::simulation->simulationStep;
}

double Scene::getTime() const
{
  return Simulation::simulation->simulatedTime;
}

unsigned int Scene::getFrameRate() const
{
  return Simulation::simulation->currentFrameRate;
}

Texture* Scene::loadTexture(const std::string& file)
{
  auto iter = textures.find(file);
  if(iter != textures.end())
  {
    Texture& texture = iter->second;
    return texture.imageData ? &texture : 0;
  }
  Texture& texture = textures[file];
  texture.load(file);
  return texture.imageData ? &texture : 0;
}

Scene::Light::Light()
{
  diffuseColor[0] = diffuseColor[1] = diffuseColor[2] = diffuseColor[3] = 1.f;
  ambientColor[0] = ambientColor[1] = ambientColor[2] = 0.f;
  ambientColor[3] = 1.f;
  specularColor[0] = specularColor[1] = specularColor[2] = specularColor[3] = 1.f;
  position[0] = position[1] = position[3] = 0.f;
  position[2] = 1.f;
}

void Scene::Light::addParent(Element& element)
{
  Scene* scene = dynamic_cast<Scene*>(&element);
  ASSERT(scene);
  scene->lights.push_back(this);
}
