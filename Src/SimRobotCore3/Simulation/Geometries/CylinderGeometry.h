/**
 * @file Simulation/Geometries/CylinderGeometry.h
 * Declaration of class CylinderGeometry
 * @author Colin Graf
 */

#pragma once

#include "Simulation/Geometries/Geometry.h"

/**
 * @class CylinderGeometry
 * A cylinder shaped geometry
 */
class CylinderGeometry : public Geometry
{
public:
  float height; /**< The height of the cylinder */
  float radius; /**< The radius of the cylinder */

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
