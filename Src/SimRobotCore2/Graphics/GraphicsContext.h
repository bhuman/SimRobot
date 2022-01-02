/**
 * @file Graphics/GraphicsContext.h
 *
 * This file declares a class that handles graphics.
 *
 * @author Arne Hasselbring
 */

#pragma once

#include "Graphics/OpenGL.h"
#include "Platform/Assert.h"
#include "Tools/Math/Eigen.h"
#include <QOpenGLFunctions_3_3_Core>
#include <stack>
#include <unordered_map>
#include <vector>

class Light;
class QOffscreenSurface;
class QOpenGLContext;
class QOpenGLFramebufferObject;

class GraphicsContext : public QOpenGLFunctions_3_3_Core
{
public:
  enum PrimitiveTopology
  {
    pointList,
    lineList,
    triangleList
  };

  /**
   * A vertex with a 3D position and 3D normal.
   */
  struct VertexPN final
  {
    VertexPN(const Vector3f& position, const Vector3f& normal) :
      position(position), normal(normal)
    {}

    static constexpr std::uint32_t size = 6 * sizeof(float);
    static constexpr std::size_t vaoIndex = 0;

  private:
    Vector3f position;
    Vector3f normal;
  };

  /**
   * A vertex with a 3D position, 3D normal and 2D texture coordinates.
   */
  struct VertexPNT final
  {
    VertexPNT(const Vector3f& position, const Vector3f& normal, const Vector2f& textureCoordinates) :
      position(position), normal(normal), textureCoordinates(textureCoordinates)
    {}

    static constexpr std::uint32_t size = 8 * sizeof(float);
    static constexpr std::size_t vaoIndex = 1;

  private:
    Vector3f position;
    Vector3f normal;
    Vector2f textureCoordinates;
  };

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
    std::size_t vaoIndex = 0; /**< The index of the VAO this buffer belongs to. */

  private:
    int base = 0; /**< The index of the first vertex within the global VBO. */
    std::uint64_t offset = 0; /**< The offset of this buffer's memory within the VBO. */

    friend class GraphicsContext;
  };

  template<typename VertexType>
  struct VertexBuffer : VertexBufferBase
  {
    std::vector<VertexType> vertices; /**< Vertices which can be filled by the user. */

    /** Sets base class members. Must be called after \c vertices has been filled. */
    void finish() override
    {
      data = vertices.data();
      count = static_cast<std::uint32_t>(vertices.size());
      vaoIndex = VertexType::vaoIndex;
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

  struct Mesh final
  {
  private:
    GLenum mode = GL_TRIANGLES; /**< The primitive type of this mesh. */
    const VertexBufferBase* vertexBuffer = nullptr; /**< The vertex buffer of this mesh. */
    const IndexBuffer* indexBuffer = nullptr; /**< The (optional) index buffer of this mesh. */

    friend class GraphicsContext;
  };

  struct Texture final
  {
  private:
    /**
     * Loads a texture.
     * @param file Path to ...
     */
    Texture(const std::string& file);

    void* data = nullptr; /**< The raw texture data. */
    GLsizei width = 0; /**< The width of the texture in pixels. */
    GLsizei height = 0; /**< The height of the texture in pixels. */
    bool hasAlpha = false; /**< Whether the texture has an alpha channel. */
    GLenum byteOrder = 0; /**< The format of \c data. */
    std::size_t index = 0; /**< The index of this texture in the texture ID array. */

    friend class GraphicsContext;
  };

  struct Surface final
  {
  private:
    float diffuseColor[4];
    float ambientColor[4];
    float specularColor[4];
    float emissionColor[4];
    float shininess = 1.f;
    const Texture* texture = nullptr;

    friend class GraphicsContext;
  };

  struct ModelMatrix final
  {
  private:
    ModelMatrix(const std::vector<const float*>& product) :
      product(product)
    {}

    const std::vector<const float*> product;
    float memory[16];

    friend class GraphicsContext;
  };

  /** Destructor. */
  ~GraphicsContext();

  /** Determine buffer offsets of all declared buffers etc. */
  bool compile();

  /** Create per context data for the current context (which may include uploading data to the GPU). */
  void createGraphics();

  /** Destroy per context data for the current context. */
  void destroyGraphics();

  /**
   * Set the color that the color buffer is cleared to.
   * @param color Pointer to a RGBA color.
   */
  void setClearColor(const float* color);

  template<typename VertexType>
  VertexBuffer<VertexType>* requestVertexBuffer()
  {
    VertexBuffer<VertexType>* vertexBuffer = new VertexBuffer<VertexType>();
    vertexBuffers.push_back(vertexBuffer);
    return vertexBuffer;
  }

  IndexBuffer* requestIndexBuffer();

  Mesh* requestMesh(const VertexBufferBase* vertexBuffer, const IndexBuffer* indexBuffer, PrimitiveTopology primitiveTopology);

  Texture* requestTexture(const std::string& file);

  Surface* requestSurface(const float* diffuseColor, const float* ambientColor, const float* specularColor = nullptr, const float* emissionColor = nullptr, float shininess = 1.f, const Texture* texture = nullptr);

  void setGlobalAmbientLight(const float* color);

  void addLight(const ::Light* light);

  ModelMatrix* requestModelMatrix();

  void pushModelMatrixStack();

  void popModelMatrixStack();

  void pushModelMatrix(const Matrix4f& transformation);

  void pushModelMatrixByReference(const Matrix4f& transformation);

  void popModelMatrix();

  bool emptyModelMatrixStack() const;

  void updateModelMatrices(bool forceUpdate);





  // Commands to be used in draw functions:

  void startRendering(const Matrix4f& projection, const Matrix4f& view, int sx, int sy, int wx, int wh, bool clear, bool lighting = true, bool textures = true, bool smoothShading = true, bool fillPolygons = true);

  void startDepthOnlyRendering(const Matrix4f& projection, const Matrix4f& view, int sx, int sy, int wy, int wh, bool clear);

  void setForcedSurface(const Surface* surface);

  void draw(const Mesh* mesh, const ModelMatrix* modelMatrix, const Surface* surface);

  void finishRendering();

  /**
   * Prepares the off-screen renderer to render something. This call changes the
   * rendering context to the rendering context of the off-screen renderer.
   */
  void initOffscreenRenderer();

  /**
   * Selects the OpenGL context of the off-screen renderer. Call prepareRendering() before "drawing" the OpenGL image.
   * @param width The width of an image that will be rendered using this off-screen renderer
   * @param height The height of an image that will be rendered using this off-screen renderer
   * @param sampleBuffers Are sample buffers for multi-sampling required?
   * @return Whether the OpenGL context was successfully selected
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

private:
  /**
   * Sets uniforms for a surface.
   * @param surface The surface to set.
   */
  void setSurface(const Surface* surface);

  struct Shader
  {
    GLuint program; /**< The program object. */
    GLint cameraPVLocation = -1;
    GLint cameraPosLocation = -1;
    GLint modelMatrixLocation = -1;
    GLint surfaceHasTextureLocation = -1;
    GLint surfaceDiffuseColorLocation = -1;
    GLint surfaceAmbientColorLocation = -1;
    GLint surfaceSpecularColorLocation = -1;
    GLint surfaceEmissionColorLocation = -1;
    GLint surfaceShininessLocation = -1;
  };

  struct PerContextData
  {
    GLuint vao[2]; /**< The VAOs per vertex type. These exist per context. */
    GLuint vbo; /**< The VBO (shared between contexts within a share group). */
    GLuint ebo; /**< The EBO (shared between contexts within a share group). */

    std::vector<GLuint> textureIDs; /**< IDs for all textures. */

    std::array<Shader, 9> shaders; /**< The shaders. */

    // TODO: reference counter
  };

  std::unordered_map<const QOpenGLContext*, PerContextData> perContextData;
  std::unordered_map<std::string, Texture*> textures;
  std::vector<ModelMatrix*> modelMatrices;
  std::vector<Surface*> surfaces;
  std::vector<VertexBufferBase*> vertexBuffers;
  std::size_t vertexBufferTotalSize;
  std::vector<IndexBuffer*> indexBuffers;
  std::size_t indexBufferTotalSize;
  std::vector<Mesh*> meshes;
  std::vector<std::string> lights;

  // Used during initialization:
  using FloatPointerStack = std::stack<const float*, std::vector<const float*>>;
  struct ModelMatrixStack : FloatPointerStack
  {
    using FloatPointerStack::push;
    using FloatPointerStack::pop;
    using FloatPointerStack::empty;
    auto& getC() { return c; }
  };
  std::stack<ModelMatrixStack, std::vector<ModelMatrixStack>> modelMatrixStackStack;

  float clearColor[4] = {0.f};
  std::string globalAmbientLight = "vec4(0.0)";
  unsigned lastModelMatrixTimestamp = -1;

  // Used during render calls:
  PerContextData* data = nullptr;
  Shader* shader = nullptr;
  const Surface* forcedSurface = nullptr;

  /**
   * @class OffscreenBuffer
   * A render buffer data specialized on rendering images of a defined size.
   */
  class OffscreenBuffer final
  {
  public:
    QOpenGLFramebufferObject* framebuffer = nullptr;

    /** Destructor */
    ~OffscreenBuffer();
  };

  QOpenGLContext* offscreenContext = nullptr;
  QOffscreenSurface* offscreenSurface = nullptr;
  std::unordered_map<unsigned int, OffscreenBuffer> offscreenBuffers;
};
