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
   * Records draw calls for the physical primitives of the object (including children) in the draw list that the graphics
   * context is currently recording (see \c GraphicsContextBase::getDrawList).
   * @param graphicsContext The graphics context to record the draw calls in
   * @param parts The parts of the object to record (see \c PhysicsParts)
   */
  void recordPhysics(GraphicsContext& graphicsContext, unsigned int parts) const override;

  GraphicsContext::Mesh* pyramid = nullptr; /**< The pyramid mesh for the sensor drawing. */
  GraphicsContext::Surface* surface = nullptr; /**< The surface for the sensor drawing. */
};
