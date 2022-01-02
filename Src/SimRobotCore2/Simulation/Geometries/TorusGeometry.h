/**
 * @file Simulation/Geometries/TorusGeometry.h
 * Declaration of class TorusGeometry
 * @author Arne Hasselbring
 */

#pragma once

#include "Simulation/Geometries/Geometry.h"

/**
 * @class TorusGeometry
 * A torus shaped geometry
 */
class TorusGeometry : public Geometry
{
public:
  float majorRadius; /**< The major radius of the torus (radius of the ring skeleton in the xy plane) */
  float minorRadius; /**< The minor radius of the torus (radius of the "tube") */

  /** Registers the custom torus geometry class */
  static void registerGeometryClass();

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

  GraphicsContext::Mesh* sphere = nullptr; /**< The sphere mesh */
};
