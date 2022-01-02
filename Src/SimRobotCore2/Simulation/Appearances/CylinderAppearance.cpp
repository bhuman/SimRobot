/**
 * @file Simulation/Appearances/CylinderAppearance.cpp
 * Implementation of class CylinderAppearance
 * @author Colin Graf
 */

#include "CylinderAppearance.h"
#include "Graphics/Primitives.h"

void CylinderAppearance::createGraphics(GraphicsContext& graphicsContext)
{
  Appearance::createGraphics(graphicsContext);

  if(!cylinder)
    cylinder = Primitives::createCylinder(graphicsContext, radius, height, 16);
}

void CylinderAppearance::drawAppearances(GraphicsContext& graphicsContext, bool drawControllerDrawings) const
{
  if(!drawControllerDrawings)
    graphicsContext.draw(cylinder, modelMatrices[modelMatrixIndex], surface->surface);

  Appearance::drawAppearances(graphicsContext, drawControllerDrawings);
}
