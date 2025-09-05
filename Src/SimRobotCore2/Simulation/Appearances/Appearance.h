/**
 * @file Simulation/Appearances/Appearance.h
 * Declaration of class Appearance
 * @author Colin Graf
 */

#pragma once

#include "SimRobotCore2.h"
#include "Simulation/GraphicalObject.h"
#include "Simulation/SimObject.h"

/**
 * @class Appearance
 * Abstract class for the graphical representation of physical objects
 */
class Appearance : public SimObject, public GraphicalObject, public SimRobotCore2::Appearance
{
public:
  class Surface : public ElementCore2
  {
  public:

    float diffuseColor[4];
    bool hasAmbientColor = false;
    float ambientColor[4];
    float specularColor[4];
    float emissionColor[4];
    float shininess = 1.f;
    std::string diffuseTexture;
    GraphicsContext::Texture* texture = nullptr;
    GraphicsContext::Surface* surface = nullptr;

    /** Constructor */
    Surface();

    /**
     * Creates resources for this surface in the given graphics context
     * @param graphicsContext The graphics context to create resources in
     */
    void createGraphics(GraphicsContext& graphicsContext);

  private:
    /**
     * Registers an element as parent
     * @param element The element to register
     */
    void addParent(Element& element) override;
  };

protected:
  /**
   * Creates resources to later draw the object in the given graphics context
   * @param graphicsContext The graphics context to create resources in
   */
  void createGraphics(GraphicsContext& graphicsContext) override;

  /**
   * Submits draw calls for appearance primitives of the object (including children) in the given graphics context
   * @param graphicsContext The graphics context to draw the object to
   */
  void drawAppearances(GraphicsContext& graphicsContext) const override;

  /**
   * Creates a mesh for this appearance in the given graphics context
   * @param graphicsContext The graphics context to create the mesh in
   * @return The resulting mesh
   */
  virtual GraphicsContext::Mesh* createMesh(GraphicsContext& graphicsContext)
  {
    static_cast<void>(graphicsContext);
    return nullptr;
  }

  Surface* surface = nullptr; /**< The visual material of the object */

private:
  /**
   * Registers an element as parent
   * @param element The element to register
   */
  void addParent(Element& element) override;

  GraphicsContext::Mesh* mesh = nullptr; /**< The mesh to draw */

  //API
  const QString& getFullName() const override {return SimObject::getFullName();}
  SimRobot::Widget* createWidget() override {return SimObject::createWidget();}
  const QIcon* getIcon() const override;
  SimRobotCore2::Renderer* createRenderer() override {return SimObject::createRenderer();}
  bool registerDrawing(SimRobotCore2::Controller3DDrawing& drawing) override {return ::GraphicalObject::registerDrawing(drawing);}
  bool unregisterDrawing(SimRobotCore2::Controller3DDrawing& drawing) override {return ::GraphicalObject::unregisterDrawing(drawing);}
};
