/**
 * @file Simulation/Scene.h
 * Implementation of class Scene
 * @author Colin Graf
 */

#include "Scene.h"
#include "CoreModule.h"
#include "Platform/Assert.h"
#include "Simulation/Actuators/Actuator.h"
#include "Simulation/Body.h"
#include "Simulation/Simulation.h"

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

void Scene::createGraphics(GraphicsContext& graphicsContext)
{
  // The model matrix is needed for controller drawings.
  ASSERT(!::PhysicalObject::modelMatrix);
  ::PhysicalObject::modelMatrix = graphicsContext.requestModelMatrix(GraphicsContext::ModelMatrix::controllerDrawing);

  graphicsContext.setClearColor(Simulation::simulation->scene->color);

  const float color[3] = {0.4f, 0.4f, 0.4f};
  graphicsContext.setGlobalAmbientLight(color);
  for(Light* light : lights)
    graphicsContext.addLight(light);

  for(Body* body : bodies)
    body->createGraphics(graphicsContext);
  GraphicalObject::createGraphics(graphicsContext);
}

void Scene::drawAppearances(GraphicsContext& graphicsContext) const
{
  for(const Body* body : bodies)
    body->drawAppearances(graphicsContext);
  GraphicalObject::drawAppearances(graphicsContext);
}

void Scene::drawPhysics(GraphicsContext& graphicsContext, unsigned int flags) const
{
  for(const Body* body : bodies)
    body->drawPhysics(graphicsContext, flags);
  ::PhysicalObject::drawPhysics(graphicsContext, flags);
}

void Scene::collectPhysicalDrawings(GraphicsContext::DrawingList& list) const
{
  for(const Body* body : bodies)
    body->collectPhysicalDrawings(list);
  ::PhysicalObject::collectPhysicalDrawings(list);
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
