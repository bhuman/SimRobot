/**
 * @file Simulation/Sensors/ImageSensorPort.h
 * Declaration of class ImageSensorPort
 * @author Colin Graf
 * @author Arne Hasselbring
 */

#pragma once

#include "Graphics/GraphicsContext.h"
#include "Simulation/Sensors/Sensor.h"
#include "Tools/Math/Eigen.h"
#include "Tools/Math/Pose3f.h"
#include <vector>

/**
 * @class ImageSensorPort
 * The port of a sensor that renders color images of the scene (\c Camera and \c ObjectSegmentedImageSensor).
 * The images are converted by the GPU into a readback buffer, which the sensor reading points to. The images of
 * several sensors of the same kind and size can be rendered at once (\c renderCameraImages), which only starts the
 * rendering. Reading the value waits for it.
 */
class ImageSensorPort : public Sensor::Port
{
public:
  ::PhysicalObject* physicalObject = nullptr; /**< The physical object the sensor is mounted on. */
  Pose3f offset; /**< Offset of the sensor relative to the object it is mounted on. */

  /**
   * Initializes the port (in \c createPhysics of the sensor).
   * @param graphicsContext The graphics context in which the images are rendered.
   * @param imageWidth The width of the images.
   * @param imageHeight The height of the images.
   * @param angleX The horizontal opening angle.
   * @param angleY The vertical opening angle.
   * @param format The format of the images.
   */
  void initialize(GraphicsContext& graphicsContext, unsigned int imageWidth, unsigned int imageHeight, float angleX, float angleY, GraphicsContext::ImageFormat format);

protected:
  bool lighting = true; /**< Whether the scene is lit (with textures and smooth shading), otherwise its surfaces are plain. */

private:
  /**
   * Draws the scene (between \c startRendering and \c finishRendering).
   * @param graphicsContext The graphics context to draw to.
   */
  virtual void drawScene(GraphicsContext& graphicsContext) const = 0;

  /** Update the sensor value. Is called when required. */
  void updateValue() override;

  /**
   * Starts rendering the images of some ports into their readback buffers.
   * @param ports The ports, which have the same kind and image size.
   */
  static void render(const std::vector<ImageSensorPort*>& ports);

  //API
  bool getMinAndMax(float& min, float& max) const override {min = 0; max = 0xff; return true;}
  bool renderCameraImages(SimRobotCore3::SensorPort** cameras, unsigned int count) override;

  unsigned int imageWidth = 0; /**< The width of the images. */
  unsigned int imageHeight = 0; /**< The height of the images. */
  Matrix4f projection; /**< The perspective projection matrix. */
  GraphicsContext::ReadbackBuffer* readbackBuffer = nullptr; /**< The buffer into which the images are converted. */
};
