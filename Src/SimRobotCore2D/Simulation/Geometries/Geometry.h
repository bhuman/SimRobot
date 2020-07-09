/**
 * @file Geometry.h
 *
 * This file declares a base class for geometries.
 *
 * @author Arne Hasselbring
 */

#pragma once

#include "Simulation/PhysicalObject.h"
#include "SimRobotCore2D.h"
#include <list>

class b2Body;
class b2Fixture;
class b2Shape;
class QPainter;

class Geometry : public PhysicalObject, public SimRobotCore2D::Geometry
{
public:
  /**
   * Adds the geometry to a body.
   * @param body The Box2D body to which the geometry should be attached.
   * @param geometryPose The pose relative to the Box2D body where the geometry should be.
   */
  void createGeometry(b2Body* body, const b2Transform& geometryPose);

  std::list<SimRobotCore2D::CollisionCallback*> callbacks; /**< The list of collision callbacks registered for this geometry. */
  std::uint16_t category = 0; /**< The category for collision filtering (0-15). */
  std::uint16_t mask = 0xffff; /**< The mask of categories with which this geometry wants to collide. */

protected:
  /**
   * Creates an instance of the concrete shape.
   * @param pose The pose of the shape relative to the Box2D body.
   * @return A pointer to a new shape.
   */
  virtual b2Shape* createShape(const b2Transform& pose);

  /**
   * Draws the shape.
   * @param painter The drawing helper (which already has the correct transformation).
   */
  virtual void drawShape(QPainter& painter) const;

  /** Initializes the physical properties of the geometry. */
  void createPhysics() override;

  /**
   * Draws the physical properties of the geometry.
   * @param painter The drawing helper.
   */
  void drawPhysics(QPainter& painter) const override;

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
   * Registers a collision callback for this geometry.
   * @param callback The callback to register.
   */
  void registerCollisionCallback(SimRobotCore2D::CollisionCallback& callback) override;

  /**
   * Unregisters a collision callback for this geometry.
   * @param callback The callback to unregister.
   * @return Whether the callback was previously registered.
   */
  bool unregisterCollisionCallback(SimRobotCore2D::CollisionCallback& callback) override;

private:
  b2Fixture* fixture = nullptr; /**< The Box2D fixture that this object represents. */
};

inline b2Shape* Geometry::createShape(const b2Transform&) {return nullptr;}
inline void Geometry::drawShape(QPainter&) const {}
