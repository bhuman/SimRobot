/**
 * @file Simulation/Geometries/BoxGeometry.h
 * Declaration of class BoxGeometry
 * @author Colin Graf
 */

#pragma once

#include "Simulation/Geometries/Geometry.h"

/**
 * @class BoxGeometry
 * A box shaped geometry
 */
class BoxGeometry : public Geometry
{
public:
  float width; /**< The width of the box (cy) */
  float height; /**< The height of the box (cz) */
  float depth; /**< The depth of the box (cx) */

private:
  /**
   * Creates the geometry (not including \c translation and \c rotation)
   * @param space A space to create the geometry in
   * @param The created geometry
   */
  dGeomID createGeometry(dSpaceID space) override;

  /**
   * Creates the physical objects used by the OpenDynamicsEngine (ODE).
   * These are a geometry object for collision detection and/or a body,
   * if the simulation object is movable.
   * @param graphicsContext The graphics context to create resources in
   */
  void createPhysics(GraphicsContext& graphicsContext) override;

  /**
   * Submits draw calls for physical primitives of the object (including children) in the given graphics context
   * @param graphicsContext The graphics context to draw the object to
   * @param flags Flags to enable or disable certain features
   */
  void drawPhysics(GraphicsContext& graphicsContext, unsigned int flags) const override;

  GraphicsContext::Mesh* box = nullptr; /**< The box mesh */
};
