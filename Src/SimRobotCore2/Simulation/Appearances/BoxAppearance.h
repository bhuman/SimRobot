/**
 * @file Simulation/Appearances/BoxAppearance.h
 * Declaration of class BoxAppearance
 * @author Colin Graf
 */

#pragma once

#include "Graphics/GraphicsContext.h"
#include "Simulation/Appearances/Appearance.h"

/**
 * @class BoxAppearance
 * The graphical representation of a simple box
 */
class BoxAppearance : public Appearance
{
public:
  float width; /**< The width of the box (cy) */
  float height; /**< The height of the box (cz) */
  float depth; /**< The depth of the box (cx) */

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

  GraphicsContext::Mesh* box = nullptr; /**< The box mesh */
};
