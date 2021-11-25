/**
 * @file OffscreenRenderer.h
 * Declaration of class OffscreenRenderer
 * @author Colin Graf
 */

#pragma once

#include <unordered_map>

class QOffscreenSurface;
class QOpenGLContext;
class QOpenGLFramebufferObject;

/**
 * @class OffscreenRenderer
 * A hardware accelerated off-screen rendering module that uses the Qt 5 OpenGL library
 */
class OffscreenRenderer final
{
public:
  enum Method
  {
    unknown, /**< Call prepareRendering() first */
    frameBuffer /**< Good. */
  };

  /** Destructor */
  ~OffscreenRenderer();

  /**
   * Prepares the off-screen renderer to render something. This call changes the
   * rendering context to the rendering context of the off-screen renderer.
   */
  void init();

  /**
   * Selects the OpenGL context of the off-screen renderer. Call prepareRendering() before "drawing" the OpenGL image.
   * @param width The width of an image that will be rendered using this off-screen renderer
   * @param height The height of an image that will be rendered using this off-screen renderer
   * @param sampleBuffers Are sample buffers for multi-sampling required?
   * @return Whether the OpenGL context was successfully selected
   */
  bool makeCurrent(int width, int height, bool sampleBuffers = true);

  /**
   * Reads an image from current rendering context.
   * @param image The buffer where is image will be saved to.
   * @param width The image width.
   * @param height The image height.
   */
  void finishImageRendering(void* image, int width, int height);

  /**
   * Reads a depth image from current rendering context.
   * @param image The buffer where is image will be saved to.
   * @param width The image width.
   * @param height The image height.
   */
  void finishDepthRendering(void* image, int width, int height);

  /**
   * Requests the used rendering method. Only available when prepareRendering() was called at least once.
   * @return The used rendering method.
   */
  Method getRenderingMethod() const;

  /**
   * Accesses the QOpenGLContext used for rendering. It can be used for creating further QOpenGLContexts with shared display lists and textures.
   * @return The QOpenGLContext used for rendering
   */
  QOpenGLContext* getContext() const {return glContext;}

private:

  /**
   * @class Buffer
   * A render buffer data specialized on rendering images of a defined size.
   */
  class Buffer final
  {
  public:
    QOpenGLFramebufferObject* framebuffer = nullptr;

    /** Destructor */
    ~Buffer();
  };

  QOpenGLContext* glContext = nullptr;
  QOffscreenSurface* glSurface = nullptr;
  std::unordered_map<unsigned int, Buffer> renderBuffers;
};
