/**
 * @file Simulation/Appearances/SphereAppearance.h
 * Declaration of class SphereAppearance
 * @author Colin Graf
 */

#pragma once

#include "Graphics/GraphicsContext.h"
#include "Simulation/Appearances/Appearance.h"

/**
 * @class SphereAppearance
 * The graphical representation of a sphere
 */
class SphereAppearance : public Appearance
{
public:
  float radius; /**< The radius of the sphere */

private:
  /**
   * Creates a mesh for this appearance in the given graphics context
   * @param graphicsContext The graphics context to create the mesh in
   * @return The resulting mesh
   */
  GraphicsContext::Mesh* createMesh(GraphicsContext& graphicsContext) override;
};
