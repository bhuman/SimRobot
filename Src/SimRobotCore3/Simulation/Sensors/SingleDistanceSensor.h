/**
 * @file Simulation/Sensors/SingleDistanceSensor.h
 * Declaration of class SingleDistanceSensor
 * @author Colin Graf
 */

#pragma once

#include "Graphics/GraphicsContext.h"
#include "Simulation/Sensors/Sensor.h"
#include "Tools/Math/Pose3f.h"

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
    ::PhysicalObject* physicalObject;
    float min;
    float max;
    Pose3f offset;

  private:
    Pose3f pose; /**< The pose of the sensor relative to the origin of the scene */

    /** Update the sensor value. Is called when required. */
    void updateValue() override;

    //API
    bool getMinAndMax(float& min, float& max) const override;
  } sensor;

  /**
   * Creates the physical objects used by MuJoCo.
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
   * Records draw calls for the physical primitives of the object (including children) in the draw list that the graphics
   * context is currently recording (see \c GraphicsContextBase::getDrawList).
   * @param graphicsContext The graphics context to record the draw calls in
   * @param parts The parts of the object to record (see \c PhysicsParts)
   */
  void recordPhysics(GraphicsContext& graphicsContext, unsigned int parts) const override;

  GraphicsContext::Mesh* ray = nullptr; /**< The ray mesh for the sensor drawing. */
  GraphicsContext::Surface* surface = nullptr; /**< The surface for the sensor drawing. */
};
