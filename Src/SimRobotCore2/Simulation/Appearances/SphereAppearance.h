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
   * Creates resources to later draw the object in the given graphics context
   * @param graphicsContext The graphics context to create resources in
   */
  void createGraphics(GraphicsContext& graphicsContext) override;

  /**
   * Submits draw calls for appearance primitives of the object (including children) in the given graphics context
   * @param graphicsContext The graphics context to draw the object to
   * @param drawControllerDrawings Whether controller drawings should be drawn instead of the real appearance
   */
  void drawAppearances(GraphicsContext& graphicsContext, bool drawControllerDrawings) const override;

  GraphicsContext::Mesh* sphere = nullptr; /**< The sphere mesh */
};
