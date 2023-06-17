/**
 * @file Simulation/Body.h
 * Declaration of class Body
 * @author Colin Graf
 */

#pragma once

#include "SimRobotCore2.h"
#include "Graphics/GraphicsContext.h"
#include "Simulation/PhysicalObject.h"
#include "Simulation/GraphicalObject.h"
#include <ode/common.h>
#include <ode/mass.h>
#include <list>

class Geometry;
class Mass;

/**
 * @class Body
 * A movable rigid body
 */
class Body : public PhysicalObject, public GraphicalObject, public SimRobotCore2::Body
{
public:
  dBodyID body = nullptr;
  Body* rootBody = nullptr; /**< The first movable body in a chain of bodies (might point to itself) */
  dMass mass; /**< The mass of the body (at \c centerOfMass)*/

  /** Default constructor */
  Body();

  /**
   * Creates resources to later draw the object in the given graphics context
   * @param graphicsContext The graphics context to create resources in
   */
  void createGraphics(GraphicsContext& graphicsContext) override;

  /**
   * Submits draw calls for physical primitives of the object (including children) in the given graphics context
   * @param graphicsContext The graphics context to draw the object to
   * @param flags Flags to enable or disable certain features
   */
  void drawPhysics(GraphicsContext& graphicsContext, unsigned int flags) const override;

  /**
   * Submits draw calls for appearance primitives of the object (including children) in the given graphics context
   * @param graphicsContext The graphics context to draw the object to
   */
  void drawAppearances(GraphicsContext& graphicsContext) const override;

  /** Updates the transformation from the parent to this body (since the pose of the body may have changed) */
  void updateTransformation();

  /**
   * Moves the object and its children relative to its current position
   * @param offset The distance to move
   */
  void move(const Vector3f& offset);

  /**
   * Rotate the object and its children around a point
   * @param rotation The rotation offset
   * @param point The point to turn around
   */
  void rotate(const RotationMatrix& rotation, const Vector3f& point);

private:
  Vector3f centerOfMass = Vector3f::Zero(); /**< The position of the center of mass relative to the pose of the body */
  Vector3f velocityInWorld; /**< A buffer used by \c getVelocity */

  dSpaceID bodySpace = nullptr; /**< The collision space for a connected group of movable objects */

  std::list<Body*> bodyChildren; /**< List of first-degree child bodies that are connected to this body over a joint */

  /** Destructor */
  ~Body();

  /**
   * Creates the physical objects used by the OpenDynamicsEngine (ODE).
   * These are a geometry object for collision detection and/or a body,
   * if the simulation object is movable.
   * @param graphicsContext The graphics context to create resources in
   */
  void createPhysics(GraphicsContext& graphicsContext) override;

  /**
   * Creates a ODE geometry and attaches it to the body
   * @param parentOffset the base geometry offset from the center of mass of the body
   * @param geometry A geometry description
   */
  void addGeometry(const Pose3f& parentOffset, Geometry& geometry);

  /**
   * Adds a mass to the mass of the body
   * @param mass A mass description of the mass to add
   */
  void addMass(Mass& mass);

  /**
   * Registers an element as parent
   * @param element The element to register
   */
  void addParent(Element& element) override;

  /**
   * Visits controller drawings of physical children
   * @param accept The functor to apply to every child
   */
  void visitPhysicalControllerDrawings(const std::function<void(::PhysicalObject&)>& accept) override;

  /**
   * Visits controller drawings of graphical children
   * @param accept The functor to apply to every child
   */
  void visitGraphicalControllerDrawings(const std::function<void(GraphicalObject&)>& accept) override;

  friend class Accelerometer;
  friend class CollisionSensor;

  GraphicsContext::ModelMatrix* comModelMatrix = nullptr; /**< The model matrix of the CoM sphere drawing */

private:
  // API
  const QString& getFullName() const override {return SimObject::getFullName();}
  SimRobot::Widget* createWidget() override {return SimObject::createWidget();}
  const QIcon* getIcon() const override {return SimObject::getIcon();}
  SimRobotCore2::Renderer* createRenderer() override {return SimObject::createRenderer();}
  bool registerDrawing(SimRobotCore2::Controller3DDrawing& drawing) override {return ::PhysicalObject::registerDrawing(drawing);}
  bool unregisterDrawing(SimRobotCore2::Controller3DDrawing& drawing) override {return ::PhysicalObject::unregisterDrawing(drawing);}
  SimRobotCore2::Body* getParentBody() override {return parentBody;}
  const float* getPosition() const override;
  const float* getVelocity() const override;
  void setVelocity(const float* velocity) override;
  bool getPose(float* position, float (*rotation)[3]) const override;
  void move(const float* pos) override;
  void move(const float* pos, const float (*rot)[3]) override;
  void resetDynamics() override;
  SimRobotCore2::Body* getRootBody() override {return rootBody;}
  void enablePhysics(bool enable) override;
};
