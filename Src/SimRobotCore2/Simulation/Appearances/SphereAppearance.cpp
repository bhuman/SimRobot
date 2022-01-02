/**
 * @file Simulation/Appearances/SphereAppearance.cpp
 * Implementation of class SphereAppearance
 * @author Colin Graf
 */

#include "SphereAppearance.h"
#include "Graphics/Primitives.h"

void SphereAppearance::createGraphics(GraphicsContext& graphicsContext)
{
  Appearance::createGraphics(graphicsContext);

  if(!sphere)
    sphere = Primitives::createSphere(graphicsContext, radius, 16, 16, surface->texture);
}

void SphereAppearance::drawAppearances(GraphicsContext& graphicsContext, bool drawControllerDrawings) const
{
  if(!drawControllerDrawings)
    graphicsContext.draw(sphere, modelMatrices[modelMatrixIndex], surface->surface);

  Appearance::drawAppearances(graphicsContext, drawControllerDrawings);
}
