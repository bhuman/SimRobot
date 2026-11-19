/**
 * @file Simulation/Sensors/Camera.h
 * Declaration of class Camera
 * @author Colin Graf
 */

#pragma once

#include "Graphics/GraphicsContext.h"
#include "Simulation/Sensors/ImageSensorPort.h"
#include "Simulation/Sensors/Sensor.h"

/**
 * @class Camera
 * A simulated camera
 */
class Camera : public Sensor
{
public:
  unsigned int imageWidth; /**< The width of a camera image */
  unsigned int imageHeight; /**< The height of a camera image */
  float angleX;
  float angleY;
  GraphicsContext::ImageFormat format = GraphicsContext::rgbImage; /**< The format of the camera images (RGB or YUYV). */
  GraphicsContext::SensorModel sensorModel; /**< The properties of the sensor that are applied to the images. */
  bool multisample = false; /**< Whether the images are rendered with multisampling. */

  /** Default constructor */
  Camera();

private:
  /**
   * @class CameraSensor
   * The camera sensor interface
   */
  class CameraSensor : public ImageSensorPort
  {
    /**
     * Draws the scene.
     * @param graphicsContext The graphics context to draw to.
     */
    void drawScene(GraphicsContext& graphicsContext) const override;
  } sensor;

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
};
