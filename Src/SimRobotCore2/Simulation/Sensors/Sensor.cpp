/**
 * @file Simulation/Sensor.cpp
 * Implementation of class Sensor
 * @author Colin Graf
 */

#include "Sensor.h"
#include "CoreModule.h"
#include "Graphics/GraphicsContext.h"
#include "SensorWidget.h"
#include "Simulation/Simulation.h"
#include "Tools/OpenGLTools.h"

void Sensor::createPhysics(GraphicsContext& graphicsContext)
{
  OpenGLTools::convertTransformation(rotation, translation, poseInParent);

  graphicsContext.pushModelMatrix(poseInParent);
  ASSERT(!modelMatrix);
  modelMatrix = graphicsContext.requestModelMatrix(GraphicsContext::ModelMatrix::sensorDrawing);
  ::PhysicalObject::createPhysics(graphicsContext);
  graphicsContext.popModelMatrix();
}

const QIcon* Sensor::Port::getIcon() const
{
  return &CoreModule::module->sensorIcon;
}

SimRobot::Widget* Sensor::Port::createWidget()
{
  return new SensorWidget(this);
}

SimRobotCore2::SensorPort::Data Sensor::Port::getValue()
{
  if(lastSimulationStep != Simulation::simulation->simulationStep)
  {
    updateValue();
    lastSimulationStep = Simulation::simulation->simulationStep;
  }
  return data;
}
