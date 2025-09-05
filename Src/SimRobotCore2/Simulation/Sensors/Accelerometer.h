/**
 * @file Simulation/Sensors/Accelerometer.h
 * Declaration of class Accelerometer
 * @author Colin Graf
 */

#pragma once

#include "Simulation/Sensors/Sensor.h"

class Body;

/**
 * @class Accelerometer
 * A simulated accelerometer
 */
class Accelerometer : public Sensor
{
public:
  /** Default constructor */
  Accelerometer();

private:
  /**
   * @class AccelerometerSensor
   * The acceleration sensor interface
   */
  class AccelerometerSensor : public Sensor::Port
  {
  public:
    Body* body; /**< The body where the accelerometer is mounted on. */
    float linearAcc[4]; /**< The sensor reading. */
    int sensorID = -1; /**< The ID within the sensor array of MuJoCo. */

    /** Update the sensor value. Is called when required. */
    void updateValue() override;

    //API
    bool getMinAndMax(float&, float&) const override {return false;}
  } sensor;

  /** Initializes the accelerometer after all attributes have been set */
  void createPhysics(GraphicsContext& graphicsContext) override;

  /**
   * Registers an element as parent
   * @param element The element to register
   */
  void addParent(Element& element) override;

  /** Registers this object with children, actuators and sensors at SimRobot's GUI */
  void registerObjects() override;
};
