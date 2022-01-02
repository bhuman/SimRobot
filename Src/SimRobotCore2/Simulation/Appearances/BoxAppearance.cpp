/**
 * @file Simulation/Appearances/BoxAppearance.cpp
 * Implementation of class BoxAppearance
 * @author Colin Graf
 */

#include "BoxAppearance.h"
#include "Graphics/Primitives.h"

void BoxAppearance::createGraphics(GraphicsContext& graphicsContext)
{
  Appearance::createGraphics(graphicsContext);

  if(!box)
    box = Primitives::createBox(graphicsContext, width, height, depth);
}

void BoxAppearance::drawAppearances(GraphicsContext& graphicsContext, bool drawControllerDrawings) const
{
  if(!drawControllerDrawings)
    graphicsContext.draw(box, modelMatrices[modelMatrixIndex], surface->surface);

  Appearance::drawAppearances(graphicsContext, drawControllerDrawings);
}
