/**
 * @file Simulation/Appearances/SphereAppearance.cpp
 * Implementation of class SphereAppearance
 * @author Colin Graf
 */

#include "SphereAppearance.h"
#include "Graphics/Primitives.h"

GraphicsContext::Mesh* SphereAppearance::createMesh(GraphicsContext& graphicsContext)
{
  return Primitives::createSphere(graphicsContext, radius, 16, 16, surface->texture);
}
