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
  sceneIcon(":/Icons/bricks.png"), objectIcon(":/Icons/brick.png"), sensorIcon(":/Icons/transmit_go.png"), actuatorIcon(":/Icons/arrow_rotate_clockwise.png"),
  hingeIcon(":/Icons/link.png"), sliderIcon(":/Icons/slider.png"), appearanceIcon(":/Icons/note.png")
{
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

  class RendererLabel : public QLabel, public SimRobot::StatusLabel
  {
    int lastRenderingMethod = -1;
    QWidget* getWidget() override {return this;}
    void update() override
    {
      int renderingMethod = Simulation::simulation->renderer.getRenderingMethod();
      if(renderingMethod != lastRenderingMethod)
      {
        lastRenderingMethod = renderingMethod;
        static const char* renderingMethods[] =
        {
          "unkn renderer", "pbuf renderer", "fbuf renderer", "hwin renderer"
        };
        setText(renderingMethods[(renderingMethod < 0 || renderingMethod >= static_cast<int>(sizeof(renderingMethods) / sizeof(*renderingMethods))) ? 0 : renderingMethod]);
      }
    }
  };

  application->addStatusLabel(*this, new StepsLabel());
  application->addStatusLabel(*this, new StepsPerSecondLabel());
  application->addStatusLabel(*this, new CollisionsLabel());
  application->addStatusLabel(*this, new RendererLabel());

  // suggest further modules
  application->registerModule(*this, "File Editor", "SimRobotEditor", SimRobot::Flag::ignoreReset);

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
