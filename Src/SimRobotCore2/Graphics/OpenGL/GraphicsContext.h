/**
 * @file Graphics/OpenGL/GraphicsContext.h
 *
 * This file declares a class that handles graphics using OpenGL 3.3 Core.
 *
 * @author Arne Hasselbring
 */

#pragma once

#include "Graphics/OpenGL.h"
#include "Platform/Assert.h"
#include "Tools/Math/Eigen.h"
#include "Tools/Math/Pose3f.h"
#include <stack>
#include <unordered_map>
#include <vector>

class Light;
class QOffscreenSurface;
class QOpenGLContext;
class QOpenGLFramebufferObject;
class QOpenGLFunctions_3_3_Core;

class GraphicsContext
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
  struct VertexPN final
  {
    VertexPN(const Vector3f& position, const Vector3f& normal) :
      position(position), normal(normal)
    {}

    Vector3f position; /**< Vertex position in world space. */
    Vector3f normal; /**< Surface normal in world space. */

  private:
    /**
     * Declares the vertex attributes in an OpenGL context (VAO and VBO are already bound).
     * @param functions The OpenGL functions to use.
     */
    static void setupVertexAttributes(QOpenGLFunctions_3_3_Core& functions);
    static constexpr std::uint32_t size = 6 * sizeof(float); /**< Binary size of this vertex type. */
    static constexpr std::size_t index = 0; /**< Index of this vertex type in the vertex category array. */

    friend class GraphicsContext;
  };

  /**
   * A vertex with a 3D position, 3D normal and 2D texture coordinates.
   */
  struct VertexPNT final
  {
    VertexPNT(const Vector3f& position, const Vector3f& normal, const Vector2f& textureCoordinates) :
      position(position), normal(normal), textureCoordinates(textureCoordinates)
    {}

    Vector3f position; /**< Vertex position in world space. */
    Vector3f normal; /**< Surface normal in world space. */
    Vector2f textureCoordinates; /**< Texture coordinates at this vertex. */

  private:
    /**
     * Declares the vertex attributes in an OpenGL context (VAO and VBO are already bound).
     * @param functions The OpenGL functions to use.
     */
    static void setupVertexAttributes(QOpenGLFunctions_3_3_Core& functions);
    static constexpr std::uint32_t size = 8 * sizeof(float); /**< Binary size of this vertex type. */
    static constexpr std::size_t index = 1; /**< Index of this vertex type in the vertex category array. */

    friend class GraphicsContext;
  };

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

  private:
    int base = 0; /**< The index of the first vertex within the global VBO. */
    std::uint64_t offset = 0; /**< The offset of this buffer's memory within the VBO. */
    std::size_t vaoIndex = 0; /**< The index of the VAO this buffer belongs to. */

    friend class GraphicsContext;
  };

  /**
   * A vertex buffer for a specific vertex type.
   * @tparam VertexType The type of vertices in this buffer.
   */
  template<typename VertexType>
  struct VertexBuffer : VertexBufferBase
  {
    std::vector<VertexType> vertices; /**< Vertices which can be filled by the user. */

    /** Sets base class members. Must be called after \c vertices has been filled. */
    void finish() override
    {
      data = vertices.data();
      count = static_cast<std::uint32_t>(vertices.size());
      ASSERT(count);
    }

  private:
    /**
     * Returns the size of this buffer in bytes.
     * @return The size of this buffer in bytes.
     */
    std::size_t size() const override
    {
      return count * VertexType::size;
    }
  };

  /**
   * An index buffer.
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
      switch(type)
      {
        case GL_UNSIGNED_BYTE:
          return count;
        case GL_UNSIGNED_SHORT:
          return count * 2;
        case GL_UNSIGNED_INT:
          return count * 4;
      }
      return 0;
    }

    std::uint64_t offset = 0; /**< The offset of this buffer's memory within the EBO. */
    std::uint32_t count = 0; /**< The number of indices in this buffer. */
    GLenum type = GL_UNSIGNED_INT; /**< The type of the indices in this buffer. */

    friend class GraphicsContext;
  };

  /**
   * A structure that represents a drawable mesh.
   */
  struct Mesh final
  {
  private:
    GLenum mode = GL_TRIANGLES; /**< The primitive type of this mesh. */
    const VertexBufferBase* vertexBuffer = nullptr; /**< The vertex buffer of this mesh. */
    const IndexBuffer* indexBuffer = nullptr; /**< The (optional) index buffer of this mesh. */

    friend class GraphicsContext;
  };

  /**
   * A texture.
   */
  struct Texture final
  {
  private:
    /**
     * Loads a texture.
     * @param file Path to the texture file.
     */
    Texture(const std::string& file);

    /* Destructor. Frees texture memory. */
    ~Texture();

    GLubyte* data = nullptr; /**< The raw texture data. */
    GLsizei width = 0; /**< The width of the texture in pixels. */
    GLsizei height = 0; /**< The height of the texture in pixels. */
    bool hasAlpha = false; /**< Whether the texture has an alpha channel. */
    GLenum byteOrder = 0; /**< The format of \c data. */
    std::size_t index = 0; /**< The index of this texture in the texture ID array. */

    friend class GraphicsContext;
  };

  /**
   * The visual properties of a material.
   */
  struct Surface final
  {
  private:
    float diffuseColor[4]; /**< The RGBA color of the diffuse component. */
    float ambientColor[4]; /**< The RGBA color of the ambient component.  */
    float specularColor[4]; /**< The RGBA color of the specular component. */
    float emissionColor[4]; /**< The RGBA color that is emitted by the surface regardless of light sources. */
    float shininess = 1.f; /**< Shininess of the surface (for the specular component). */
    const Texture* texture = nullptr; /**< The texture of the surface. */

    std::size_t index = 0; /**< The index of this surface in the UBO. */

    static constexpr std::size_t memorySize = 80; /**< Size of a surface in graphics memory. The std140 layout pads array elements to multiples of 16 bytes. */

    friend class GraphicsContext;
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
     * @return A pointer to the calculated column-major 4x4 mmodel matrix.
     */
    const float* getPointer() const {return memory.data();}

  private:
    Pose3f constantPart; /**< The constant part of the model matrix. */
    const Pose3f* variablePart = nullptr; /**< An optional (pre-)multiplier that is evaluated each frame. */
    Matrix4f memory; /**< The memory for the final product. */

    friend class GraphicsContext;
  };

  /** Constructor. */
  GraphicsContext();

  /** Destructor. */
  ~GraphicsContext();

  /**
   * Determine buffer offsets of all declared buffers etc. and prepares the off-screen renderer to render something.
   * This call changes the rendering context to the rendering context of the off-screen renderer.
   */
  void compile();

  /** Create per context data for the current context (which may include uploading data to the GPU). */
  void createGraphics();

  /** Destroy per context data for the current context. */
  void destroyGraphics();

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
    VertexBuffer<VertexType>* vertexBuffer = new VertexBuffer<VertexType>();
    vertexBuffer->vaoIndex = VertexType::index;
    vertexBuffers[VertexType::index].buffers.push_back(vertexBuffer);
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
   * @return The new texture. The graphics context retains ownership of the object.
   */
  Texture* requestTexture(const std::string& file);

  /**
   * Requests a surface with the given properties.
   * @param diffuseColor The diffuse color (RGBA).
   * @param ambientColor The ambient color (RGBA).
   * @param specularColor The specular color (RGBA). May be \c nullptr.
   * @param emissionColor The emission color (RGBA). May be \c nullptr.
   * @param shininess The shininess of the surface (for the specular component).
   * @param texture The texture of the surface. May be \c nullptr.
   * @return The new surface. The graphics context retains ownership of the object.
   */
  Surface* requestSurface(const float* diffuseColor, const float* ambientColor, const float* specularColor = nullptr, const float* emissionColor = nullptr, float shininess = 1.f, const Texture* texture = nullptr);

  /**
   * Sets the color of the global ambient light.
   * @param color Pointer to a four-element (RGBA) color.
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

  /** Starts a new empty model matrix stack. */
  void pushModelMatrixStack();

  /** Switches back to the model matrix stack before the current one was created. */
  void popModelMatrixStack();

  /**
   * Pushes a pose on the model matrix stack that is constant.
   * @param pose The pose relative to the parent. This reference must be valid at least until it has been popped of the stack.
   */
  void pushModelMatrix(const Pose3f& pose);

  /**
   * Pushes a pose on the model matrix stack which is reevaluated every frame.
   * @param pose The relative to the parent. This reference must be valid for the entire lifetime of the graphics context.
   */
  void pushModelMatrixByReference(const Pose3f& pose);

  /** Pops a model matrix off the current stack. */
  void popModelMatrix();

  /**
   * Returns whether the current model matrix stack is empty.
   * @return Whether the current model matrix stack is empty.
   */
  bool emptyModelMatrixStack() const;

  /**
   * Recalculates the model matrices that have a reference component.
   * @param usage The usage class that should be updated.
   * @param forceUpdate Whether the model matrices should be updated although the simulation step did not change.
   */
  void updateModelMatrices(ModelMatrix::Usage usage, bool forceUpdate);

  /**
   * Starts a color render pass.
   * @param projection The projection matrix of the camera.
   * @param view The view matrix (= inverse pose) of the camera.
   * @param viewportX Lower left corner of the viewport. If negative, the viewport is not set.
   * @param viewportY Lower left corner of the viewport.
   * @param viewportWidth Width of the viewport.
   * @param viewportHeight Height of the viewport.
   * @param clear Whether to clear the color and depth buffers.
   * @param lighting Whether lighting should be active.
   * @param textures Whether textures should be active.
   * @param smoothShading Whether vertex normals are interpolated.
   * @param fillPolygons Whether polygons are filled (instead of wireframe rendering).
   */
  void startColorRendering(const Matrix4f& projection, const Matrix4f& view, int viewportX, int viewportY, int viewportWidth, int viewportHeight, bool clear, bool lighting = true, bool textures = true, bool smoothShading = true, bool fillPolygons = true);

  /**
   * Starts a depth only render pass.
   * @param projection The projection matrix of the camera.
   * @param view The view matrix (= inverse pose) of the camera.
   * @param viewportX Lower left corner of the viewport. If negative, the viewport is not set.
   * @param viewportY Lower left corner of the viewport.
   * @param viewportWidth Width of the viewport.
   * @param viewportHeight Height of the viewport.
   * @param clear Whether to clear the depth buffer.
   */
  void startDepthOnlyRendering(const Matrix4f& projection, const Matrix4f& view, int viewportX, int viewportY, int viewportWidth, int viewportHeight, bool clear);

  /**
   * Forces the following draw calls to use a specific surface.
   * @param surface The surface to use. \c nullptr makes draw calls use the default surface again.
   */
  void setForcedSurface(const Surface* surface);

  /**
   * Draws a mesh with a given transformation and surface.
   * @param mesh The mesh to draw.
   * @param modelMatrix The model matrix representing the transformation of the mesh.
   * @param surface The surface to use (ignored if a forced surface has been set).
   */
  void draw(const Mesh* mesh, const ModelMatrix* modelMatrix, const Surface* surface);

  /** Must be called as counterpart to \c startColorRendering / \c startDepthOnlyRendering. */
  void finishRendering();

  /**
   * Selects the OpenGL context of the off-screen renderer.
   * @param width The width of an image that will be rendered using this off-screen renderer.
   * @param height The height of an image that will be rendered using this off-screen renderer.
   * @param sampleBuffers Are sample buffers for multi-sampling required?
   * @return Whether the OpenGL context was successfully selected.
   */
  bool makeCurrent(int width, int height, bool sampleBuffers = true);

  /**
   * Reads an image from current rendering context.
   * @param image The buffer where is image will be saved to.
   * @param width The image width.
   * @param height The image height.
   */
  void finishImageRendering(void* image, int width, int height);

  /**
   * Reads a depth image from current rendering context.
   * @param image The buffer where is image will be saved to.
   * @param width The image width.
   * @param height The image height.
   */
  void finishDepthRendering(void* image, int width, int height);

  /**
   * Accesses the QOpenGLContext used for rendering. It can be used for creating further QOpenGLContexts with shared display lists and textures.
   * @return The QOpenGLContext used for rendering
   */
  QOpenGLContext* getOffscreenContext() const {return offscreenContext;}

  /**
   * Returns the OpenGL functions for the current context.
   * @return The functions or \c nullptr if the context is not registered.
   */
  QOpenGLFunctions_3_3_Core* getOpenGLFunctions() const;

private:
  /**
   * A shader (OpenGL: program) with extracted uniform locations.
   */
  struct Shader
  {
    GLuint program; /**< The program object. */
    GLint cameraPVLocation = -1; /**< The location of the cameraPV uniform in the program. */
    GLint cameraPosLocation = -1; /**< The location of the cameraPos uniform in the program. */
    GLint modelMatrixLocation = -1; /**< The location of the modelMatrix uniform in the program. */
    GLint surfaceIndexLocation = -1; /**< The location of the surfaceIndex uniform in the program. */
  };

  /**
   * A structure that captures OpenGL object IDs per context (some of them are shared).
   */
  struct PerContextData
  {
    QOpenGLFunctions_3_3_Core* f = nullptr; /**< OpenGL functions for this context (shared between contexts within a share group). */

    std::vector<GLuint> vao; /**< The VAOs per vertex type. These exist per context. */
    GLuint vbo; /**< The VBO (shared between contexts within a share group). */
    GLuint ebo; /**< The EBO (shared between contexts within a share group). */
    GLuint ubo; /**< The UBO (shared between contexts within a share group). */

    std::vector<GLuint> textureIDs; /**< IDs for all textures (shared between contexts within a share group). */

    std::array<Shader, 9> shaders; /**< Shaders for different settings (shared between contexts within a share group). */

    bool blendEnabled = false; /**< The current blend state in this context. */
    GLuint boundTexture = 0; /**< The currently bound texture in this context. */
    GLuint boundVAO = 0; /**< The currently bound VAO in this context. */

    std::size_t referenceCounter = 1; /**< Reference counter for this context. */
    std::size_t referenceCounterIndex; /**< Index in the vector of reference counters for the share group this context belongs to. */
  };

  /**
   * A structure that represents a category of vertices (corresponding to a VAO and a vertex type).
   */
  struct VertexCategory
  {
    void (*setupVertexAttributes)(QOpenGLFunctions_3_3_Core&) = nullptr; /**< A function that declares the vertex attributes of this type. */
    std::size_t stride = 0; /**< The stride between adjacent vertices in this category. */
    std::vector<VertexBufferBase*> buffers; /**< The vertex buffers of this category. */
  };

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
    const auto& getC() const {return c;}

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

  /**
   * Sets uniforms for a surface.
   * @param surface The surface to set.
   */
  void setSurface(const Surface* surface);

  /**
   * Compile a shader from a list of vertex shader sources and fragment shader sources
   * @param vertexShaderSources A list of source code fragments that are concatenated to form the vertex shader.
   * @param fragmentShaderSources A list of source code fragments that are concatenated to form the fragment shader.
   * @return A shader ID.
   */
  GLuint compileShader(const std::vector<const char*>& vertexShaderSources, const std::vector<const char*>& fragmentShaderSources);

  /**
   * Compile a shader for color render passes.
   * @param lighting Whether lighting is enabled.
   * @param textures Whether textures are enabled.
   * @param smooth Whether normals are interpolated smoothly between vertices.
   * @return A shader object.
   */
  Shader compileColorShader(bool lighting, bool textures, bool smooth);

  /**
   * Compile a shader for depth only render passes.
   * @return A shader object.
   */
  Shader compileDepthOnlyShader();

  // Context handling:
  std::vector<unsigned> referenceCounters; /**< Reference counters of shared data per share group. */
  std::unordered_map<const QOpenGLContext*, PerContextData> perContextData; /**< Map of OpenGL context pointers to per context data. */

  // Objects that are created during initialization (i.e. before the first call to \c createGraphics) but used throughout the runtime.
  std::unordered_map<std::string, Texture*> textures; /**< Map of filenames to textures. */
  std::array<ModelMatrixSet, ModelMatrix::numOfUsages> modelMatrixSets; /**< List of all registered model matrices. */
  std::vector<Surface*> surfaces; /**< List of all registered surfaces. */
  std::vector<VertexCategory> vertexBuffers; /**< List of the known vertex categories, pointing to all registered vertex buffers. */
  std::size_t vertexBufferTotalSize; /**< The total size of the vertex buffer object. */
  std::vector<IndexBuffer*> indexBuffers; /**< List of all registered index buffers. */
  std::size_t indexBufferTotalSize; /**< The total size of the element buffer object. */
  std::vector<Mesh*> meshes; /**< List of all registered meshes. */
  std::vector<std::string> lightDeclarations; /**< GLSL declarations for light sources. */
  std::vector<std::string> lightCalculations; /**< GLSL code that adds a light source in the fragment shader. */
  float clearColor[4] = {0.f}; /**< The color to clear the framebuffer to. */
  std::string globalAmbientLight = "vec4(0.0)"; /**< GLSL expression that represents the color of the global ambient light. */

  // To construct the model matrices:
  std::stack<ModelMatrixStack, std::vector<ModelMatrixStack>> modelMatrixStackStack; /**< A stack of model matrix stacks. */

  // Only valid between \c startColorRendering / \c startDepthOnlyRendering and \c finishRendering:
  PerContextData* data = nullptr; /**< The per context data for the current OpenGL context. */
  Shader* shader = nullptr; /**< The currently selected shader. */
  QOpenGLFunctions_3_3_Core* f = nullptr; /**< The OpenGL functions for the current OpenGL context. */
  const Surface* forcedSurface = nullptr; /**< The surface which overrides \c draw's argument. */

  // Offscreen rendering:
  QOpenGLContext* offscreenContext = nullptr; /**< The OpenGL context used for offscreen rendering. */
  QOffscreenSurface* offscreenSurface = nullptr; /**< The surface used for offscreen rendering. */
  std::unordered_map<unsigned int, QOpenGLFramebufferObject*> offscreenBuffers; /**< Map from encoded sizes to framebuffer objects. */
};
