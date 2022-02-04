/**
 * @file Simulation/Sensors/ObjectSegmentedImageSensor.h
 * Declaration of class ObjectSegmentedImageSensor
 *
 * @author Jesse Richter-Klug
 *  includes a lot of duplicated code origin in Simulation/Sensors/Camera.h authored by Colin Graf
 */

#pragma once

#include "Simulation/Sensors/Sensor.h"

/**
 * @class ObjectSegmentedImageSensor
 * A simulated camera that takes pictures where each pixel that belongs to a different object gets a
 *   different color value. In fact, each pixel that belongs to the same object gets the same color value
 */
class ObjectSegmentedImageSensor : public Sensor
{
public:
  unsigned int imageWidth; /**< The width of a camera image */
  unsigned int imageHeight; /**< The height of a camera image */
  float angleX;
  float angleY;

  /** Default constructor */
  ObjectSegmentedImageSensor();

private:
  /**
   * @class ObjectSegmentedImageSensorPort
   * The camera sensor interface
   */
  class ObjectSegmentedImageSensorPort : public Sensor::Port
  {
  public:
    ::PhysicalObject* physicalObject; /**< The physical object were the camera is mounted on */
    ObjectSegmentedImageSensor* camera;
    unsigned char* imageBuffer; /**< A buffer for rendered image data */
    unsigned int imageBufferSize;
    Pose3f offset; /**< Offset of the camera relative to the body it mounted on */
    Matrix4f projection; /**< The perspective projection matrix */

    /** Update the sensor value. Is called when required. */
    void updateValue() override;

    //API
    bool getMinAndMax(float& min, float& max) const override {min = 0; max = 0xff; return true;}
    bool renderCameraImages(SimRobotCore2::SensorPort** cameras, unsigned int count) override;
  } sensor;

  /** Destructor */
  ~ObjectSegmentedImageSensor();

  /** Initializes the camera after all attributes have been set */
  void createPhysics(GraphicsContext& graphicsContext) override;

  /**
   * Registers an element as parent
   * @param element The element to register
   */
  void addParent(Element& element) override;

  /** Registers this object with children, actuators and sensors at SimRobot's GUI */
  void registerObjects() override;

  /**
   * Submits draw calls for physical primitives of the object (including children) in the given graphics context
   * @param graphicsContext The graphics context to draw the object to
   * @param flags Flags to enable or disable certain features
   */
  void drawPhysics(GraphicsContext& graphicsContext, unsigned int flags) const override;

  GraphicsContext::Mesh* pyramid = nullptr; /**< The pyramid mesh for the sensor drawing. */
  GraphicsContext::Surface* surface = nullptr; /**< The surface for the sensor drawing. */

  std::vector<GraphicsContext::Surface*>& surfaces; /**< The special surfaces for each body. */
};
