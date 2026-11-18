/**
 * @file Simulation/Scene.h
 * Declaration of class Scene
 * @author Colin Graf
 */

#pragma once

#include "SimRobotCore3.h"
#include "Simulation/Actuators/Actuator.h"
#include "Simulation/Appearances/Appearance.h"
#include "Simulation/GraphicalObject.h"
#include "Simulation/PhysicalObject.h"
#include <list>
#include <string>
#include <unordered_map>

class Body;
class Environment;

/**
 * @class Scene
 * Class for the root node of the scene graph
 */
class Scene : public PhysicalObject, public GraphicalObject, public SimRobotCore3::Scene
{
public:

  std::string controller; /**< The name of the controller library. */
  float stepLength; /**< The length of a simulation step. */
  float gravity; /**< The gravity in the simulated world. */
  bool detectBodyCollisions = true; /**< Whether to detect collision between different bodies. */

  std::list<Body*> bodies; /**< List of bodies without a parent body */
  std::list<Actuator::Port*> actuators; /**< List of actuators that need to do something in every simulation step */
  Environment* environment = nullptr; /**< The environment of the scene (its background, ambient light and lights) or \c nullptr if there is none */

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
   * Records draw calls for the appearance primitives of the object (including children) in the draw list that the graphics
   * context is currently recording (see \c GraphicsContextBase::getDrawList).
   * @param graphicsContext The graphics context to record the draw calls in
   */
  void recordAppearances(GraphicsContext& graphicsContext) const override;

  /**
   * Records draw calls for the physical primitives of the object (including children) in the draw list that the graphics
   * context is currently recording (see \c GraphicsContextBase::getDrawList).
   * @param graphicsContext The graphics context to record the draw calls in
   * @param flags Flags to enable or disable certain features
   */
  void recordPhysics(GraphicsContext& graphicsContext, unsigned int flags) const override;

private:
  /**
   * Adds the drawings that controllers attached to the scene (and its bodies and physical children) to a list
   * @param list The list of drawings of a frame
   */
  void collectPhysicalDrawings(GraphicsContext::DrawingList& list) const override;

private:
  // API
  const QString& getFullName() const override {return SimObject::getFullName();}
  SimRobot::Widget* createWidget() override {return SimObject::createWidget();}
  const QIcon* getIcon() const override;
  SimRobotCore3::Renderer* createRenderer() override {return SimObject::createRenderer();}
  void setDrawing(const void* key, SimRobotCore3::Drawing&& drawing) override {::PhysicalObject::setDrawing(key, std::move(drawing));}
  bool removeDrawing(const void* key) override {return ::PhysicalObject::removeDrawing(key);}
  SimRobotCore3::Body* getParentBody() override {return ::PhysicalObject::getParentBody();}
  double getStepLength() const override {return stepLength;}
  unsigned int getStep() const override;
  double getTime() const override;
  unsigned int getFrameRate() const override;
};
