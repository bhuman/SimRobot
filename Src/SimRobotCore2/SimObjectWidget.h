/**
 * @file SimObjectWidget.h
 * Declaration of class SimObjectWidget
 * @author Colin Graf
 */

#pragma once

#include <QOpenGLWidget>

#include "SimRobotCore2.h"
#include "SimObjectRenderer.h"

class SimObject;
class Simulation;

/**
 * @class SimObjectWidget
 * A class that implements the 3D-view for simulated objects
 */
class SimObjectWidget : public QOpenGLWidget, public SimRobot::Widget
{
  Q_OBJECT

public:
  /**
   * Constructor
   * @param simObject The object that should be displayed
   */
  SimObjectWidget(SimObject& simObject);

  /** Destructor */
  ~SimObjectWidget();

private:
  const SimRobot::Object& object; /**< The object that should be displayed */
  SimObjectRenderer objectRenderer; /**< For rendering the object */
  int fovY;

  bool wKey, aKey, sKey, dKey;

  QWidget* getWidget() override {return this;}
  void update() override;
  QMenu* createEditMenu() const override;
  QMenu* createUserMenu() const override;

  void initializeGL() override;
  void paintGL() override;
  void resizeGL(int width, int height) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseDoubleClickEvent(QMouseEvent* event) override;
  void keyPressEvent(QKeyEvent* event) override;
  void keyReleaseEvent(QKeyEvent* event) override;
  bool event(QEvent* event) override;
  void wheelEvent(QWheelEvent* event) override;
  QSize sizeHint() const override {return QSize(320, 240);}

private slots:
  void copy();
  void setSurfaceShadeMode(int style);
  void setPhysicsShadeMode(int style);
  void setDrawingsShadeMode(int style);
  void setDrawingsOcclusion(int flag);
  void setCameraMode(int mode);
  void setFovY(int fovY);
  void setDragPlane(int plane);
  void setDragMode(int mode);
  void resetCamera();
  void toggleCameraMode();
  void toggleRenderFlag(int flag);
  void exportAsImage(int width, int height);
};
