/**
 * @file SimObjectWidget.cpp
 * Implementation of class SimObjectWidget
 * @author Colin Graf
 */

#include <QMouseEvent>
#include <QActionGroup>
#include <QApplication>
#include <QSettings>
#include <QMenu>
#include <QPinchGesture>
#include <QClipboard>
#include <QFileDialog>

#include "SimObjectWidget.h"
#include "CoreModule.h"
#include "Platform/Assert.h"
#include "Simulation/Simulation.h"
#include "Simulation/Scene.h"

#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>

SimObjectWidget::SimObjectWidget(SimObject& simObject) : QOpenGLWidget(),
  object(dynamic_cast<SimRobot::Object&>(simObject)), objectRenderer(simObject),
  wKey(false), aKey(false), sKey(false), dKey(false)
{
  QSurfaceFormat format = Simulation::simulation->graphicsContext.getOffscreenContext()->format();
  format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
  setFormat(format);

  setFocusPolicy(Qt::StrongFocus);
  grabGesture(Qt::PinchGesture);

  // load layout settings
  QSettings* settings = &CoreModule::application->getLayoutSettings();
  settings->beginGroup(object.getFullName());

  objectRenderer.setSurfaceShadeMode(SimRobotCore2::Renderer::ShadeMode(settings->value("SurfaceShadeMode", int(objectRenderer.getSurfaceShadeMode())).toInt()));
  objectRenderer.setPhysicsShadeMode(SimRobotCore2::Renderer::ShadeMode(settings->value("PhysicsShadeMode", int(objectRenderer.getPhysicsShadeMode())).toInt()));
  objectRenderer.setDrawingsShadeMode(SimRobotCore2::Renderer::ShadeMode(settings->value("DrawingsShadeMode", int(objectRenderer.getDrawingsShadeMode())).toInt()));
  objectRenderer.setCameraMode(SimRobotCore2::Renderer::CameraMode(settings->value("CameraMode", int(objectRenderer.getCameraMode())).toInt()));
  fovY = settings->value("FovY", objectRenderer.getFovY()).toInt();
  objectRenderer.setDragPlane(SimRobotCore2::Renderer::DragAndDropPlane(settings->value("DragPlane", int(objectRenderer.getDragPlane())).toInt()));
  objectRenderer.setDragMode(SimRobotCore2::Renderer::DragAndDropMode(settings->value("DragMode", int(objectRenderer.getDragMode())).toInt()));
  objectRenderer.setRenderFlags(settings->value("RenderFlags", objectRenderer.getRenderFlags()).toInt());

  float pos[3];
  float target[3];
  objectRenderer.getCamera(pos, target);
  pos[0] = settings->value("cameraPosX", pos[0]).toFloat();
  pos[1] = settings->value("cameraPosY", pos[1]).toFloat();
  pos[2] = settings->value("cameraPosZ", pos[2]).toFloat();
  target[0] = settings->value("cameraTargetX", target[0]).toFloat();
  target[1] = settings->value("cameraTargetY", target[1]).toFloat();
  target[2] = settings->value("cameraTargetZ", target[2]).toFloat();
  objectRenderer.setCamera(pos, target);

  settings->endGroup();
}

SimObjectWidget::~SimObjectWidget()
{
  // save layout settings
  QSettings* settings = &CoreModule::application->getLayoutSettings();
  settings->beginGroup(object.getFullName());

  settings->setValue("SurfaceShadeMode", int(objectRenderer.getSurfaceShadeMode()));
  settings->setValue("PhysicsShadeMode", int(objectRenderer.getPhysicsShadeMode()));
  settings->setValue("DrawingsShadeMode", int(objectRenderer.getDrawingsShadeMode()));
  settings->setValue("CameraMode", int(objectRenderer.getCameraMode()));
  settings->setValue("FovY", objectRenderer.getFovY());
  settings->setValue("DragPlane", int(objectRenderer.getDragPlane()));
  settings->setValue("DragMode", int(objectRenderer.getDragMode()));
  settings->setValue("RenderFlags", objectRenderer.getRenderFlags());

  float pos[3];
  float target[3];
  objectRenderer.getCamera(pos, target);

  settings->setValue("cameraPosX", pos[0]);
  settings->setValue("cameraPosY", pos[1]);
  settings->setValue("cameraPosZ", pos[2]);
  settings->setValue("cameraTargetX", target[0]);
  settings->setValue("cameraTargetY", target[1]);
  settings->setValue("cameraTargetZ", target[2]);

  settings->endGroup();

  makeCurrent();
  objectRenderer.destroy();
}

void SimObjectWidget::initializeGL()
{
  objectRenderer.init();
}

void SimObjectWidget::paintGL()
{
  objectRenderer.draw();
}

void SimObjectWidget::resizeGL(int width, int height)
{
  objectRenderer.resize(fovY, width, height);
}

void SimObjectWidget::mouseMoveEvent(QMouseEvent* event)
{
  QOpenGLWidget::mouseMoveEvent(event);

  const Qt::KeyboardModifiers m = QApplication::keyboardModifiers();
  const QPointF position = event->position();
  if(objectRenderer.moveDrag(static_cast<int>(position.x()),
                             static_cast<int>(position.y()),
                             m & Qt::ShiftModifier
                             ? (m & Qt::ControlModifier
                                ? SimObjectRenderer::dragRotateWorld
                                : SimObjectRenderer::dragRotate)
                             : (m & Qt::ControlModifier
                                ? SimObjectRenderer::dragNormalObject
                                : SimObjectRenderer::dragNormal)))
  {
    event->accept();
    update();
  }
}

void SimObjectWidget::mousePressEvent(QMouseEvent* event)
{
  QOpenGLWidget::mousePressEvent(event);

  if(event->button() == Qt::LeftButton || event->button() == Qt::MiddleButton)
  {
    const Qt::KeyboardModifiers m = QApplication::keyboardModifiers();
    const QPointF position = event->position();
    if(objectRenderer.startDrag(static_cast<int>(position.x()), static_cast<int>(position.y()), m & Qt::ShiftModifier ? (m & Qt::ControlModifier ? SimObjectRenderer::dragRotateWorld : SimObjectRenderer::dragRotate) : (m & Qt::ControlModifier ? SimObjectRenderer::dragNormalObject : SimObjectRenderer::dragNormal)))
    {
      event->accept();
      update();
    }
  }
}

void SimObjectWidget::mouseReleaseEvent(QMouseEvent* event)
{
  QOpenGLWidget::mouseReleaseEvent(event);

  const QPointF position = event->position();
  if(objectRenderer.releaseDrag(static_cast<int>(position.x()), static_cast<int>(position.y())))
  {
    event->accept();
    update();
  }
}

void SimObjectWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
  QOpenGLWidget::mouseDoubleClickEvent(event);

  if(event->button() == Qt::LeftButton)
  {
    SimRobotCore2::Object* selectedObject = objectRenderer.getDragSelection();
    if(selectedObject)
      CoreModule::application->selectObject(*selectedObject);
  }
}

void SimObjectWidget::keyPressEvent(QKeyEvent* event)
{
  if(event->modifiers() != 0)
  {
    QOpenGLWidget::keyPressEvent(event);
    return;
  }

  switch(event->key())
  {
    case Qt::Key_PageUp:
    case Qt::Key_Plus:
      event->accept();
      objectRenderer.zoom(-100.f, -1.f, -1.f);
      update();
      break;

    case Qt::Key_PageDown:
    case Qt::Key_Minus:
      event->accept();
      objectRenderer.zoom(100.f, -1.f, -1.f);
      update();
      break;

    case Qt::Key_W:
    case Qt::Key_A:
    case Qt::Key_S:
    case Qt::Key_D:
      event->accept();
      switch(event->key())
      {
        case Qt::Key_W:
          wKey = true;
          break;
        case Qt::Key_A:
          aKey = true;
          break;
        case Qt::Key_S:
          sKey = true;
          break;
        case Qt::Key_D:
          dKey = true;
          break;
      }
      objectRenderer.setCameraMove(aKey, dKey, wKey, sKey);
      update();
      break;

    default:
      QOpenGLWidget::keyPressEvent(event);
      break;
  }
}

void SimObjectWidget::keyReleaseEvent(QKeyEvent* event)
{
  if(event->modifiers() != 0)
  {
    QOpenGLWidget::keyReleaseEvent(event);
    return;
  }

  switch(event->key())
  {
    case Qt::Key_W:
    case Qt::Key_A:
    case Qt::Key_S:
    case Qt::Key_D:
      event->accept();
      update();
      if(!event->isAutoRepeat())
      {
        switch(event->key())
        {
          case Qt::Key_W:
            wKey = false;
            break;
          case Qt::Key_A:
            aKey = false;
            break;
          case Qt::Key_S:
            sKey = false;
            break;
          case Qt::Key_D:
            dKey = false;
            break;
        }
        objectRenderer.setCameraMove(aKey, dKey, wKey, sKey);
      }
      break;

    default:
      QOpenGLWidget::keyReleaseEvent(event);
      break;
  }
}

bool SimObjectWidget::event(QEvent* event)
{
  if(event->type() == QEvent::Gesture)
  {
    QPinchGesture* pinch = static_cast<QPinchGesture*>(static_cast<QGestureEvent*>(event)->gesture(Qt::PinchGesture));
    if(pinch && (pinch->changeFlags() & QPinchGesture::ScaleFactorChanged))
    {
#ifdef FIX_MACOS_PINCH_SCALE_RELATIVE_BUG
      pinch->setLastScaleFactor(1.f);
#endif
      float change = static_cast<float>(pinch->scaleFactor() > pinch->lastScaleFactor()
                                        ? -pinch->scaleFactor() / pinch->lastScaleFactor()
                                        : pinch->lastScaleFactor() / pinch->scaleFactor());
      objectRenderer.zoom(change * 100.f, -1, -1);
      update();
      return true;
    }
  }
  return QOpenGLWidget::event(event);
}

void SimObjectWidget::wheelEvent(QWheelEvent* event)
{
  if(event->angleDelta().y() != 0.f)
  {
    const QPointF position = event->position();
    objectRenderer.zoom(event->angleDelta().y(), static_cast<float>(position.x()),
                        static_cast<float>(position.y()));
    update();
    event->accept();
    return;
  }
  QOpenGLWidget::wheelEvent(event);
}

void SimObjectWidget::update()
{
  QOpenGLWidget::update();
}

QMenu* SimObjectWidget::createEditMenu() const
{
  QMenu* menu = new QMenu(tr("&Edit"));
  QIcon icon(":/Icons/icons8-copy-to-clipboard-50.png");
  icon.setIsMask(true);
  QAction* action = menu->addAction(icon, tr("&Copy"));
  action->setShortcut(QKeySequence(QKeySequence::Copy));
  action->setStatusTip(tr("Copy the rendered object to the clipboard"));
  connect(action, &QAction::triggered, this, &SimObjectWidget::copy);

  return menu;
}

QMenu* SimObjectWidget::createUserMenu() const
{
  QMenu* menu = new QMenu(tr(&object == Simulation::simulation->scene ? "S&cene" : "&Object"));

  {
    QMenu* subMenu = menu->addMenu(tr("&Drag and Drop"));
    QAction* action = subMenu->menuAction();
    QIcon icon(":/Icons/icons8-coordinate-system-50.png");
    icon.setIsMask(true);
    action->setIcon(icon);
    action->setStatusTip(tr("Select the drag and drop dynamics mode and plane along which operations are performed"));
    QActionGroup* actionGroup = new QActionGroup(subMenu);
    auto addPlaneAction = [this, subMenu, actionGroup](const char* label, Qt::Key key, SimRobotCore2::Renderer::DragAndDropPlane plane)
    {
      auto* action = subMenu->addAction(tr(label));
      actionGroup->addAction(action);
      action->setShortcut(QKeySequence(key));
      action->setCheckable(true);
      action->setChecked(objectRenderer.getDragPlane() == plane);
      connect(action, &QAction::triggered, this, [this, plane]{ const_cast<SimObjectWidget*>(this)->setDragPlane(plane); });
    };
    addPlaneAction("X/Y Plane", Qt::Key_Z, SimRobotCore2::Renderer::xyPlane);
    addPlaneAction("X/Z Plane", Qt::Key_Y, SimRobotCore2::Renderer::xzPlane);
    addPlaneAction("Y/Z Plane", Qt::Key_X, SimRobotCore2::Renderer::yzPlane);
    subMenu->addSeparator();
    actionGroup = new QActionGroup(subMenu);
    auto addModeAction = [this, subMenu, actionGroup](const char* label, Qt::Key key, SimRobotCore2::Renderer::DragAndDropMode mode)
    {
      auto* action = subMenu->addAction(tr(label));
      actionGroup->addAction(action);
      action->setShortcut(QKeySequence(key));
      action->setCheckable(true);
      action->setChecked(objectRenderer.getDragMode() == mode);
      connect(action, &QAction::triggered, this, [this, mode]{ const_cast<SimObjectWidget*>(this)->setDragMode(mode); });
    };
    addModeAction("&Keep Dynamics", Qt::Key_7, SimRobotCore2::Renderer::keepDynamics);
    addModeAction("&Reset Dynamics", Qt::Key_8, SimRobotCore2::Renderer::resetDynamics);
    addModeAction("A&dopt Dynamics", Qt::Key_9, SimRobotCore2::Renderer::adoptDynamics);
    addModeAction("&Apply Dynamics", Qt::Key_0, SimRobotCore2::Renderer::applyDynamics);
  }

  menu->addSeparator();

  {
    QAction* action = menu->addAction(tr("&Reset Camera"));
    QIcon icon(":/Icons/icons8-camera-50.png");
    icon.setIsMask(true);
    action->setIcon(icon);
    action->setShortcut(QKeySequence(Qt::Key_R));
    connect(action, &QAction::triggered, this, &SimObjectWidget::resetCamera);
  }

  {
    QMenu* subMenu = menu->addMenu(tr("&Vertical Opening Angle"));
    QAction* action = subMenu->menuAction();
    QIcon icon(":/Icons/icons8-focal-length-50.png");
    icon.setIsMask(true);
    action->setIcon(icon);
    QActionGroup* actionGroup = new QActionGroup(subMenu);
    auto addFovYAction = [this, subMenu, actionGroup](const char* label, Qt::Key key, int fovY)
    {
      auto* action = subMenu->addAction(tr(label));
      actionGroup->addAction(action);
      action->setShortcut(QKeySequence(key));
      action->setCheckable(true);
      action->setChecked(objectRenderer.getFovY() == fovY);
      connect(action, &QAction::triggered, this, [this, fovY]{ const_cast<SimObjectWidget*>(this)->setFovY(fovY); });
    };
    addFovYAction("&20°", Qt::Key_1, 20);
    addFovYAction("&40°", Qt::Key_2, 40);
    addFovYAction("&60°", Qt::Key_3, 60);
    addFovYAction("&80°", Qt::Key_4, 80);
    addFovYAction("100°", Qt::Key_5, 100);
    addFovYAction("120°", Qt::Key_6, 120);
  }

  menu->addSeparator();

  {
    QMenu* subMenu = menu->addMenu(tr("&Appearances Rendering"));
    QActionGroup* actionGroup = new QActionGroup(subMenu);
    QAction* action = subMenu->menuAction();
    QIcon icon(":/Icons/icons8-layers-50.png");
    icon.setIsMask(true);
    action->setIcon(icon);
    action->setStatusTip(tr("Select different shading techniques for displaying the scene"));
    auto addShadingAction = [this, subMenu, actionGroup](const char* label, Qt::Key key, SimRobotCore2::Renderer::ShadeMode shading)
    {
      auto* action = subMenu->addAction(tr(label));
      actionGroup->addAction(action);
      if(key)
        action->setShortcut(QKeySequence(static_cast<int>(Qt::CTRL) + static_cast<int>(key)));
      action->setCheckable(true);
      action->setChecked(objectRenderer.getSurfaceShadeMode() == shading);
      connect(action, &QAction::triggered, this, [this, shading]{ const_cast<SimObjectWidget*>(this)->setSurfaceShadeMode(shading); });
    };
    addShadingAction("&Off", Qt::Key(0), SimRobotCore2::Renderer::noShading);
    addShadingAction("&Wire Frame", Qt::Key_W, SimRobotCore2::Renderer::wireframeShading);
    addShadingAction("&Flat Shading", Qt::Key_F, SimRobotCore2::Renderer::flatShading);
    addShadingAction("&Smooth Shading", Qt::Key_M, SimRobotCore2::Renderer::smoothShading);
  }

  {
    QMenu* subMenu = menu->addMenu(tr("&Physics Rendering"));
    QActionGroup* actionGroup = new QActionGroup(subMenu);
    QAction* action = subMenu->menuAction();
    action->setStatusTip(tr("Select different shading techniques for displaying the physical representation of objects"));
    auto addShadingAction = [this, subMenu, actionGroup](const char* label, SimRobotCore2::Renderer::ShadeMode shading)
    {
      auto* action = subMenu->addAction(tr(label));
      actionGroup->addAction(action);
      action->setCheckable(true);
      action->setChecked(objectRenderer.getPhysicsShadeMode() == shading);
      connect(action, &QAction::triggered, this, [this, shading]{ const_cast<SimObjectWidget*>(this)->setPhysicsShadeMode(shading); });
    };
    addShadingAction("&Off", SimRobotCore2::Renderer::noShading);
    addShadingAction("&Wire Frame", SimRobotCore2::Renderer::wireframeShading);
    addShadingAction("&Flat Shading", SimRobotCore2::Renderer::flatShading);
    addShadingAction("&Smooth Shading", SimRobotCore2::Renderer::smoothShading);
  }

  {
    QMenu* subMenu = menu->addMenu(tr("&Drawings Rendering"));
    QActionGroup* actionGroup = new QActionGroup(subMenu);
    QAction* action = subMenu->menuAction();
    QIcon icon(":/Icons/icons8-line-chart-50.png");
    icon.setIsMask(true);
    action->setIcon(icon);
    action->setStatusTip(tr("Select different shading techniques for displaying controller drawings"));
    auto addShadingAction = [this, subMenu, actionGroup](const char* label, SimRobotCore2::Renderer::ShadeMode shading)
    {
      auto* action = subMenu->addAction(tr(label));
      actionGroup->addAction(action);
      action->setCheckable(true);
      action->setChecked(objectRenderer.getDrawingsShadeMode() == shading);
      connect(action, &QAction::triggered, this, [this, shading]{ const_cast<SimObjectWidget*>(this)->setDrawingsShadeMode(shading); });
    };
    addShadingAction("&Off", SimRobotCore2::Renderer::noShading);
    addShadingAction("&Wire Frame", SimRobotCore2::Renderer::wireframeShading);
    addShadingAction("&Filled", SimRobotCore2::Renderer::flatShading);

    subMenu->addSeparator();

    subMenu = subMenu->addMenu(tr("&Occlusion"));
    actionGroup = new QActionGroup(subMenu);
    action = subMenu->menuAction();
    action->setStatusTip(tr("Select different drawings occlusion modes"));

    auto addOcclusionAction = [this, subMenu, actionGroup](const char* label, SimRobotCore2::Renderer::RenderFlags flag)
    {
      auto* action = subMenu->addAction(tr(label));
      actionGroup->addAction(action);
      action->setCheckable(true);
      action->setChecked((objectRenderer.getRenderFlags() & (SimRobotCore2::Renderer::enableDrawingsOcclusion | SimRobotCore2::Renderer::enableDrawingsTransparentOcclusion)) == flag);
      connect(action, &QAction::triggered, this, [this, flag]{ const_cast<SimObjectWidget*>(this)->setDrawingsOcclusion(flag); });
    };

    addOcclusionAction("&On", SimRobotCore2::Renderer::enableDrawingsOcclusion);
    addOcclusionAction("&Off", SimRobotCore2::Renderer::RenderFlags(0));
    addOcclusionAction("&Transparent", SimRobotCore2::Renderer::enableDrawingsTransparentOcclusion);
  }

  menu->addSeparator();

  auto addRenderFlagAction = [this, menu](const char* label, const char* tip, SimRobotCore2::Renderer::RenderFlags flag, const char* icon = nullptr)
  {

    QAction* action;
    if(icon)
    {
      QIcon qIcon(icon);
      qIcon.setIsMask(true);
      action = menu->addAction(qIcon, tr(label));
    }
    else
      action = menu->addAction(tr(label));
    action->setStatusTip(tr(tip));
    action->setCheckable(true);
    action->setChecked(objectRenderer.getRenderFlags() & flag);
    connect(action, &QAction::triggered, this, [this, flag]{ const_cast<SimObjectWidget*>(this)->toggleRenderFlag(flag); });
  };

  addRenderFlagAction("Enable &Lights", "Enable lighting", SimRobotCore2::Renderer::enableLights);
  addRenderFlagAction("Enable &Textures", "Enable textures", SimRobotCore2::Renderer::enableTextures);
  addRenderFlagAction("Enable &Multisample", "Enable multisampling", SimRobotCore2::Renderer::enableMultisample);

  menu->addSeparator();

  addRenderFlagAction("Show &Coordinate System", "Show the coordinate system of the displayed object", SimRobotCore2::Renderer::showCoordinateSystem);
  addRenderFlagAction("Show &Sensors", "Show the values of the sensors in the scene view", SimRobotCore2::Renderer::showSensors, ":/Icons/icons8-speed-50.png");

  menu->addSeparator();

  {
    QMenu* subMenu = menu->addMenu(tr("Export as Image..."));
    auto* action = subMenu->addAction(tr("3840x2160"));
    connect(action, &QAction::triggered, this, [this]{ const_cast<SimObjectWidget*>(this)->exportAsImage(3840, 2160); });
    action = subMenu->addAction(tr("2880x1620"));
    connect(action, &QAction::triggered, this, [this]{ const_cast<SimObjectWidget*>(this)->exportAsImage(2880, 1620); });
    action = subMenu->addAction(tr("1920x1080"));
    connect(action, &QAction::triggered, this, [this]{ const_cast<SimObjectWidget*>(this)->exportAsImage(1920, 1080); });
    action = subMenu->addAction(tr("1280x1024"));
    connect(action, &QAction::triggered, this, [this]{ const_cast<SimObjectWidget*>(this)->exportAsImage(1280, 1024); });
  }

  return menu;
}

void SimObjectWidget::copy()
{
  QApplication::clipboard()->setImage(grabFramebuffer());
}

void SimObjectWidget::exportAsImage(int width, int height)
{
  QSettings& settings = CoreModule::application->getSettings();
  QString fileName = QFileDialog::getSaveFileName(this,
                                                  tr("Export as Image"), settings.value("ExportDirectory", "").toString(), tr("Portable Network Graphic (*.png)")
#ifdef LINUX
                                                  , nullptr, QFileDialog::DontUseNativeDialog
#endif
                                                  );
  if(fileName.isEmpty())
    return;
  settings.setValue("ExportDirectory", QFileInfo(fileName).dir().path());

  QImage image;
  {
    unsigned int winWidth, winHeight;
    objectRenderer.getSize(winWidth, winHeight);
    makeCurrent();

    // render object using a temporary framebuffer
    QOpenGLFramebufferObject framebuffer(width, height, QOpenGLFramebufferObject::Depth);
    framebuffer.bind();
    objectRenderer.resize(fovY, width, height);
    objectRenderer.draw();
    image = framebuffer.toImage();
    framebuffer.release();

    objectRenderer.resize(fovY, winWidth, winHeight);
  }

  image.save(fileName);
}

void SimObjectWidget::setSurfaceShadeMode(int style)
{
  objectRenderer.setSurfaceShadeMode(SimRobotCore2::Renderer::ShadeMode(style));
  update();
}

void SimObjectWidget::setPhysicsShadeMode(int style)
{
  objectRenderer.setPhysicsShadeMode(SimRobotCore2::Renderer::ShadeMode(style));
  update();
}

void SimObjectWidget::setDrawingsShadeMode(int style)
{
  objectRenderer.setDrawingsShadeMode(SimRobotCore2::Renderer::ShadeMode(style));
  update();
}

void SimObjectWidget::setDrawingsOcclusion(int flag)
{
  unsigned int flags = objectRenderer.getRenderFlags();
  flags &= ~(SimRobotCore2::Renderer::enableDrawingsOcclusion | SimRobotCore2::Renderer::enableDrawingsTransparentOcclusion);
  flags |= flag;
  objectRenderer.setRenderFlags(flags);
  update();
}

void SimObjectWidget::setCameraMode(int mode)
{
  objectRenderer.setCameraMode(SimRobotCore2::Renderer::CameraMode(mode));
  update();
}

void SimObjectWidget::setFovY(int fovY)
{
  unsigned int width, height;
  this->fovY = fovY;
  objectRenderer.getSize(width, height);
  makeCurrent();
  objectRenderer.resize(fovY, width, height);
  update();
}

void SimObjectWidget::setDragPlane(int plane)
{
  objectRenderer.setDragPlane(SimRobotCore2::Renderer::DragAndDropPlane(plane));
  update();
}

void SimObjectWidget::setDragMode(int mode)
{
  objectRenderer.setDragMode(SimRobotCore2::Renderer::DragAndDropMode(mode));
  update();
}

void SimObjectWidget::resetCamera()
{
  objectRenderer.resetCamera();
  update();
}

void SimObjectWidget::toggleCameraMode()
{
  objectRenderer.toggleCameraMode();
  update();
}

void SimObjectWidget::fitCamera()
{
  /*
  objectRenderer.fitCamera();
  update();
   */
}

void SimObjectWidget::toggleRenderFlag(int flag)
{
  unsigned int flags = objectRenderer.getRenderFlags();
  if(flags & flag)
    flags &= ~flag;
  else
    flags |= flag;
  objectRenderer.setRenderFlags(flags);

  update();
}
