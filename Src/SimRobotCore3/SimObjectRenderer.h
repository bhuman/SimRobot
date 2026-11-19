/**
 * @file SimObjectRenderer.h
 * Declaration of class SimObjectRenderer
 * @author Colin Graf
 */

#pragma once

#include "SimRobotCore3.h"
#include "Tools/Math/Eigen.h"
#include "Tools/Math/Pose3f.h"

class SimObject;
class Body;

/**
 * @class SimObjectRenderer
 * An interface for rendering scene objects on an OpenGL context
 */
class SimObjectRenderer : public SimRobotCore3::Renderer
{
public:
  /**
   * Constructor
   * @param simObject The object to render
   */
  SimObjectRenderer(SimObject& simObject);

  /** Destructor. Ensures that \c destroy has been called.  */
  ~SimObjectRenderer();

private:
  SimObject& simObject;
  unsigned int width = 0;
  unsigned int height = 0;

  Vector3f defaultCameraPos;
  Vector3f cameraPos;
  Vector3f cameraTarget;
  Pose3f cameraTransformation;
  float fovY = 40.f;
  Matrix4f projection;
  Matrix4f viewMatrix; /**< The view matrix (set by \c prepare and used by \c draw). */

  ShadeMode surfaceShadeMode = smoothShading;
  ShadeMode physicsShadeMode = noShading;
  ShadeMode drawingsShadeMode = flatShading;
  unsigned int renderFlags = enableLights | enableTextures | enableMultisample;

  bool initialized = false;
  bool dragging = false;
  DragType dragType;
  Body* dragSelection;
  Vector3f dragPlaneOffset;
  Vector3f dragLastPos;
  Vector3f interCameraPos;
  DragAndDropPlane dragPlane = xyPlane;
  Vector3f dragPlaneNormal;
  DragAndDropMode dragMode = keepDynamics;
  unsigned int dragStartTime;
  static constexpr int degreeSteps = 15;

  void updateCameraTransformation();

  bool intersectRayAndPlane(const Vector3f& point, const Vector3f& v,
                            const Vector3f& plane, const Vector3f& n,
                            Vector3f& intersection) const;
  Vector3f projectClick(int x, int y) const;
  Body* selectObject(const Vector3f& projectedClick);
  bool intersectClickAndCoordinatePlane(int x, int y, DragAndDropPlane plane, Vector3f& point) const;

  void calcDragPlaneNormal();

public:
  // API
#ifndef SIMROBOTCORE3_USE_OPENGL
  void init(const QRhiNativeHandles* nativeHandles) override;
  void destroy(const QRhiNativeHandles* nativeHandles) override;
  void prepare(const QRhiNativeHandles* nativeHandles) override;
  void draw(const QRhiNativeHandles* nativeHandles) override;
#else
  void init() override;
  void destroy() override;
  void prepare() override;
  void draw() override;
#endif
  void resize(float fovY, unsigned int width, unsigned int height) override;
  void getSize(unsigned int& width, unsigned int& height) const override;
  void setSurfaceShadeMode(ShadeMode shadeMode) override {surfaceShadeMode = shadeMode;}
  ShadeMode getSurfaceShadeMode() const override {return surfaceShadeMode;}
  void setPhysicsShadeMode(ShadeMode shadeMode) override {physicsShadeMode = shadeMode;}
  ShadeMode getPhysicsShadeMode() const override {return physicsShadeMode;}
  void setDrawingsShadeMode(ShadeMode shadeMode) override {drawingsShadeMode = shadeMode == smoothShading ? flatShading : shadeMode;}
  ShadeMode getDrawingsShadeMode() const override {return drawingsShadeMode;}
  void zoom(float change, float x, float y) override;
  void setRenderFlags(unsigned int renderFlags) override {this->renderFlags = renderFlags;}
  unsigned int getRenderFlags() const override {return renderFlags;}
  void resetCamera() override;
  int getFovY() const override {return int(fovY);}
  void setDragPlane(DragAndDropPlane plane) override;
  DragAndDropPlane getDragPlane() const override {return dragPlane;}
  void setDragMode(DragAndDropMode mode) override {dragMode = mode;}
  DragAndDropMode getDragMode() const override {return dragMode;}
  bool startDrag(int x, int y, DragType type) override;
  SimRobotCore3::Object* getDragSelection() override;
  bool moveDrag(int x, int y, DragType type) override;
  bool releaseDrag(int x, int y) override;
  void setCamera(const float* pos, const float* target) override;
  void getCamera(float* pos, float* target) override;
};
