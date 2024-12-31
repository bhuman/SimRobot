/**
 * @file Simulation/Sensors/CollisionSensor.h
 * Declaration of class CollisionSensor
 * @author Colin Graf
 */

#pragma once

#include "SimRobotCore2.h"
#include "Simulation/Sensors/Sensor.h"
#include <list>

/**
 * @class CollisionSensor
 * A collision sensor which uses geometries to detect collisions with other objects
 */
class CollisionSensor : public Sensor
{
public:
  /** Default constructor */
  CollisionSensor();

private:
  /**
   * @class CollisionSensorPort
   * The collision sensor interface
   */
  class CollisionSensorPort : public Sensor::Port, public SimRobotCore2::CollisionCallback
  {
    unsigned int lastCollisionStep = 0xffffffff; /**< The simulation step in which the last collision occurred. */

    /** Update the sensor value. Is called when required. */
    void updateValue() override;

    /**
     * The collision callback function.
     * Called whenever a geometry of the sensors collides with another geometry.
     * @param geom1 A geometry of the sensor
     * @param geom2 The other geometry
     */
    void collided(SimRobotCore2::Geometry& geom1, SimRobotCore2::Geometry& geom2) override;

    //API
    bool getMinAndMax(float&, float&) const override {return false;}
  } sensor;

  bool hasGeometries = false; /**< Whether there are geometries especially for this sensor */

  /**
   * Creates the physical objects used by the OpenDynamicsEngine (ODE).
   * These are a geometry object for collision detection and/or a body,
   * if the simulation object is movable.
   * @param graphicsContext The graphics context to create resources in
   */
  void createPhysics(GraphicsContext& graphicsContext) override;

  /**
   * Registers the sensor collision callback function to a list of geometries and subordinate geometries
   * @param geometries The list of geometries
   * @param setNotCollidable Whether the geometries will be immaterialized
   */
  void registerCollisionCallback(std::list<::PhysicalObject*>& geometries, bool setNotCollidable);

  /** Registers this object with children, actuators and sensors at SimRobot's GUI. */
  void registerObjects() override;

  /**
   * Submits draw calls for physical primitives of the object (including children) in the given graphics context
   * @param graphicsContext The graphics context to draw the object to
   * @param flags Flags to enable or disable certain features
   */
  void drawPhysics(GraphicsContext& graphicsContext, unsigned int flags) const override;
};
