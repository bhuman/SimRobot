/**
 * @file Simulation/Scene.h
 * Implementation of class Scene
 * @author Colin Graf
 */

#include "Scene.h"
#include "CoreModule.h"
#include "Graphics/Environment.h"
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

  // A scene without an environment has one with the default values (a black background and a dark gray ambient light).
  // Elements register themselves in the simulation, which owns them, so the object cannot be a temporary one.
  if(!environment)
    environment = new Environment();
  graphicsContext.setEnvironment(*environment);

  for(Body* body : bodies)
    body->createGraphics(graphicsContext);
  GraphicalObject::createGraphics(graphicsContext);
}

void Scene::recordAppearances(GraphicsContext& graphicsContext) const
{
  for(const Body* body : bodies)
    body->recordAppearances(graphicsContext);
  GraphicalObject::recordAppearances(graphicsContext);
}

void Scene::recordPhysics(GraphicsContext& graphicsContext, unsigned int parts) const
{
  for(const Body* body : bodies)
    body->recordPhysics(graphicsContext, parts);
  ::PhysicalObject::recordPhysics(graphicsContext, parts);
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
