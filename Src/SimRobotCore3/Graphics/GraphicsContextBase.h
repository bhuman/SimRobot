/**
 * @file Graphics/GraphicsContextBase.h
 *
 * This file declares a class that provides the functionality of a graphics context that does not depend
 * on the graphics API: the CPU side of all resources (vertex and index buffers, meshes, textures, surfaces,
 * model matrices) and of the scene (lights and their shadow maps). The API specific class \c GraphicsContext
 * (selected at compile time) derives from it and creates the GPU resources, shaders and render passes.
 *
 * @author Arne Hasselbring
 */

#pragma once

#include "Platform/Assert.h"
#include "SimRobotCore3.h"
#include "Tools/Math/Eigen.h"
#include "Tools/Math/Pose3f.h"
#include <array>
#include <cstdint>
#include <optional>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

class GraphicsContext;
class Light;

class GraphicsContextBase
{
public:
  /**
   * Possible types of primitive topologies.
   */
  enum PrimitiveTopology
  {
    pointList, /**< Vertices are drawn as a list of points. */
    lineList, /**< Vertices are drawn as a list of lines (must be multiple of 2). */
    triangleList /**< Vertices are drawn as a list of triangles (must be multiple of 3). */
  };

  /**
   * A vertex with a 3D position and 3D normal.
   */
  struct VertexPN
  {
    VertexPN(const Vector3f& position, const Vector3f& normal) :
      position(position), normal(normal)
    {}

    Vector3f position; /**< Vertex position in world space. */
    Vector3f normal; /**< Surface normal in world space. */

    static constexpr std::uint32_t size = 6 * sizeof(float); /**< Binary size of this vertex type. */
    static constexpr std::size_t category = 0; /**< Index of this vertex type in the vertex category array. */
  };

  /**
   * A vertex with a 3D position, 3D normal and 2D texture coordinates.
   */
  struct VertexPNT
  {
    VertexPNT(const Vector3f& position, const Vector3f& normal, const Vector2f& textureCoordinates) :
      position(position), normal(normal), textureCoordinates(textureCoordinates)
    {}

    Vector3f position; /**< Vertex position in world space. */
    Vector3f normal; /**< Surface normal in world space. */
    Vector2f textureCoordinates; /**< Texture coordinates at this vertex. */

    static constexpr std::uint32_t size = 8 * sizeof(float); /**< Binary size of this vertex type. */
    static constexpr std::size_t category = 1; /**< Index of this vertex type in the vertex category array. */
  };

  static constexpr std::size_t numOfVertexCategories = 2; /**< The number of vertex types. */

  /**
   * Base class for vertex buffers.
   */
  struct VertexBufferBase
  {
    /** Virtual destructor for polymorphism. */
    virtual ~VertexBufferBase() = default;

    /** Sets members of this class. */
    virtual void finish() = 0;

  protected:
    /**
     * Returns the size of this buffer in bytes.
     * @return The size of this buffer in bytes.
     */
    virtual std::size_t size() const = 0;

    void* data = nullptr; /**< Pointer to the vertex data. */
    std::uint32_t count = 0; /**< The number of vertices in this buffer. */
    std::uint32_t base = 0; /**< The index of the first vertex within the global vertex buffer. */
    std::uint64_t offset = 0; /**< The offset of this buffer's memory within the global vertex buffer. */
    std::size_t category = 0; /**< The index of the vertex category this buffer belongs to. */

    friend class GraphicsContextBase;
    friend class ::GraphicsContext;
  };

  /**
   * A vertex buffer for a specific vertex type.
   * @tparam VertexType The type of vertices in this buffer.
   */
  template<typename VertexType>
  struct VertexBuffer final : VertexBufferBase
  {
    std::vector<VertexType> vertices; /**< Vertices which can be filled by the user. */

    /** Sets base class members. Must be called after \c vertices has been filled. */
    void finish() override
    {
      VertexBufferBase::data = vertices.data();
      VertexBufferBase::count = static_cast<std::uint32_t>(vertices.size());
      ASSERT(VertexBufferBase::count);
    }

  private:
    /**
     * Returns the size of this buffer in bytes.
     * @return The size of this buffer in bytes.
     */
    std::size_t size() const override
    {
      return VertexBufferBase::count * VertexType::size;
    }
  };

  /**
   * An index buffer (with 32 bit indices).
   */
  struct IndexBuffer final
  {
    std::vector<std::uint32_t> indices; /**< Indices which can be filled by the user. */

  private:
    /**
     * Returns the size of this buffer in bytes.
     * @return The size of this buffer in bytes.
     */
    std::uint32_t size() const
    {
      return count * static_cast<std::uint32_t>(sizeof(std::uint32_t));
    }

    std::uint64_t offset = 0; /**< The offset of this buffer's memory within the global index buffer. */
    std::uint32_t count = 0; /**< The number of indices in this buffer. */

    friend class GraphicsContextBase;
    friend class ::GraphicsContext;
  };

  /**
   * A structure that represents a drawable mesh.
   */
  struct Mesh final
  {
  private:
    PrimitiveTopology topology = triangleList; /**< The primitive type of this mesh. */
    const VertexBufferBase* vertexBuffer = nullptr; /**< The vertex buffer of this mesh. */
    const IndexBuffer* indexBuffer = nullptr; /**< The (optional) index buffer of this mesh. */

    friend class GraphicsContextBase;
    friend class ::GraphicsContext;
  };

  /**
   * A texture.
   */
  struct Texture final
  {
  private:
    /**
     * The pixels (bottom row first) in the memory layout of \c QImage::Format_ARGB32 on little endian machines:
     * four bytes per pixel, ordered blue, green, red, alpha (the alpha is not used if \c hasAlpha is false).
     * It is empty if loading the texture failed.
     */
    std::vector<std::uint8_t> data;
    unsigned int width = 0; /**< The width of the texture in pixels. */
    unsigned int height = 0; /**< The height of the texture in pixels. */
    bool hasAlpha = false; /**< Whether the texture has an alpha channel. */
    std::size_t index = 0; /**< The index of this texture in the texture array of the graphics API. */

    friend class GraphicsContextBase;
    friend class ::GraphicsContext;
  };

  /**
   * The visual properties of a material.
   */
  struct Surface final
  {
  private:
    // The first five members are uploaded as they are.
    float albedo[3]; /**< The albedo (RGB). */
    float alpha = 1.f; /**< The alpha value, from 0 (transparent) to 1 (opaque). */
    float metallic = 0.f; /**< Whether the surface is metallic, from 0 (dielectric, scattering) to 1 (metallic, no scattering). */
    float roughness = 0.5f; /**< The roughness of the surface, from 0 (perfect mirror) to 1 (maximum scattering). */
    float ambient = 1.f; /**< The ambient occlusion factor from 0 (no ambient lighting) to 1 (full ambient lighting). */
    const Texture* texture = nullptr; /**< The texture of the surface. */

    std::size_t index = 0; /**< The index of this surface in the surface array of the graphics API. */

    friend class GraphicsContextBase;
    friend class ::GraphicsContext;
  };

  /**
   * The pose of a model in the world.
   */
  struct ModelMatrix final
  {
    enum Usage
    {
      appearance, /**< This class may be used for appearances and controller drawings. */
      physicalDrawing, /**< This class may be used for physical drawings and controller drawings. */
      sensorDrawing, /**< This class may be used for sensor drawings and controller drawings. */
      controllerDrawing, /**< This class may be used for controller drawings only. */
      origin, /**< This is the origin model matrix. */
      dragPlane, /**< This is the drag plane model matrix. */
      numOfUsages
    };

    /**
     * Returns a pointer to the calculated column-major 4x4 model matrix.
     * @return A pointer to the calculated column-major 4x4 model matrix.
     */
    const float* getPointer() const {return memory.data();}

    /** Updates the memory for the final product. */
    void updateMemory();

  private:
    Pose3f constantPart; /**< The constant part of the model matrix. */
    const Pose3f* variablePart = nullptr; /**< An optional (pre-)multiplier that is evaluated each frame. */
    Matrix4f memory; /**< The memory for the final product. */

    friend class GraphicsContextBase;
    friend class ::GraphicsContext;
  };

  /**
   * The controller drawings of a frame (see \c SimRobotCore3::Drawing), prepared for being drawn by the graphics context.
   * Lines, dots, quads and cylinders are tessellated into vertex lists, spheres and ellipsoids are instances of
   * a unit sphere, and images carry their pixels. Everything is given in the coordinate system of the drawing.
   */
  struct DrawingList
  {
    /** A vertex of a point, line or triangle. */
    struct Vertex
    {
      float position[3]; /**< The position (in the coordinate system of the drawing). */
      std::uint8_t color[4]; /**< The color (RGBA). */
      float size; /**< The size of a point in pixels. */
    };

    /** A sphere or ellipsoid. */
    struct Instance
    {
      Matrix4f transformation; /**< The transformation of the unit sphere (in the coordinate system of the drawing). */
      std::uint8_t color[4]; /**< The color (RGBA). */
    };

    /** An image. */
    struct Image
    {
      Matrix4f transformation; /**< The transformation of the unit quad (in the coordinate system of the drawing). */
      unsigned int width, height; /**< The size of the image in pixels. */
      std::vector<std::uint8_t> rgb; /**< The pixels (RGB, top row first). */
    };

    /** One drawing, which refers to ranges of the lists. */
    struct Item
    {
      Matrix4f transformation; /**< The transformation from the coordinate system of the drawing to the world. */
      unsigned char renderOptions; /**< A combination of \c SimRobotCore3::Drawing::RenderOption. */
      std::size_t firstPoint, numOfPoints; /**< The range in \c points. */
      std::size_t firstLine, numOfLines; /**< The range of vertices in \c lines. */
      std::size_t firstTriangle, numOfTriangles; /**< The range of vertices in \c triangles. */
      std::size_t firstInstance, numOfInstances; /**< The range in \c instances. */
      std::size_t firstImage, numOfImages; /**< The range in \c images. */
    };

    std::vector<Vertex> points; /**< The vertices of all dots. */
    std::vector<Vertex> lines; /**< The vertices of all lines (two per line). */
    std::vector<Vertex> triangles; /**< The vertices of all triangles (three per triangle). */
    std::vector<Instance> instances; /**< All spheres and ellipsoids. */
    std::vector<Image> images; /**< All images. */
    std::vector<Item> items; /**< All drawings. */

    /** Removes everything. */
    void clear();

    /**
     * Adds a drawing.
     * @param drawing The drawing
     * @param objectTransformation The column-major 4x4 matrix that transforms the coordinate system of the object that the drawing is attached to into the world
     */
    void add(const SimRobotCore3::Drawing& drawing, const float* objectTransformation);
  };

  /**
   * Sets the meshes that are used to draw controller drawings.
   * @param sphereMesh A sphere with radius 1 (centered at 0, 0, 0)
   * @param quadMesh A quad in the yz-plane (see \c Primitives::createQuad)
   */
  void setDrawingMeshes(const Mesh* sphereMesh, const Mesh* quadMesh)
  {
    drawingSphereMesh = sphereMesh;
    drawingQuadMesh = quadMesh;
  }

  /**
   * Set the color that the color buffer is cleared to.
   * @param color Pointer to a four-element (RGBA) color.
   */
  void setClearColor(const float* color);

  /**
   * Requests a vertex buffer that is filled by the client.
   * @tparam VertexType The vertex type to be stored in the buffer.
   * @return The new vertex buffer. The graphics context retains ownership of the object.
   */
  template<typename VertexType>
  VertexBuffer<VertexType>* requestVertexBuffer()
  {
    auto* vertexBuffer = new VertexBuffer<VertexType>;
    vertexBuffer->category = VertexType::category;
    vertexBuffers[VertexType::category].push_back(vertexBuffer);
    return vertexBuffer;
  }

  /**
   * Requests an index buffer that is filled by the client.
   * @return The new index buffer. The graphics context retains ownership of the object.
   */
  IndexBuffer* requestIndexBuffer();

  /**
   * Requests a mesh made of the given objects.
   * @param vertexBuffer The vertex buffer of the mesh.
   * @param indexBuffer The index buffer of the mesh. May be \c nullptr.
   * @param primitiveTopology The primitive topology to draw the mesh.
   * @return The new mesh. The graphics context retains ownership of the object.
   */
  Mesh* requestMesh(const VertexBufferBase* vertexBuffer, const IndexBuffer* indexBuffer, PrimitiveTopology primitiveTopology);

  /**
   * Requests a texture from a given file.
   * @param file The path to the texture file.
   * @return The new texture or \c nullptr if it could not be loaded. The graphics context retains ownership of the object.
   */
  Texture* requestTexture(const std::string& file);

  /**
   * Requests a surface with the given properties.
   * @param albedo The albedo (RGB).
   * @param alpha The alpha value, from 0 (transparent) to 1 (opaque).
   * @param metallic Whether the surface is metallic, from 0 (dielectric, scattering) to 1 (metallic, no scattering).
   * @param roughness The roughness of the surface, from 0 (perfect mirror) to 1 (maximum scattering).
   * @param ambient The ambient occlusion factor from 0 (no ambient lighting) to 1 (full ambient lighting).
   * @param texture The texture of the surface. May be \c nullptr.
   * @return The new surface. The graphics context retains ownership of the object.
   */
  Surface* requestSurface(const float* albedo, float alpha = 1.f, float metallic = 0.f, float roughness = 0.5f, float ambient = 1.f, const Texture* texture = nullptr);

  /**
   * Sets the color of the global ambient light.
   * @param color Pointer to a three-element (RGB) color.
   */
  void setGlobalAmbientLight(const float* color);

  /**
   * Adds a light to the scene.
   * @param light The light element.
   */
  void addLight(const Light* light);

  /**
   * Requests a model matrix that represents the state of the top of the current model matrix stack.
   * @param usage A hint for which kind of draw calls the model matrix is going to be used.
   * @return The new model matrix. The graphics context retains ownership of the object.
   */
  ModelMatrix* requestModelMatrix(ModelMatrix::Usage usage);

  /**
   * Recalculates the model matrices that have a reference component.
   * @param usage The usage class that should be updated.
   * @param forceUpdate Whether the model matrices should be updated although the simulation step did not change.
   */
  void updateModelMatrices(ModelMatrix::Usage usage, bool forceUpdate);

  /** Starts a new empty model matrix stack. */
  void pushModelMatrixStack()
  {
    modelMatrixStackStack.emplace();
  }

  /** Switches back to the model matrix stack before the current one was created. */
  void popModelMatrixStack()
  {
    ASSERT(modelMatrixStackStack.top().empty());
    modelMatrixStackStack.pop();
  }

  /**
   * Pushes a pose on the model matrix stack that is constant.
   * @param pose The pose relative to the parent. This reference must be valid at least until it has been popped of the stack.
   */
  void pushModelMatrix(const Pose3f& pose)
  {
    modelMatrixStackStack.top().push(&pose);
  }

  /**
   * Pushes a pose on the model matrix stack which is reevaluated every frame.
   * @param pose The relative to the parent. This reference must be valid for the entire lifetime of the graphics context.
   */
  void pushModelMatrixByReference(const Pose3f& pose)
  {
    // A variable pose must be the first on the stack.
    ASSERT(modelMatrixStackStack.top().empty());
    modelMatrixStackStack.top().push(&pose);
    modelMatrixStackStack.top().bottomIsVariable = true;
  }

  /** Pops a model matrix off the current stack. */
  void popModelMatrix()
  {
    modelMatrixStackStack.top().pop();
    if(modelMatrixStackStack.top().empty())
      modelMatrixStackStack.top().bottomIsVariable = false;
  }

  /**
   * Returns whether the current model matrix stack is empty.
   * @return Whether the current model matrix stack is empty.
   */
  bool emptyModelMatrixStack() const
  {
    return modelMatrixStackStack.top().empty();
  }

protected:
  /**
   * A light of the scene.
   */
  struct LightData
  {
    enum Type
    {
      directional, /**< A directional light (it only has a direction). */
      point, /**< A point light (it only has a position and attenuation). */
      spot /**< A spot light (it has a position, attenuation, direction and cutoff). */
    };

    Type type = directional; /**< The type of the light. */
    float color[3]; /**< The color of the light (including its intensity). */
    float position[3] = {0.f, 0.f, 0.f}; /**< The position of the light (point and spot lights). */
    float direction[3] = {0.f, 0.f, 0.f}; /**< The direction of the light (directional and spot lights). */
    float constantAttenuation = 1.f; /**< The constant attenuation (point and spot lights). */
    float linearAttenuation = 0.f; /**< The linear attenuation (point and spot lights). */
    float quadraticAttenuation = 0.f; /**< The quadratic attenuation (point and spot lights). */
    float cutoff = 0.f; /**< The cosine of the angle of the light cone (spot lights). */
    std::optional<std::size_t> shadowMap; /**< The index of the shadow map of this light (if it casts shadows). */
  };

  /**
   * The shadow map of a light.
   */
  struct ShadowMap
  {
    Matrix4f view; /**< The view matrix of the light. */
    Matrix4f projection; /**< The projection matrix of the light. */
    unsigned int width; /**< The width of the shadow map. */
    unsigned int height; /**< The height of the shadow map. */
  };

  static constexpr std::size_t maxShadowMaps = 8; /**< The maximum number of lights with shadow maps (limited by the number of texture units). */

  /** Destructor. Frees all objects that were requested. */
  ~GraphicsContextBase();

  /**
   * Determines the memory layout of the global vertex buffer and index buffer (\c vertexBufferTotalSize,
   * \c indexBufferTotalSize and the offsets of the buffers) and the indices of textures and surfaces.
   * Must be called after all objects have been requested and before the GPU resources are created.
   */
  void layoutResources();

  /**
   * A stack of constant \c Pose3f pointers backed by a standard vector.
   */
  using PosePointerStack = std::stack<const Pose3f*, std::vector<const Pose3f*>>;

  /**
   * A stack of transformations representing model matrices.
   */
  struct ModelMatrixStack : PosePointerStack
  {
    using PosePointerStack::push;
    using PosePointerStack::pop;
    using PosePointerStack::empty;

    /**
     * Returns the stack contents as standard vector.
     * @return The stack contents.
     */
    const std::vector<const Pose3f*>& getC() const {return c;}

    bool bottomIsVariable = false; /**< Whether the bottom of the stack (= first pushed element) is captured by reference. */
  };

  /**
   * Model matrices of a specific usage class.
   */
  struct ModelMatrixSet
  {
    std::vector<ModelMatrix*> constantModelMatrices; /**< Model matrices of a specific class that do not change. */
    std::vector<ModelMatrix*> variableModelMatrices; /**< Model matrices of a specific class that change. */
    unsigned lastUpdate = -1; /**< The simulation step of the last model matrix update. */
  };

  static constexpr std::array<std::uint32_t, numOfVertexCategories> vertexStrides = {VertexPN::size, VertexPNT::size}; /**< The size of a vertex per vertex category. */

  // Objects that are created during initialization (i.e. before \c compile) but used throughout the runtime.
  std::unordered_map<std::string, Texture*> textures; /**< Map of filenames to textures. */
  std::array<ModelMatrixSet, ModelMatrix::numOfUsages> modelMatrixSets; /**< List of all registered model matrices. */
  std::vector<Surface*> surfaces; /**< List of all registered surfaces. */
  std::array<std::vector<VertexBufferBase*>, numOfVertexCategories> vertexBuffers; /**< The registered vertex buffers per vertex category. */
  std::uint64_t vertexBufferTotalSize = 0; /**< The total size of the global vertex buffer. */
  std::vector<IndexBuffer*> indexBuffers; /**< List of all registered index buffers. */
  std::uint64_t indexBufferTotalSize = 0; /**< The total size of the global index buffer. */
  std::vector<Mesh*> meshes; /**< List of all registered meshes. */
  const Mesh* drawingSphereMesh = nullptr; /**< The mesh of the spheres of controller drawings. */
  const Mesh* drawingQuadMesh = nullptr; /**< The mesh of the images of controller drawings. */
  std::vector<LightData> lights; /**< The lights of the scene. */
  std::vector<ShadowMap> shadowMaps; /**< The shadow maps of the lights that cast shadows. */
  float clearColor[4] = {0.f}; /**< The color to clear the framebuffer to. */
  float globalAmbientLight[3] = {0.f}; /**< The color of the global ambient light. */

private:
  // To construct the model matrices:
  std::stack<ModelMatrixStack, std::vector<ModelMatrixStack>> modelMatrixStackStack; /**< A stack of model matrix stacks. */
};
