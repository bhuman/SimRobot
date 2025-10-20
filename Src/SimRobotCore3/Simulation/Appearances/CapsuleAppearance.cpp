/**
 * @file Simulation/Appearances/CapsuleAppearance.cpp
 * Implementation of class CapsuleAppearance
 * @author Colin Graf
 */

#include "CapsuleAppearance.h"
#include "Graphics/Primitives.h"

GraphicsContext::Mesh* CapsuleAppearance::createMesh(GraphicsContext& graphicsContext)
{
  return Primitives::createCapsule(graphicsContext, radius, height, 16, 17);
}
