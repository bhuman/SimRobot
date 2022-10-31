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

void Scene::createGraphics(GraphicsContext& graphicsContext)
{
  // The model matrix is needed for controller drawings.
  // Physical object and graphical object share it because it really is just at the origin.
  ASSERT(!::PhysicalObject::modelMatrix);
  ASSERT(!GraphicalObject::modelMatrix);
  GraphicalObject::modelMatrix = ::PhysicalObject::modelMatrix = graphicsContext.requestModelMatrix(GraphicsContext::ModelMatrix::controllerDrawing);

  graphicsContext.setClearColor(Simulation::simulation->scene->color);

  const float color[4] = {0.2f, 0.2f, 0.2f, 1.f};
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

void Scene::visitGraphicalControllerDrawings(const std::function<void(GraphicalObject&)>& accept)
{
  for(Body* body : bodies)
    accept(*body);
  GraphicalObject::visitGraphicalControllerDrawings(accept);
}

void Scene::visitPhysicalControllerDrawings(const std::function<void(::PhysicalObject&)>& accept)
{
  for(Body* body : bodies)
    accept(*body);
  ::PhysicalObject::visitPhysicalControllerDrawings(accept);
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

bool Scene::registerDrawingManager(SimRobotCore2::Controller3DDrawingManager& manager)
{
  if(drawingManager)
    return false;
  drawingManager = &manager;
  return true;
}
