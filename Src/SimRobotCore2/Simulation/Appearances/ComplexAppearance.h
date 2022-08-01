/**
 * @file Simulation/Appearances/ComplexAppearance.h
 * Declaration of class ComplexAppearance
 * @author Colin Graf
 */

#pragma once

#include "Simulation/Appearances/Appearance.h"
#include "Tools/Math/Eigen.h"
#include <list>
#include <vector>

/**
 * @class ComplexAppearance
 * The graphical representation of a complex shape
 */
class ComplexAppearance : public Appearance
{
public:
  /**
   * A vertex library
   */
  class Vertices : public ElementCore2
  {
  public:
    float unit;
    std::vector<Vector3f> vertices; /**< Available vertices */

  private:
    /**
     * Registers an element as parent
     * @param element The element to register
     */
    void addParent(Element& element) override;
  };

  /**
   * A normals library
   */
  class Normals : public ElementCore2
  {
  public:
    std::vector<Vector3f> normals; /**< Available normals */

  private:
    /**
     * Registers an element as parent
     * @param element The element to register
     */
    void addParent(Element& element) override;
  };

  /**
   * @class TexCoords
   * A texture point library
   */
  class TexCoords : public ElementCore2
  {
  public:
    std::vector<Vector2f> coords; /**< Available points */

  private:
    /**
     * Registers an element as parent
     * @param element The element to register
     */
    void addParent(Element& element) override;
  };

  /**
   * @enum Mode
   * Possibile primitive group types (\c triangles, \c quads, ...)
   */
  enum Mode
  {
    triangles,
    quads
  };

  /**
   * @class PrimitiveGroup
   * A primitive (aka. face, like triangle or quad...) or a group of primitives
   */
  class PrimitiveGroup : public ElementCore2
  {
  public:
    Mode mode; /**< The primitive group type (\c triangles, \c quads, ...) */
    std::list<unsigned int> vertices; /**< The indices of the vertices used to draw the primitive */

    /**
     * Constructor
     * @param mode The primitive group type (\c triangles, \c quads, ...)
     */
    PrimitiveGroup(Mode mode) : mode(mode) {}

  private:
    /**
     * Registers an element as parent
     * @param element The element to register
     */
    void addParent(Element& element) override;
  };

  struct Hasher;

  /**
   * @class Descriptor
   * A class that describes a complex appearance and can be used as a key in a map.
   */
  class Descriptor
  {
  public:
    /**
     * Constructor.
     * @param appearance The appearance which this object should describe.
     */
    Descriptor(const ComplexAppearance& appearance);

    /**
     * Checks whether two descriptors describe the same appearance.
     * @param other The descriptor to compare with.
     * @return Whether both descriptors describe the same appearance.
     */
    bool operator==(const Descriptor& other) const;

  private:
    ComplexAppearance::Vertices* vertices;
    ComplexAppearance::Normals* normals;
    ComplexAppearance::TexCoords* texCoords;
    const std::list<ComplexAppearance::PrimitiveGroup*>* primitiveGroups;

    friend struct ComplexAppearance::Hasher;
  };

  /**
   * @struct Hasher
   * A struct that can hash complex appearance descriptors.
   */
  struct Hasher
  {
    /**
     * Calculates a hash from a descriptor.
     * @param descriptor The descriptor to hash.
     * @return A hash value derived from the descriptor.
     */
    std::size_t operator()(const Descriptor& descriptor) const;
  };

  Vertices* vertices = nullptr; /**< The vertex library used for drawing the primitives */
  Normals* normals = nullptr; /**< The normals library used for drawing the primitives */
  TexCoords* texCoords = nullptr; /**< Optional texture points for textured primitives */
  std::list<PrimitiveGroup*> primitiveGroups; /**< The primitives that define the complex shape */

private:
  /**
   * Creates a mesh for this appearance in the given graphics context
   * @param graphicsContext The graphics context to create the mesh in
   * @return The resulting mesh
   */
  GraphicsContext::Mesh* createMesh(GraphicsContext& graphicsContext) override;

  /**
   * Creates the mesh if it is not already cached
   * @tparam VertexType The vertex type from the \c GraphicsContext that is used for this mesh
   * @tparam withTextureCoordinates Whether the vertex type has texture coordinates
   * @param graphicsContext The graphics context to create the mesh in
   * @return The resulting mesh
   */
  template<typename VertexType, bool withTextureCoordinates>
  GraphicsContext::Mesh* createMeshImpl(GraphicsContext& graphicsContext);
};
