/**
 * @file Simulation/Geometries/SphereGeometry.h
 * Declaration of class SphereGeometry
 * @author Colin Graf
 */

#pragma once

#include "Simulation/Geometries/Geometry.h"

/**
 * @class SphereGeometry
 * A sphere shaped geometry
 */
class SphereGeometry : public Geometry
{
public:
  float radius; /**< The radius of the sphere */

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
