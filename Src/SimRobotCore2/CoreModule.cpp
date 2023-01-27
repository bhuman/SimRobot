/**
 * @file CoreModule.cpp
 * Implementation of class CoreModule
 * @author Colin Graf
 */

#include "CoreModule.h"
#include "Simulation/PhysicalObject.h"
#include "Simulation/Scene.h"
#include <QDir>
#include <QLabel>

extern "C" DLL_EXPORT SimRobot::Module* createModule(SimRobot::Application& simRobot)
{
  return new CoreModule(simRobot);
}

SimRobot::Application* CoreModule::application;
CoreModule* CoreModule::module;

CoreModule::CoreModule(SimRobot::Application& application) :
  sceneIcon(":/Icons/icons8-3d-model-50.png"), objectIcon(":/Icons/icons8-orthogonal-view-50.png"), sensorIcon(":/Icons/icons8-speed-50.png"), actuatorIcon(":/Icons/icons8-engine-50.png"),
  hingeIcon(":/Icons/icons8-link-50.png"), sliderIcon(":/Icons/icons8-slider-control-50.png"), appearanceIcon(":/Icons/icons8-octaedro-50.png")
{
  sceneIcon.setIsMask(true);
  objectIcon.setIsMask(true);
  sensorIcon.setIsMask(true);
  actuatorIcon.setIsMask(true);
  hingeIcon.setIsMask(true);
  sliderIcon.setIsMask(true);
  appearanceIcon.setIsMask(true);
  CoreModule::application = &application;
  CoreModule::module = this;
}

bool CoreModule::compile()
{
  Q_ASSERT(!scene);

  // change working directory
  QString filePath = application->getFilePath();
  QDir::setCurrent(QFileInfo(filePath).dir().path());

  // load simulation
  std::list<std::string> errors;
  if(!loadFile(filePath.toUtf8().constData(), errors))
  {
    QString errorMessage;
    for(const std::string& error : errors)
    {
      if(!errorMessage.isEmpty())
        errorMessage += "\n";
      errorMessage += error.c_str();
    }
    application->showWarning(QObject::tr("SimRobotCore2"), errorMessage);
    return false;
  }

  // register scene graph objects
  registerObjects();
  application->registerObject(*this, actuatorsObject, 0, SimRobot::Flag::hidden);

  // register status bar labels
  class StepsLabel : public QLabel, public SimRobot::StatusLabel
  {
    unsigned int lastStep = -1;
    QWidget* getWidget() override {return this;}
    void update() override
    {
      unsigned int step = Simulation::simulation->simulationStep;
      if(step != lastStep)
      {
        lastStep = step;
        char buf[33];
        sprintf(buf, "%u steps", step);
        setText(buf);
      }
    }
  };

  class StepsPerSecondLabel : public QLabel, public SimRobot::StatusLabel
  {
    int lastFps = -1;
    QWidget* getWidget() override {return this;}
    void update() override
    {
      int fps = Simulation::simulation->currentFrameRate;
      if(fps != lastFps)
      {
        lastFps = fps;
        char buf[33];
        sprintf(buf, "%u steps/s", fps);
        setText(buf);
      }
    }
  };

  class CollisionsLabel : public QLabel, public SimRobot::StatusLabel
  {
    int lastCols = -1;
    QWidget* getWidget() override {return this;}
    void update() override
    {
      int cols = Simulation::simulation->collisions;
      if(cols != lastCols)
      {
        lastCols = cols;
        char buf[33];
        sprintf(buf, "%u collisions", cols);
        setText(buf);
      }
    }
  };

  application->addStatusLabel(*this, new StepsLabel());
  application->addStatusLabel(*this, new StepsPerSecondLabel());
  application->addStatusLabel(*this, new CollisionsLabel());

  // suggest further modules
  application->registerModule(*this, "File Editor", "SimRobotEditor");

  // load controller
  if(simulation->scene->controller != "")
    application->loadModule(simulation->scene->controller.c_str());
  return true;
}

void CoreModule::update()
{
  if(ActuatorsWidget::actuatorsWidget)
    ActuatorsWidget::actuatorsWidget->adoptActuators();
  doSimulationStep();
}
