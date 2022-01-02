/**
 * @file Simulation/Compound.h
 * Declaration of class Compound
 * @author Colin Graf
 */

#pragma once

#include "SimRobotCore2.h"
#include "Simulation/GraphicalObject.h"
#include "Simulation/PhysicalObject.h"

class Geometry;

/**
 * @class Compound
 * A non-movable physical object
 */
class Compound : public PhysicalObject, public GraphicalObject, public SimRobotCore2::Compound
{
private:
  /**
   * Creates the physical objects used by the OpenDynamicsEngine (ODE).
   * These are a geometry object for collision detection and/or a body,
   * if the simulation object is movable.
   * @param graphicsContext The graphics context to create resources in
   */
  void createPhysics(GraphicsContext& graphicsContext) override;

  /**
   * Creates a stationary ODE geometry
   * @param parentPose The pose of the group or geometry
   * @param geometry A geometry description
   * @param callback A collision callback function attached to the geometry
   */
  void addGeometry(const Pose3f& parentPose, Geometry& geometry, SimRobotCore2::CollisionCallback* callback);

  /**
   * Creates resources to later draw the object in the given graphics context
   * @param graphicsContext The graphics context to create resources in
   */
  void createGraphics(GraphicsContext& graphicsContext) override;

  /**
   * Registers an element as parent
   * @param element The element to register
   */
  void addParent(Element& element) override;

private:
  // API
  const QString& getFullName() const override {return SimObject::getFullName();}
  SimRobot::Widget* createWidget() override {return SimObject::createWidget();}
  const QIcon* getIcon() const override {return SimObject::getIcon();}
  SimRobotCore2::Renderer* createRenderer() override {return SimObject::createRenderer();}
  bool registerDrawing(SimRobotCore2::Controller3DDrawing& drawing) override {return ::PhysicalObject::registerDrawing(drawing);}
  bool unregisterDrawing(SimRobotCore2::Controller3DDrawing& drawing) override {return ::PhysicalObject::unregisterDrawing(drawing);}
  SimRobotCore2::Body* getParentBody() override {return ::PhysicalObject::getParentBody();}
};
