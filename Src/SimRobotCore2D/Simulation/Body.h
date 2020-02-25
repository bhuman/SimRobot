/**
 * @file Body.h
 *
 * This file declares a class for dynamic bodies.
 *
 * @author Arne Hasselbring
 */

#pragma once

#include "Simulation/PhysicalObject.h"
#include "SimRobotCore2D.h"

class b2Body;

class Body : public PhysicalObject, public SimRobotCore2D::Body
{
public:
  /** Destructor. Destroys the associated Box2D body. */
  ~Body() override;

  /**
   * Draws the physical properties of the body.
   * @param painter The drawing helper.
   */
  void drawPhysics(QPainter& painter) const override;

  /** Updates the transformation of the body. */
  void updateTransformation();

  Body* rootBody = nullptr; /**< The ancestor body which is a direct child of the scene element. */
  b2Body* body = nullptr; /**< The Box2D body object. */

protected:
  /** Initializes the physical properties of the body. */
  void createPhysics() override;

  /**
   * Registers another physical object as parent of this element (in order to avoid being registered as physical drawing).
   * @param element The element to register.
   */
  void addParent(Element& element) override;

  /**
   * Returns the full path to the object in the scene graph.
   * @return The full path ...
   */
  [[nodiscard]] const QString& getFullName() const override;

  /**
   * Returns an icon to visualize the object in the scene graph.
   * @return An icon ...
   */
  [[nodiscard]] const QIcon* getIcon() const override;

  /**
   * Creates a widget for this object.
   * @return The new widget instance.
   */
  SimRobot::Widget* createWidget() override;

  /**
   * Creates a painter for this object.
   * @return The new painter instance.
   */
  SimRobotCore2D::Painter* createPainter() override;

  /**
   * Returns the parent body of the physical object.
   * @return The parent body.
   */
  SimRobotCore2D::Body* getParentBody() const override;

  /**
   * Returns the position of the body in world coordinates.
   * @return The position of the body.
   */
  const float* getPosition() const override;

  /**
   * Fills the pose of the body in world coordinates.
   * @param position The translation of the body.
   * @param rotation The rotation of the body.
   */
  void getPose(float* position, float* rotation) const override;

  /**
   * Moves the body to a position in world coordinates, leaving its rotation untouched.
   * @param position The new translation of the body.
   */
  void move(const float* position) override;

  /**
   * Moves the body to a pose in world coordinates.
   * @param position The new translation of the body.
   * @param rotation The new rotation of the body.
   */
  void move(const float* position, float rotation) override;

  /**
   * Fills the linear velocity of the body in world coordinates.
   * @param velocity The linear velocity of the body.
   */
  void getVelocity(float* velocity) const override;

  /**
   * Fills the velocity of the body in world coordinates.
   * @param linear The linear velocity of the body.
   * @param angular The angular velocity of the body.
   */
  void getVelocity(float* linear, float* angular) const override;

  /**
   * Sets the linear velocity of the body.
   * @param velocity The new linear velocity of the body.
   */
  void setVelocity(const float* velocity) override;

  /**
   * Sets the linear and angular velocity of the body.
   * @param linear The new linear velocity of the body.
   * @param angular The new angular velocity of the body.
   */
  void setVelocity(const float* linear, float angular) override;

  /** Resets the linear and angular velocity of this body and all its children. */
  void resetDynamics() override;

  /**
   * Returns the ancestor body which is not a child of another body.
   * @return The root body.
   */
  SimRobotCore2D::Body* getRootBody() const override;

  /**
   * Sets whether the body should be physically simulated (i.e. collide with objects).
   * @param enable True to enable, false to disable.
   */
  void enablePhysics(bool enable) override;

private:
  std::list<Body*> bodyChildren; /**< The child bodies of this body. */
};
