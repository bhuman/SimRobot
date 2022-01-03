/**
 * @file Simulation/Appearances/ComplexAppearance.h
 * Declaration of class ComplexAppearance
 * @author Colin Graf
 */

#pragma once

#include "Simulation/Appearances/Appearance.h"
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
   * @class Vertex
   * A class encapsulating the three components of a vertex
   */
  class Vertex
  {
  public:
    float x = 0.f; /**< The x-component of the vertex */
    float y = 0.f; /**< The y-component of the vertex */
    float z = 0.f; /**< The z-component of the vertex */

    /**
     * Default constructor; Sets all components to zero
     */
    Vertex() = default;

    /**
     * Constructs a vertex
     * @param x The x-component of the vertex
     * @param y The y-component of the vertex
     * @param z The z-component of the vertex
     */
    Vertex(float x, float y, float z) : x(x), y(y), z(z) {}
  };

  /**
   * The normal vector of a vertex
   */
  class Normal : public Vertex
  {
  public:
    unsigned int length = 0; /**< The length of the normal (used to normalize accumulated normals) */

    /**
     * @class Normal
     * Default constructor; Sets all components to zero
     */
    Normal() = default;

    /**
     * Constructs a vertex
     * @param x The x-component of the vertex
     * @param y The y-component of the vertex
     * @param z The z-component of the vertex
     * @param length The length of the normal
     */
    Normal(float x, float y, float z, unsigned int length) : Vertex(x, y, z), length(length) {}

    /**
     * Addition of another normal to this one.
     * @param other The other normal that will be added to this one
     */
    void operator+=(const Normal& other)
    {
      x += other.x;
      y += other.y;
      z += other.z;
      length += other.length;
    }
  };

  /**
   * @class TexCoord
   * A point on a texture
   */
  class TexCoord
  {
  public:
    float x; /**< The x-component of the point */
    float y; /**< The y-component of the point */

    /**
     * Constructs a point of a texture
     * @param x The x-component of the point
     * @param y The y-component of the point
     */
    TexCoord(float x, float y) : x(x), y(y) {}
  };

  /**
   * A vertex library
   */
  class Vertices : public Element
  {
  public:
    float unit;
    std::vector<Vertex> vertices; /**< Available vertices */

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
  class Normals : public Element
  {
  public:
    std::vector<Normal> normals; /**< Available normals */

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
  class TexCoords : public Element
  {
  public:
    std::vector<TexCoord> coords; /**< Available points */

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
  class PrimitiveGroup : public Element
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

  Vertices* vertices = nullptr; /**< The vertex library used for drawing the primitives */
  Normals* normals = nullptr; /**< The normals library used for drawing the primitives */
  TexCoords* texCoords = nullptr; /**< Optional texture points for textured primitives */
  std::list<PrimitiveGroup*> primitiveGroups; /**< The primitives that define the complex shape */
  bool normalsDefined = false; /**< Normals were manually defined */

private:
  /**
   * Creates a mesh for this appearance in the given graphics context
   * @param graphicsContext the graphics context to create the mesh in
   * @return The resulting mesh
   */
  GraphicsContext::Mesh* createMesh(GraphicsContext& graphicsContext) override;
};
