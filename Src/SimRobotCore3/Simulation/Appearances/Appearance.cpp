/**
 * @file Simulation/Appearances/Appearance.cpp
 * Implementation of class Appearance
 * @author Colin Graf
 */

#include "Appearance.h"
#include "CoreModule.h"
#include "Platform/Assert.h"
#include "Tools/OpenGLTools.h"
#include <cstring>

Appearance::Surface::Surface()
{
  albedo[0] = albedo[1] = albedo[2] = 1.f;
}

void Appearance::Surface::createGraphics(GraphicsContext& graphicsContext)
{
  if(surface)
    return;

  if(!texturePath.empty())
  {
    ASSERT(!texture);
    texture = graphicsContext.requestTexture(texturePath);
  }

  surface = graphicsContext.requestSurface(albedo, alpha, metallic, roughness, ambient, texture);
}

void Appearance::createGraphics(GraphicsContext& graphicsContext)
{
  OpenGLTools::convertTransformation(rotation, translation, poseInParent);
  if(surface)
    surface->createGraphics(graphicsContext);

  ASSERT(!mesh);
  mesh = createMesh(graphicsContext);
  ASSERT(!mesh == !surface);

  graphicsContext.pushModelMatrix(poseInParent);
  ASSERT(!modelMatrix);
  modelMatrix = graphicsContext.requestModelMatrix(mesh ? GraphicsContext::ModelMatrix::appearance : GraphicsContext::ModelMatrix::controllerDrawing);
  GraphicalObject::createGraphics(graphicsContext);
  graphicsContext.popModelMatrix();
}

const QIcon* Appearance::getIcon() const
{
  return &CoreModule::module->appearanceIcon;
}

void Appearance::addParent(Element& element)
{
  SimObject::addParent(element);
  GraphicalObject::addParent(element);
}

void Appearance::Surface::addParent(Element& element)
{
  Appearance* appearance = dynamic_cast<Appearance*>(&element);
  ASSERT(appearance);
  ASSERT(!appearance->surface);
  appearance->surface = this;
}

void Appearance::drawAppearances(GraphicsContext& graphicsContext) const
{
  if(mesh)
    graphicsContext.draw(mesh, modelMatrix, surface->surface);

  GraphicalObject::drawAppearances(graphicsContext);
}
