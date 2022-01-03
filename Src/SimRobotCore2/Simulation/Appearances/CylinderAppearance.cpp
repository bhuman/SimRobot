/**
 * @file Simulation/Appearances/CylinderAppearance.cpp
 * Implementation of class CylinderAppearance
 * @author Colin Graf
 */

#include "CylinderAppearance.h"
#include "Graphics/Primitives.h"

GraphicsContext::Mesh* CylinderAppearance::createMesh(GraphicsContext& graphicsContext)
{
  return Primitives::createCylinder(graphicsContext, radius, height, 16);
}
