/**
 * @file Simulation/Appearances/CapsuleAppearance.cpp
 * Implementation of class CapsuleAppearance
 * @author Colin Graf
 */

#include "CapsuleAppearance.h"
#include "Graphics/Primitives.h"

void CapsuleAppearance::createGraphics(GraphicsContext& graphicsContext)
{
  Appearance::createGraphics(graphicsContext);

  if(!capsule)
    capsule = Primitives::createCapsule(graphicsContext, radius, height, 16, 17);
}

void CapsuleAppearance::drawAppearances(GraphicsContext& graphicsContext, bool drawControllerDrawings) const
{
  if(!drawControllerDrawings)
    graphicsContext.draw(capsule, modelMatrices[modelMatrixIndex], surface->surface);

  Appearance::drawAppearances(graphicsContext, drawControllerDrawings);
}
