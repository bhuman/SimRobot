/**
 * @file Simulation/Sensors/SingleDistanceSensor.h
 * Declaration of class SingleDistanceSensor
 * @author Colin Graf
 */

#pragma once

#include "Graphics/GraphicsContext.h"
#include "Simulation/Sensors/Sensor.h"
#include <ode/common.h>

/**
 * @class SingleDistanceSensor
 * A distance sensor that uses a ray to detect distances to other objects
 */
class SingleDistanceSensor : public Sensor
{
public:
  float min; /**< The minimum distance the distance sensor can measure */
  float max; /**< The maximum distance the distance sensor can measure */

  /** Default constructor */
  SingleDistanceSensor();

private:
  /**
   * @class DistanceSensor
   * The distance sensor interface
   */
  class DistanceSensor : public Sensor::Port
  {
  public:
    ::PhysicalObject* physicalObject; /**< The physical object were the distance sensor is mounted on */
    dGeomID geom;
    float min;
    float max;
    float maxSqrDist;
    Pose3f offset;

  private:
    float closestSqrDistance;
    dGeomID closestGeom;
    Pose3f pose; /**< The pose of the sensor relative to the origin of the scene */

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

  /** Registers this object with children, actuators and sensors at SimRobot's GUI. */
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

  GraphicsContext::Mesh* ray = nullptr; /**< The ray mesh for the sensor drawing. */
  GraphicsContext::Surface* surface = nullptr; /**< The surface for the sensor drawing. */
};
