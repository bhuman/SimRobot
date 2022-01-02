/**
 * @file Simulation/Geometries/CapsuleGeometry.h
 * Declaration of class CapsuleGeometry
 * @author Colin Graf
 */

#pragma once

#include "Simulation/Geometries/Geometry.h"

/**
 * @class CapsuleGeometry
 * A capsule shaped geometry
 */
class CapsuleGeometry : public Geometry
{
public:
  float height; /**< The height of the capsule (including the spheres at the ends) */
  float radius; /**< The radius of the capsule */

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

  GraphicsContext::Mesh* capsule = nullptr; /**< The capsule mesh */
};
