/**
 * @file Simulation/Scene.h
 * Declaration of class Scene
 * @author Colin Graf
 */

#pragma once

#include "SimRobotCore2.h"
#include "Simulation/Actuators/Actuator.h"
#include "Simulation/Appearances/Appearance.h"
#include "Simulation/GraphicalObject.h"
#include "Simulation/PhysicalObject.h"
#include "Tools/Math/Constants.h"
#include <list>
#include <string>
#include <unordered_map>

class Body;
class Light;

/**
 * @class Scene
 * Class for the root node of the scene graph
 */
class Scene : public PhysicalObject, public GraphicalObject, public SimRobotCore2::Scene
{
public:

  std::string controller; /**< The name of the controller library. */
  float color[4]; /**< The background (clear color) */
  float stepLength; /**< The length of a simulation step */
  float gravity; /**< The gravity in the simulated world */
  float erp; /**< ODE's erp parameter */
  float cfm; /**< ODE's cfm parameter */
  int contactMode = 0; /**< The default contact mode for contacts between bodies */
  float contactSoftERP;
  float contactSoftCFM;
  bool useQuickSolver = false; /**< Whether to use ODE's quick solver */
  int quickSolverIterations = -1; /**< The iteration count for ODE's quick solver */
  int quickSolverSkip; /**< Controls how often the normal solver will be used instead of the quick solver */
  bool detectBodyCollisions; /**< Whether to detect collision between different bodies */

  SimRobotCore2::Controller3DDrawingManager* drawingManager = nullptr; /**< The manager for 3D controller drawings */
  std::list<Body*> bodies; /**< List of bodies without a parent body */
  std::list<Actuator::Port*> actuators; /**< List of actuators that need to do something in every simulation step */
  std::list<Light*> lights; /**< List of scene lights */

  /** Default constructor */
  Scene()
  {
    color[0] = color[1] = color[2] = color[3] = 0.f;
  }

  /** Updates the transformation of movable objects */
  void updateTransformations();
  unsigned int lastTransformationUpdateStep = 0;

  /** Updates all actuators that need to do something for each simulation step */
  void updateActuators();

  /**
   * Creates resources to later draw the object in the given graphics context
   * @param graphicsContext The graphics context to create resources in
   */
  void createGraphics(GraphicsContext& graphicsContext) override;

  /**
   * Submits draw calls for appearance primitives of the object (including children) in the given graphics context
   * @param graphicsContext The graphics context to draw the object to
   */
  void drawAppearances(GraphicsContext& graphicsContext) const override;

  /**
   * Submits draw calls for physical primitives of the object (including children) in the given graphics context
   * @param graphicsContext The graphics context to draw the object to
   * @param flags Flags to enable or disable certain features
   */
  void drawPhysics(GraphicsContext& graphicsContext, unsigned int flags) const override;

private:
  /**
   * Visits controller drawings of graphical children
   * @param accept The functor to apply to every child
   */
  void visitGraphicalControllerDrawings(const std::function<void(GraphicalObject&)>& accept) override;

  /**
   * Visits controller drawings of physical children
   * @param accept The functor to apply to every child
   */
  void visitPhysicalControllerDrawings(const std::function<void(::PhysicalObject&)>& accept) override;

private:
  // API
  const QString& getFullName() const override {return SimObject::getFullName();}
  SimRobot::Widget* createWidget() override {return SimObject::createWidget();}
  const QIcon* getIcon() const override;
  SimRobotCore2::Renderer* createRenderer() override {return SimObject::createRenderer();}
  bool registerDrawing(SimRobotCore2::Controller3DDrawing& drawing) override {return ::PhysicalObject::registerDrawing(drawing);}
  bool unregisterDrawing(SimRobotCore2::Controller3DDrawing& drawing) override {return ::PhysicalObject::unregisterDrawing(drawing);}
  SimRobotCore2::Body* getParentBody() override {return ::PhysicalObject::getParentBody();}
  double getStepLength() const override {return stepLength;}
  unsigned int getStep() const override;
  double getTime() const override;
  unsigned int getFrameRate() const override;
  bool registerDrawingManager(SimRobotCore2::Controller3DDrawingManager& manager) override;
};
