/**
 * @file Scene.cpp
 *
 * This file implements a class that represents a scene.
 *
 * @author Arne Hasselbring
 */

#include "Scene.h"
#include "Simulation/Body.h"
#include "Simulation/Simulation.h"
#include "CoreModule.h"
#include <QPainter>

void Scene::createPhysics()
{
  if(!background.empty())
    backgroundRenderer.load(QString::fromStdString(background));
  ::PhysicalObject::createPhysics();
}

void Scene::drawPhysics(QPainter& painter) const
{
  if(!background.empty())
  {
    painter.save();
    const QRectF viewBox = backgroundRenderer.viewBoxF();
    painter.setTransform(QTransform::fromTranslate(-viewBox.width() / 2, -viewBox.height() / 2).
                         scale(viewBox.width() / painter.device()->width(), viewBox.height() / painter.device()->height()),
                         true);
    backgroundRenderer.render(&painter);
    painter.restore();
  }
  for(const Body* body : bodies)
    body->drawPhysics(painter);
  ::PhysicalObject::drawPhysics(painter);
}

void Scene::updateTransformations()
{
  for(Body* body : bodies)
    body->updateTransformation();
}

const QString& Scene::getFullName() const
{
  return SimObject::getFullName();
}

const QIcon* Scene::getIcon() const
{
  return &CoreModule::module->sceneIcon;
}

SimRobot::Widget* Scene::createWidget()
{
  return SimObject::createWidget();
}

SimRobotCore2D::Painter* Scene::createPainter()
{
  return SimObject::createPainter();
}

SimRobotCore2D::Body* Scene::getParentBody() const
{
  return parentBody;
}

double Scene::getStepLength() const
{
  return stepLength;
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
