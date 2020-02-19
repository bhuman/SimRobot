/**
 * @file Scene.h
 *
 * This file declares a class that represents a scene.
 *
 * @author Arne Hasselbring
 */

#pragma once

#include "Simulation/PhysicalObject.h"
#include "SimRobotCore2D.h"
#include <QSvgRenderer>
#include <list>
#include <string>

class Body;

class Scene : public PhysicalObject, public SimRobotCore2D::Scene
{
public:
  /** Loads the background image and creates phyiscs of children. */
  void createPhysics() override;

  /**
   * Draws the physical properties of the scene.
   * @param painter The drawing helper.
   */
  void drawPhysics(QPainter& painter) const override;

  /** Updates the transformations of all bodies. */
  void updateTransformations();

  std::string controller; /**< The name of the controller library for the scene. */
  float stepLength = 0.01f; /**< The duration of a simulation step [s]. */
  int velocityIterations = 8; /**< The number of Box2D iterations for solving the velocities. */
  int positionIterations = 3; /**< The number of Box2D iterations for solving the positions. */
  std::string background; /**< An optional background image that is drawn behind the physics. */
  std::list<Body*> bodies; /**< All bodies without a parent in the scene. */

protected:
  [[nodiscard]] const QString& getFullName() const override;
  [[nodiscard]] const QIcon* getIcon() const override;
  SimRobot::Widget* createWidget() override;
  SimRobotCore2D::Painter* createPainter() override;
  SimRobotCore2D::Body* getParentBody() const override;

  [[nodiscard]] double getStepLength() const override;
  [[nodiscard]] unsigned int getStep() const override;
  [[nodiscard]] double getTime() const override;
  [[nodiscard]] unsigned int getFrameRate() const override;

private:
  mutable QSvgRenderer backgroundRenderer; /**< The renderer for the background image. */
};
