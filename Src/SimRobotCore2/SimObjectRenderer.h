/**
 * @file SimObjectRenderer.h
 * Declaration of class SimObjectRenderer
 * @author Colin Graf
 */

#pragma once

#include "SimRobotCore2.h"
#include "Tools/Math/Eigen.h"

class SimObject;
class Body;

/**
 * @class SimObjectRenderer
 * An interface for rendering scene objects on an OpenGL context
 */
class SimObjectRenderer : public SimRobotCore2::Renderer
{
public:
  /**
   * Constructor
   * @param simObject The object to render
   */
  SimObjectRenderer(SimObject& simObject);

private:
  SimObject& simObject;
  unsigned int width = 0;
  unsigned int height = 0;

  CameraMode cameraMode;
  Vector3f defaultCameraPos;
  Vector3f cameraPos;
  Vector3f cameraTarget;
  float cameraTransformation[16];
  float fovY = 40.f;
  float projection[16];
  int viewport[4];

  ShadeMode surfaceShadeMode = smoothShading;
  ShadeMode physicsShadeMode = noShading;
  ShadeMode drawingsShadeMode = smoothShading;
  unsigned int renderFlags = enableLights | enableTextures | enableMultisample;

  bool dragging = false;
  DragType dragType;
  Body* dragSelection;
  Vector3f dragStartPos;
  Vector3f interCameraPos;
  DragAndDropPlane dragPlane = xyPlane;
  Vector3f dragPlaneVector;
  DragAndDropMode dragMode = keepDynamics;
  unsigned int dragStartTime;
  int degreeSteps = 15;

  void updateCameraTransformation();

  bool intersectRayAndPlane(const Vector3f& point, const Vector3f& v,
                            const Vector3f& plane, const Vector3f& n,
                            Vector3f& intersection) const;
  Vector3f projectClick(int x, int y) const;
  Body* selectObject(const Vector3f& projectedClick);
  bool intersectClickAndCoordinatePlane(int x, int y, DragAndDropPlane plane, Vector3f& point) const;

  void calcDragPlaneVector();

public:
  // API
  void init(bool hasSharedDisplayLists) override;
  void draw() override;
  void resize(float fovY, unsigned int width, unsigned int height) override;
  void getSize(unsigned int& width, unsigned int& height) const override;
  void setSurfaceShadeMode(ShadeMode shadeMode) override {surfaceShadeMode = shadeMode;}
  ShadeMode getSurfaceShadeMode() const override {return surfaceShadeMode;}
  void setPhysicsShadeMode(ShadeMode shadeMode) override {physicsShadeMode = shadeMode;}
  ShadeMode getPhysicsShadeMode() const override {return physicsShadeMode;}
  void setDrawingsShadeMode(ShadeMode shadeMode) override {drawingsShadeMode = shadeMode;}
  ShadeMode getDrawingsShadeMode() const override {return drawingsShadeMode;}
  void zoom(float change, float x, float y) override;
  void setRenderFlags(unsigned int renderFlags) override {this->renderFlags = renderFlags;}
  unsigned int getRenderFlags() const override {return renderFlags;}
  void setCameraMode(CameraMode) override {}
  CameraMode getCameraMode() const override {return cameraMode;}
  void toggleCameraMode() override {};
  void resetCamera() override;
  void fitCamera() override;
  int getFovY() const override {return int(fovY);}
  void setDragPlane(DragAndDropPlane plane) override;
  DragAndDropPlane getDragPlane() const override {return dragPlane;}
  void setDragMode(DragAndDropMode mode) override {dragMode = mode;}
  DragAndDropMode getDragMode() const override {return dragMode;}
  bool startDrag(int x, int y, DragType type) override;
  SimRobotCore2::Object* getDragSelection() override;
  void setCameraMove(bool, bool, bool, bool) override {}
  bool moveDrag(int x, int y, DragType type) override;
  bool releaseDrag(int x, int y) override;
  void setCamera(const float* pos, const float* target) override;
  void getCamera(float* pos, float* target) override;
  void rotateCamera(float, float) override {}
};
