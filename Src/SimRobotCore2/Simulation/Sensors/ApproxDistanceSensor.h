/**
 * @file Simulation/Sensors/ApproxDistanceSensor.h
 * Declaration of class ApproxDistanceSensor
 * @author Colin Graf
 */

#pragma once

#include "Graphics/GraphicsContext.h"
#include "Simulation/Sensors/Sensor.h"
#include "Tools/Math/Pose3f.h"
#include <ode/ode.h>

/**
 * @class ApproxDistanceSensor
 * A distance sensor that uses a ray to detect distances to other objects
 */
class ApproxDistanceSensor : public Sensor
{
public:
  float min; /**< The minimum distance the distance sensor can measure */
  float max; /**< The maximum distance the distance sensor can measure */
  float angleX;
  float angleY;

  /** Default constructor */
  ApproxDistanceSensor();

private:
  /**
   * @class DistanceSensor
   * The distance sensor interface
   */
  class DistanceSensor : public Sensor::Port
  {
  public:
    ::PhysicalObject* physicalObject;
    dGeomID geom;
    dGeomID scanRayGeom;
    float min;
    float max;
    float maxSqrDist;
    Pose3f offset;
    float tanHalfAngleX;
    float tanHalfAngleY;

  private:
    float closestSqrDistance;
    dGeomID closestGeom;
    Pose3f pose; /**< The pose of the sensor relative to the origin of the scene */
    Pose3f invertedPose;

    static void staticCollisionCallback(DistanceSensor* sensor, dGeomID geom1, dGeomID geom2);
    static void staticCollisionWithSpaceCallback(DistanceSensor* sensor, dGeomID geom1, dGeomID geom2);

    /** Update the sensor value. Is called when required. */
    void updateValue() override;

    //API
    bool getMinAndMax(float& min, float& max) const override;
  } sensor;

  /**
   * Creates the physical objects used by the OpenDynamicsEngine (ODE).
   * These are a geometry object for collision detection and/or a body,
   * if the simulation object is movable.
   * @param graphicsContext The graphics context to create resources in
   */
  void createPhysics(GraphicsContext& graphicsContext) override;

  /** Registers this object with children, actuators and sensors at SimRobot's GUI */
  void registerObjects() override;

  /**
   * Registers an element as parent
   * @param element The element to register
   */
  void addParent(Element& element) override;

  /**
   * Submits draw calls for physical primitives of the object (including children) in the given graphics context
   * @param graphicsContext The graphics context to draw the object to
   * @param flags Flags to enable or disable certain features
   */
  void drawPhysics(GraphicsContext& graphicsContext, unsigned int flags) const override;

  GraphicsContext::Mesh* pyramid = nullptr; /**< The pyramid mesh for the sensor drawing. */
  GraphicsContext::Surface* surface = nullptr; /**< The surface for the sensor drawing. */
};
