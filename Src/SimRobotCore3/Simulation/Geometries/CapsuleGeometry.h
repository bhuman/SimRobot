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
   * @param body The body to which to attach the geometry
   * @param The created geometry
   */
  mjsGeom* assembleGeometry(mjsBody* body) override;

  /**
   * Creates the physical objects used by the OpenDynamicsEngine (ODE).
   * These are a geometry object for collision detection and/or a body,
   * if the simulation object is movable.
   * @param graphicsContext The graphics context to create resources in
   */
  void createPhysics(GraphicsContext& graphicsContext) override;
};
