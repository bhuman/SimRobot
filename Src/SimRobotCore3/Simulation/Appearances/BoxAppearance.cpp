/**
 * @file Simulation/Appearances/BoxAppearance.cpp
 * Implementation of class BoxAppearance
 * @author Colin Graf
 */

#include "BoxAppearance.h"
#include "Graphics/Primitives.h"

GraphicsContext::Mesh* BoxAppearance::createMesh(GraphicsContext& graphicsContext)
{
  return Primitives::createBox(graphicsContext, width, height, depth);
}
