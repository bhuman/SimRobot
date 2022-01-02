/**
 * @file Graphics/Primitives.h
 *
 * This file declares functions to create meshes for standard geometry objects.
 *
 * @author Arne Hasselbring
 */

#pragma once

#include "Graphics/GraphicsContext.h"
#include "Tools/Math/Eigen.h"

class Primitives final
{
public:
  /**
   * Creates a line mesh.
   * @param graphicsContext The context in which to create the mesh.
   * @param start The start of the line.
   * @param end The end of the line.
   * @return A pointer to the new mesh (owned by the context).
   */
  static GraphicsContext::Mesh* createLine(GraphicsContext& graphicsContext, const Vector3f& start, const Vector3f& end);

  /**
   * Creates a sphere mesh (centered at 0, 0, 0).
   * @param graphicsContext The context in which to create the mesh.
   * @param radius The radius of the sphere.
   * @param slices The number of subdivisions in the radial plane.
   * @param stacks The number of subdivisions along the z axis.
   * @param withTextureCoordinates Whether the sphere should have texture coordinates.
   * @return A pointer to the new mesh (owned by the context).
   */
  static GraphicsContext::Mesh* createSphere(GraphicsContext& graphicsContext, float radius, unsigned int slices, unsigned int stacks, bool withTextureCoordinates);

  /**
   * Creates a cylinder mesh (centered at 0, 0, 0, along the z axis).
   * @param graphicsContext The context in which to create the mesh.
   * @param radius The radius of the cylinder (xy plane).
   * @param height The height of the cylinder (z axis).
   * @param slices The number of subdivisions in the radial plane.
   * @return A pointer to the new mesh (owned by the context).
   */
  static GraphicsContext::Mesh* createCylinder(GraphicsContext& graphicsContext, float radius, float height, unsigned int slices);

  /**
   * Creates a disk mesh (centered at 0, 0, 0, in the xy plane).
   * @param graphicsContext The context in which to create the mesh.
   * @param inner Inner radius of the disk.
   * @param outer Outer radius of the disk.
   * @param slices The number of subdivisions in the plane.
   * @return A pointer to the new mesh (owned by the context).
   */
  static GraphicsContext::Mesh* createDisk(GraphicsContext& graphicsContext, float inner, float outer, unsigned int slices);

  /**
   * Creates a box mesh (centered at 0, 0, 0).
   * @param graphicsContext The context in which to create the mesh.
   * @param width The width of the box (y axis).
   * @param height The height of the box (z axis).
   * @param depth The depth of the box (x axis).
   * @return A pointer to the new mesh (owned by the context).
   */
  static GraphicsContext::Mesh* createBox(GraphicsContext& graphicsContext, float width, float height, float depth);

  /**
   * Creates a capsule mesh (centered at 0, 0, 0, along the z axis).
   * @param graphicsContext The context in which to create the mesh.
   * @param radius The radius of the capsule (xy plane).
   * @param height The height of the capsule (z axis, including the spheres at the ends).
   * @param slices The number of subdivisions in the radial plane.
   * @param stacks The number of subdivisions along the z axis.
   * @return A pointer to the new mesh (owned by the context).
   */
  static GraphicsContext::Mesh* createCapsule(GraphicsContext& graphicsContext, float radius, float height, unsigned int slices, unsigned int stacks);

  /**
   * Creates a pyramid mesh (the top is at 0, 0, 0, the base in the yz plane).
   * @param graphicsContext The context in which to create the mesh.
   * @param width The width of the pyramid base (y axis).
   * @param height The height of the pyramid base (z axis).
   * @param depth The depth of the pyramid (x axis).
   * @return A pointer to the new mesh (owned by the context).
   */
  static GraphicsContext::Mesh* createPyramid(GraphicsContext& graphicsContext, float width, float height, float depth);
};
