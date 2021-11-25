/**
 * @file OffscreenRenderer.cpp
 * An implementation of a hardware accelerated off-screen rendering module.
 * @author Colin Graf
 */

#include "OffscreenRenderer.h"
#include "Platform/Assert.h"
#include "Platform/OpenGL.h"
#include "Simulation/Simulation.h"
#include "Simulation/Scene.h"
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>

OffscreenRenderer::~OffscreenRenderer()
{
  if(glContext && glSurface)
    glContext->makeCurrent(glSurface);
  renderBuffers.clear();
  if(glContext)
    delete glContext;
  if(glSurface)
    delete glSurface;}

OffscreenRenderer::Buffer::~Buffer()
{
  if(framebuffer)
    delete framebuffer;}

bool OffscreenRenderer::makeCurrent(int width, int height, bool sampleBuffers)
{
  ASSERT(glContext && glSurface);
  glContext->makeCurrent(glSurface);

  // Considering weak graphics cards glClear is faster when the color and depth buffers are not greater then they have to be.
  // So we create an individual buffer for each size in demand.

  auto it = renderBuffers.find(width << 16 | height << 1 | (sampleBuffers ? 1 : 0));
  if(it == renderBuffers.end())
  {
    Buffer& buffer = renderBuffers[width << 16 | height << 1 | (sampleBuffers ? 1 : 0)];

    buffer.framebuffer = new QOpenGLFramebufferObject(width, height, QOpenGLFramebufferObject::Depth);
    if(!buffer.framebuffer->isValid())
    {
      delete buffer.framebuffer;
      buffer.framebuffer = nullptr;
      return false;
    }

    return true;
  }
  else
  {
    Buffer& buffer = it->second;
    return buffer.framebuffer && buffer.framebuffer->bind();
  }
}

void OffscreenRenderer::init()
{
  ASSERT(!glSurface && !glContext);

  glSurface = new QOffscreenSurface;
  glSurface->create();

  glContext = new QOpenGLContext;
  glContext->setShareContext(QOpenGLContext::globalShareContext());
  VERIFY(glContext->create());
  glContext->makeCurrent(glSurface);

  Simulation::simulation->scene->createGraphics(false);
}

void OffscreenRenderer::finishImageRendering(void* image, int w, int h)
{
  const int lineSize = w * 3;
  glPixelStorei(GL_PACK_ALIGNMENT, lineSize & (8 - 1) ? (lineSize & (4 - 1) ? 1 : 4) : 8);
  glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, image);
}

void OffscreenRenderer::finishDepthRendering(void* image, int w, int h)
{
  glPixelStorei(GL_PACK_ALIGNMENT, w * 4 & (8 - 1) ? 4 : 8);
  glReadPixels(0, 0, w, h, GL_DEPTH_COMPONENT, GL_FLOAT, image);
}

OffscreenRenderer::Method OffscreenRenderer::getRenderingMethod() const
{
  if(!renderBuffers.empty() && renderBuffers.begin()->second.framebuffer)
    return frameBuffer;
  return unknown;
}
