/**
 * @file SimRobotCore2D.h
 *
 * This file declares the interface to SimRobotCore2D.
 *
 * @author Arne Hasselbring
 */

#pragma once

#include "SimRobot.h"

class QPaintDevice;

namespace SimRobotCore2D
{
  class Body;
  class Painter;
  class CollisionCallback;

  enum Kind
  {
    scene = 2, /**< An object of the type SimRobotCore2D::Scene. */
    body, /**< An object of the type SimRobotCore2D::Body. */
    compound, /**< An object of the type SimRobotCore2D::Compound. */
    mass, /**< An object of the type SimRobotCore2D::Mass. */
    geometry /**< An object of the type SimRobotCore2D::Geometry. */
  };

  class Object : public SimRobot::Object
  {
  public:
    /**
     * Creates a painter instance for this object.
     * @return The new painter instance.
     */
    virtual Painter* createPainter() = 0;
  };

  class PhysicalObject : public Object
  {
  public:
    /**
     * Returns the parent body of the physical object.
     * @return The parent body.
     */
    [[nodiscard]] virtual Body* getParentBody() const = 0;
  };

  class Scene : public PhysicalObject
  {
  public:
    /**
     * Returns an object type identifier.
     * @return The identifier.
     */
    [[nodiscard]] int getKind() const override
    {
      return scene;
    }

    /**
     * Returns the length of one simulation step.
     * @return The time which is simulated by one step (in s).
     */
    [[nodiscard]] virtual double getStepLength() const = 0;

    /**
     * Returns the current simulation step.
     * @return The step.
     */
    [[nodiscard]] virtual unsigned int getStep() const = 0;

    /**
     * Returns the elapsed simulation time in seconds, starting with 0.
     * @return The time (in s).
     */
    [[nodiscard]] virtual double getTime() const = 0;

    /**
     * Returns the current frame rate.
     * @return The frame rate in frames per second.
     */
    [[nodiscard]] virtual unsigned int getFrameRate() const = 0;
  };

  class Body : public PhysicalObject
  {
  public:
    /**
     * Returns an object type identifier.
     * @return The identifier.
     */
    [[nodiscard]] int getKind() const override
    {
      return body;
    }

    /**
     * Returns the position of the body in world coordinates.
     * @return The position of the body.
     */
    [[nodiscard]] virtual const float* getPosition() const = 0;

    /**
     * Fills the pose of the body in world coordinates.
     * @param position The translation of the body.
     * @param rotation The rotation of the body.
     */
    virtual void getPose(float* position, float* rotation) const = 0;

    /**
     * Moves the body to a position in world coordinates, leaving its rotation untouched.
     * @param position The new translation of the body.
     */
    virtual void move(const float* position) = 0;

    /**
     * Moves the body to a pose in world coordinates.
     * @param position The new translation of the body.
     * @param rotation The new rotation of the body.
     */
    virtual void move(const float* position, float rotation) = 0;

    /**
     * Fills the linear velocity of the body in world coordinates.
     * @param velocity The linear velocity of the body.
     */
    virtual void getVelocity(float* velocity) const = 0;

    /**
     * Fills the velocity of the body in world coordinates.
     * @param linear The linear velocity of the body.
     * @param angular The angular velocity of the body.
     */
    virtual void getVelocity(float* linear, float* angular) const = 0;

    /**
     * Sets the linear velocity of the body.
     * @param velocity The new linear velocity of the body.
     */
    virtual void setVelocity(const float* velocity) = 0;

    /**
     * Sets the linear and angular velocity of the body.
     * @param linear The new linear velocity of the body.
     * @param angular The new angular velocity of the body.
     */
    virtual void setVelocity(const float* linear, float angular) = 0;

    /** Resets the linear and angular velocity of this body and all its children. */
    virtual void resetDynamics() = 0;

    /**
     * Returns the ancestor body which is not a child of another body.
     * @return The root body.
     */
    [[nodiscard]] virtual Body* getRootBody() const = 0;

    /**
     * Sets whether the body should be physically simulated (i.e. collide with objects).
     * @param enable True to enable, false to disable.
     */
    virtual void enablePhysics(bool enable) = 0;
  };

  class Compound : public PhysicalObject
  {
  public:
    /**
     * Returns an object type identifier.
     * @return The identifier.
     */
    [[nodiscard]] int getKind() const override
    {
      return compound;
    }
  };

  class Mass : public Object
  {
  public:
    /**
     * Returns an object type identifier.
     * @return The identifier.
     */
    [[nodiscard]] int getKind() const override
    {
      return mass;
    }
  };

  class Geometry : public PhysicalObject
  {
  public:
    /**
     * Returns an object type identifier.
     * @return The identifier.
     */
    [[nodiscard]] int getKind() const override
    {
      return geometry;
    }

    /**
     * Registers a collision callback for this geometry.
     * @param callback The callback to register.
     */
    virtual void registerCollisionCallback(CollisionCallback& callback) = 0;

    /**
     * Unregisters a collision callback for this geometry.
     * @param callback The callback to unregister.
     * @return Whether the callback was previously registered.
     */
    virtual bool unregisterCollisionCallback(CollisionCallback& callback) = 0;
  };

  class Painter
  {
  public:
    enum DragAndDropMode
    {
      keepDynamics, /**< The object keeps its previous velocity after being moved. */
      resetDynamics, /**< The object's velocity is set to zero after being moved. */
      adoptDynamics, /**< The object gets a velocity according to the movement. */
      applyDynamics /**< The object stays where it is, but gets a force/torque applied. */
    };

    enum DragType
    {
      dragNormal, /**< Translate the world/object. */
      dragRotate /**< Rotate the world/object. */
    };

    /** Virtual destructor for polymorphism. */
    virtual ~Painter() = default;

    /**
     * Draws the object to a device.
     * @param device The device to which to draw.
     */
    virtual void draw(QPaintDevice* device) = 0;

    /**
     * Changes the zoom of the painter.
     * @param change The change in zoom.
     * @param x The x coordinate of the point around which the zoom should be added.
     * @param y The y coordinate of the point around which the zoom should be added.
     */
    virtual void zoom(float change, int x, int y) = 0;

    /**
     * Resizes the painter.
     * @param width The new width.
     * @param height The new height.
     */
    virtual void resize(int width, int height) = 0;

    /**
     * Starts a dragging process.
     * @param x The x coordinate where the dragging is started.
     * @param y The y coordinate where the dragging is started.
     * @param dragType The drag type.
     */
    virtual void startDrag(int x, int y, DragType dragType) = 0;

    /**
     * Moves the dragged object.
     * @param x The x coordinate where the drag moved.
     * @param y The y coordinate where the drag moved.
     * @param dragType The drag type.
     * @return Whether there was something to drag.
     */
    virtual bool moveDrag(int x, int y, DragType dragType) = 0;

    /**
     * Ends a dragging process.
     * @param x The x coordinate where the dragging is ended.
     * @param y The y coordinate where the dragging is ended.
     * @return Whether there was something to drag.
     */
    virtual bool releaseDrag(int x, int y) = 0;

    /**
     * Returns the currently dragged object.
     * @return The currently dragged object.
     */
    virtual Object* getDragSelection() = 0;

    /**
     * Sets the drag mode of the painter.
     * @param dragMode The new drag mode.
     */
    virtual void setDragMode(DragAndDropMode dragMode) = 0;

    /**
     * Gets the drag mode of the painter.
     * @return The current drag mode.
     */
    [[nodiscard]] virtual DragAndDropMode getDragMode() const = 0;

    /**
     * Sets the view settings of the painter.
     * @param offset The offset to the center.
     * @param zoomFactor The zoom factor.
     * @param rotation The rotation.
     */
    virtual void setView(const float* offset, float zoomFactor, float rotation) = 0;

    /**
     * Gets the view settings of the painter.
     * @param offset The offset to the center.
     * @param zoomFactor The zoom factor.
     * @param rotation The rotation.
     */
    virtual void getView(float* offset, float* zoomFactor, float* rotation) const = 0;

    /** Resets the view of the painter to the default. */
    virtual void resetView() = 0;
  };

  class CollisionCallback
  {
  public:
    /**
     * Is called when the geometry collides with another geometry.
     * @param geom1 The first geometry (the one to which the callback is attached).
     * @param geom2 The other geometry.
     */
    virtual void collided(Geometry& geom1, Geometry& geom2) = 0;
  };
}
