/**
 * @file Simulation/Appearances/CapsuleAppearance.h
 * Declaration of class CapsuleAppearance
 * @author Colin Graf
 */

#pragma once

#include "Graphics/GraphicsContext.h"
#include "Simulation/Appearances/Appearance.h"

/**
 * @class CapsuleAppearance
 * The graphical representation of a capsule
 */
class CapsuleAppearance : public Appearance
{
public:
  float height; /**< The height of the capsule */
  float radius; /**< The radius */

private:
  /**
   * Creates a mesh for this appearance in the given graphics context
   * @param graphicsContext The graphics context to create the mesh in
   * @return The resulting mesh
   */
  GraphicsContext::Mesh* createMesh(GraphicsContext& graphicsContext) override;
};
