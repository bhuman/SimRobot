/**
 * @file CoreModule.cpp
 *
 * This file implements the main class of the SimRobot 2D core.
 *
 * @author Arne Hasselbring
 */

#include "CoreModule.h"
#include "Simulation/Scene.h"
#include <QDir>
#include <QFileInfo>
#include <QLabel>
#include <QString>
#include <limits>

extern "C" DLL_EXPORT SimRobot::Module* createModule(SimRobot::Application& simRobot)
{
  return new CoreModule(simRobot);
}

SimRobot::Application* CoreModule::application = nullptr;
CoreModule* CoreModule::module = nullptr;

CoreModule::CoreModule(SimRobot::Application& application) :
  sceneIcon(":/Icons/bricks.png"),
  objectIcon(":/Icons/brick.png")
{
  CoreModule::application = &application;
  CoreModule::module = this;
}

bool CoreModule::compile()
{
  Q_ASSERT(!scene);

  // Switch to the directory containing the scene description file.
  QString filePath = application->getFilePath();
  QDir::setCurrent(QFileInfo(filePath).dir().path());

  // Load simulation and show display errors to the user.
  std::list<std::string> errors;
  if(!loadFile(filePath.toUtf8().constData(), errors))
  {
    QString errorMessage;
    for(const std::string& error : errors)
    {
      if(!errorMessage.isEmpty())
        errorMessage += "\n";
      errorMessage += QString::fromStdString(error);
    }
    application->showWarning(QObject::tr("SimRobotCore2D"), errorMessage);
    return false;
  }

  // Register scene graph objects.
  registerObjects();

  // Register status bar labels.
  class StepsLabel : public QLabel, public SimRobot::StatusLabel
  {
    QWidget* getWidget() override
    {
      return this;
    }

    void update() override
    {
      const unsigned int step = Simulation::simulation->simulationStep;
      if(step != lastStep)
      {
        lastStep = step;
        setText(QString("%1 steps").arg(step));
      }
    }

    unsigned int lastStep = std::numeric_limits<unsigned int>::max(); /**< The step when the label was updated. */
  };

  class StepsPerSecondLabel : public QLabel, public SimRobot::StatusLabel
  {
    QWidget* getWidget() override
    {
      return this;
    }

    void update() override
    {
      const int fps = static_cast<int>(Simulation::simulation->currentFrameRate);
      if(fps != lastFPS)
      {
        lastFPS = fps;
        setText(QString("%1 steps/s").arg(fps));
      }
    }

    int lastFPS = -1; /**< The frame rate when the label was updated. */
  };

  class CollisionsLabel : public QLabel, public SimRobot::StatusLabel
  {
    QWidget* getWidget() override
    {
      return this;
    }

    void update() override
    {
      const int collisions = static_cast<int>(Simulation::simulation->collisions);
      if(collisions != lastCollisions)
      {
        lastCollisions = collisions;
        setText(QString("%1 collisions").arg(collisions));
      }
    }

    int lastCollisions = -1; /**< The number of collisions when the label was updated. */
  };

  application->addStatusLabel(*this, new StepsLabel);
  application->addStatusLabel(*this, new StepsPerSecondLabel);
  application->addStatusLabel(*this, new CollisionsLabel);

  // Suggest further modules.
  application->registerModule(*this, "File Editor", "SimRobotEditor", SimRobot::Flag::ignoreReset);

  // Load the controller.
  if(!scene->controller.empty())
    application->loadModule(QString::fromStdString(scene->controller));

  return true;
}

void CoreModule::update()
{
  doSimulationStep();
}
