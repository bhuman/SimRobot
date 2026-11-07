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
  static GraphicsContext::Mesh* createLine(GraphicsContextBase& graphicsContext, const Vector3f& start, const Vector3f& end);

  /**
   * Creates a sphere mesh (centered at 0, 0, 0).
   * @param graphicsContext The context in which to create the mesh.
   * @param radius The radius of the sphere.
   * @param slices The number of subdivisions in the radial plane.
   * @param stacks The number of subdivisions along the z axis.
   * @param withTextureCoordinates Whether the sphere should have texture coordinates.
   * @return A pointer to the new mesh (owned by the context).
   */
  static GraphicsContext::Mesh* createSphere(GraphicsContextBase& graphicsContext, float radius, unsigned int slices, unsigned int stacks, bool withTextureCoordinates);

  /**
   * Creates a cylinder mesh (centered at 0, 0, 0, along the z axis).
   * @param graphicsContext The context in which to create the mesh.
   * @param radius The radius of the cylinder (xy plane).
   * @param height The height of the cylinder (z axis).
   * @param slices The number of subdivisions in the radial plane.
   * @return A pointer to the new mesh (owned by the context).
   */
  static GraphicsContext::Mesh* createCylinder(GraphicsContextBase& graphicsContext, float radius, float height, unsigned int slices);

  /**
   * Creates a disk mesh (centered at 0, 0, 0, in the xy plane).
   * @param graphicsContext The context in which to create the mesh.
   * @param inner Inner radius of the disk.
   * @param outer Outer radius of the disk.
   * @param slices The number of subdivisions in the plane.
   * @return A pointer to the new mesh (owned by the context).
   */
  static GraphicsContext::Mesh* createDisk(GraphicsContextBase& graphicsContext, float inner, float outer, unsigned int slices);

  /**
   * Creates a box mesh (centered at 0, 0, 0).
   * @param graphicsContext The context in which to create the mesh.
   * @param width The width of the box (y axis).
   * @param height The height of the box (z axis).
   * @param depth The depth of the box (x axis).
   * @return A pointer to the new mesh (owned by the context).
   */
  static GraphicsContext::Mesh* createBox(GraphicsContextBase& graphicsContext, float width, float height, float depth);

  /**
   * Creates a capsule mesh (centered at 0, 0, 0, along the z axis).
   * @param graphicsContext The context in which to create the mesh.
   * @param radius The radius of the capsule (xy plane).
   * @param height The height of the capsule (z axis, including the spheres at the ends).
   * @param slices The number of subdivisions in the radial plane.
   * @param stacks The number of subdivisions along the z axis.
   * @return A pointer to the new mesh (owned by the context).
   */
  static GraphicsContext::Mesh* createCapsule(GraphicsContextBase& graphicsContext, float radius, float height, unsigned int slices, unsigned int stacks);

  /**
   * Creates a quad mesh (in the yz plane, from -1 to 1 in both directions, facing the positive x axis). The texture coordinates are
   * (0, 0) at z = 1, y = 1 and (1, 1) at z = -1, y = -1. Unlike in the other meshes, v grows downwards, so the first row of a texture
   * is at z = 1. A texture whose first row is the top row of its image (unlike the textures of the scene, which are stored bottom row
   * first) therefore appears upright and not mirrored if one looks along the positive x axis (i.e. at the back of the quad).
   * @param graphicsContext The context in which to create the mesh.
   * @return A pointer to the new mesh (owned by the context).
   */
  static GraphicsContext::Mesh* createQuad(GraphicsContextBase& graphicsContext);

  /**
   * Creates a pyramid mesh (the top is at 0, 0, 0, the base in the yz plane).
   * @param graphicsContext The context in which to create the mesh.
   * @param width The width of the pyramid base (y axis).
   * @param height The height of the pyramid base (z axis).
   * @param depth The depth of the pyramid (x axis).
   * @return A pointer to the new mesh (owned by the context).
   */
  static GraphicsContext::Mesh* createPyramid(GraphicsContextBase& graphicsContext, float width, float height, float depth);
};
