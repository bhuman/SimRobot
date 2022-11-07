/**
 * @file BenchmarkController.cpp
 *
 * This file declares a controller that only fetches a new camera image every step.
 *
 * @author Arne Hasselbring
 */

#include <SimRobot.h>
#include <SimRobotCore2.h>

class BenchmarkController : public SimRobot::Module
{
public:
  BenchmarkController(SimRobot::Application& simRobot) :
    simRobot(simRobot)
  {}

private:
  bool compile() override
  {
    camera = static_cast<SimRobotCore2::SensorPort*>(simRobot.resolveObject("RoboCup.base.camera.image", SimRobotCore2::sensorPort));
    return camera;
  }

  void update() override
  {
    camera->getValue();
  }

  SimRobot::Application& simRobot;
  SimRobotCore2::SensorPort* camera;
};

extern "C" DLL_EXPORT SimRobot::Module* createModule(SimRobot::Application& simRobot)
{
  return new BenchmarkController(simRobot);
}
