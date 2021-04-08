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
   * Draws physical primitives of the object (including children) on the currently selected OpenGL context
   * @param flags Flags to enable or disable certain features
   */
  void drawPhysics(unsigned int flags) const override;
};
