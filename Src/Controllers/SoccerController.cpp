/**
 * @file SoccerController.cpp
 *
 * This file defines a controller for the Soccer scene.
 *
 * @author Arne Hasselbring
 */

#include <SimRobotCore2D.h>
#include <QString>
#include <vector>

class SoccerController : public SimRobot::Module
{
public:
  /**
   * Constructor.
   * @param simRobot The simulator instance.
   */
  explicit SoccerController(SimRobot::Application& simRobot) :
    simRobot(simRobot)
  {}

protected:
  /**
   * Initializes the controller.
   * @return Whether the initialization was successful.
   */
  bool compile() override
  {
    auto* const scene = dynamic_cast<SimRobotCore2D::Scene*>(simRobot.resolveObject(QString("RoboCup"), SimRobotCore2D::scene));
    if(!scene)
      return false;
    // Run in real time.
    simRobot.setStepInterval(scene->getStepLength());

    SimRobot::Object* const robots = simRobot.resolveObject(QString("RoboCup.robots"), SimRobotCore2D::compound);
    if(!robots)
      return false;
    for(int i = 0; i < simRobot.getObjectChildCount(*robots); ++i)
    {
      auto* const robot = dynamic_cast<SimRobotCore2D::Body*>(simRobot.getObjectChild(*robots, i));
      if(!robot)
        return false;
      this->robots.push_back(robot);
    }

    ball = dynamic_cast<SimRobotCore2D::Body*>(simRobot.resolveObject(QString("RoboCup.balls.ball"), SimRobotCore2D::body));
    if(!ball)
      return false;
    return true;
  }

  /** Performs a simulation step in the controller. */
  void update() override
  {
    // ... play soccer ...
  }

private:
  SimRobot::Application& simRobot; /**< The simulator instance. */
  std::vector<SimRobotCore2D::Body*> robots; /**< The robots in the scene. */
  SimRobotCore2D::Body* ball = nullptr; /**< The ball in the scene. */
};

extern "C" DLL_EXPORT SimRobot::Module* createModule(SimRobot::Application& simRobot)
{
  return new SoccerController(simRobot);
}
