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
#include <QSignalMapper>
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
  QSurfaceFormat format = Simulation::simulation->renderer.getContext()->format();
  format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
  setFormat(format);

  setFocusPolicy(Qt::StrongFocus);
  grabGesture(Qt::PinchGesture);
  setAttribute(Qt::WA_AcceptTouchEvents);

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
}

void SimObjectWidget::initializeGL()
{
  objectRenderer.init(QOpenGLContext::areSharing(context(), Simulation::simulation->renderer.getContext()));
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
   if(objectRenderer.moveDrag(event->x(),
                              event->y(),
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
    if(objectRenderer.startDrag(event->x(), event->y(), m & Qt::ShiftModifier ? (m & Qt::ControlModifier ? SimObjectRenderer::dragRotateWorld : SimObjectRenderer::dragRotate) : (m & Qt::ControlModifier ? SimObjectRenderer::dragNormalObject : SimObjectRenderer::dragNormal)))
    {
      event->accept();
      update();
    }
  }
}

void SimObjectWidget::mouseReleaseEvent(QMouseEvent* event)
{
  QOpenGLWidget::mouseReleaseEvent(event);

  if(objectRenderer.releaseDrag(event->x(), event->y()))
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
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    const QPointF position = event->position();
#else
    const QPointF position = event->posF();
#endif
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

  QAction* action = menu->addAction(QIcon(":/Icons/page_copy.png"), tr("&Copy"));
  action->setShortcut(QKeySequence(QKeySequence::Copy));
  action->setStatusTip(tr("Copy the rendered object to the clipboard"));
  connect(action, &QAction::triggered, this, &SimObjectWidget::copy);

  return menu;
}

QMenu* SimObjectWidget::createUserMenu() const
{
  QMenu* menu = new QMenu(tr(&object == Simulation::simulation->scene ? "S&cene" : "&Object"));

  QSignalMapper* flagsSignalMapper = new QSignalMapper(menu);
  connect(flagsSignalMapper, &QSignalMapper::mappedInt, this, &SimObjectWidget::toggleRenderFlag);

  {
    QMenu* subMenu = menu->addMenu(tr("&Drag and Drop"));
    QAction* action = subMenu->menuAction();
    action->setIcon(QIcon(":/Icons/DragPlane.png"));
    action->setStatusTip(tr("Select the drag and drop dynamics mode and plane along which operations are performed"));
    QActionGroup* actionGroup = new QActionGroup(subMenu);
    QSignalMapper* signalMapper = new QSignalMapper(subMenu);
    connect(signalMapper, &QSignalMapper::mappedInt, this, &SimObjectWidget::setDragPlane);
    action = subMenu->addAction(tr("X/Y Plane"));
    actionGroup->addAction(action);
    signalMapper->setMapping(action, SimRobotCore2::Renderer::xyPlane);
    action->setShortcut(QKeySequence(Qt::Key_Z));
    action->setCheckable(true);
    connect(action, &QAction::triggered, signalMapper, static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));
    action = subMenu->addAction(tr("X/Z Plane"));
    actionGroup->addAction(action);
    signalMapper->setMapping(action, SimRobotCore2::Renderer::xzPlane);
    action->setShortcut(QKeySequence(Qt::Key_Y));
    action->setCheckable(true);
    connect(action, &QAction::triggered, signalMapper, static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));
    action = subMenu->addAction(tr("Y/Z Plane"));
    actionGroup->addAction(action);
    signalMapper->setMapping(action, SimRobotCore2::Renderer::yzPlane);
    action->setShortcut(QKeySequence(Qt::Key_X));
    action->setCheckable(true);
    connect(action, &QAction::triggered, signalMapper, static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));
    qobject_cast<QAction*>(signalMapper->mapping(objectRenderer.getDragPlane()))->setChecked(true);
    subMenu->addSeparator();
    actionGroup = new QActionGroup(subMenu);
    signalMapper = new QSignalMapper(subMenu);
    connect(signalMapper, &QSignalMapper::mappedInt, this, &SimObjectWidget::setDragMode);
    action = subMenu->addAction(tr("&Keep Dynamics"));
    actionGroup->addAction(action);
    signalMapper->setMapping(action, SimRobotCore2::Renderer::keepDynamics);
    action->setShortcut(QKeySequence(Qt::Key_7));
    action->setCheckable(true);
    connect(action, &QAction::triggered, signalMapper, static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));
    action = subMenu->addAction(tr("&Reset Dynamics"));
    actionGroup->addAction(action);
    signalMapper->setMapping(action, SimRobotCore2::Renderer::resetDynamics);
    action->setShortcut(QKeySequence(Qt::Key_8));
    action->setCheckable(true);
    connect(action, &QAction::triggered, signalMapper, static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));
    action = subMenu->addAction(tr("A&dopt Dynamics"));
    actionGroup->addAction(action);
    signalMapper->setMapping(action, SimRobotCore2::Renderer::adoptDynamics);
    action->setShortcut(QKeySequence(Qt::Key_9));
    action->setCheckable(true);
    connect(action, &QAction::triggered, signalMapper, static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));
    action = subMenu->addAction(tr("&Apply Dynamics"));
    actionGroup->addAction(action);
    signalMapper->setMapping(action, SimRobotCore2::Renderer::applyDynamics);
    action->setShortcut(QKeySequence(Qt::Key_0));
    action->setCheckable(true);
    connect(action, &QAction::triggered, signalMapper, static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));
    qobject_cast<QAction*>(signalMapper->mapping(objectRenderer.getDragMode()))->setChecked(true);
  }

  menu->addSeparator();

  {
    QAction* action = menu->addAction(tr("&Reset Camera"));
    action->setIcon(QIcon(":/Icons/camera.png"));
    action->setShortcut(QKeySequence(Qt::Key_R));
    connect(action, &QAction::triggered, this, &SimObjectWidget::resetCamera);
  }

  {
    QMenu* subMenu = menu->addMenu(tr("&Vertical Opening Angle"));
    QAction* action = subMenu->menuAction();
    action->setIcon(QIcon(":/Icons/opening_angle.png"));
    QActionGroup* actionGroup = new QActionGroup(subMenu);
    QSignalMapper* signalMapper = new QSignalMapper(subMenu);
    connect(signalMapper, &QSignalMapper::mappedInt, this, &SimObjectWidget::setFovY);
    action = subMenu->addAction(tr("&20°"));
    actionGroup->addAction(action);
    signalMapper->setMapping(action, 20);
    action->setShortcut(QKeySequence(Qt::Key_1));
    action->setCheckable(true);
    connect(action, &QAction::triggered, signalMapper, static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));
    action = subMenu->addAction(tr("&40°"));
    actionGroup->addAction(action);
    signalMapper->setMapping(action, 40);
    action->setShortcut(QKeySequence(Qt::Key_2));
    action->setCheckable(true);
    connect(action, &QAction::triggered, signalMapper, static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));
    action = subMenu->addAction(tr("&60°"));
    actionGroup->addAction(action);
    signalMapper->setMapping(action, 60);
    action->setShortcut(QKeySequence(Qt::Key_3));
    action->setCheckable(true);
    connect(action, &QAction::triggered, signalMapper, static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));
    action = subMenu->addAction(tr("&80°"));
    actionGroup->addAction(action);
    signalMapper->setMapping(action, 80);
    action->setShortcut(QKeySequence(Qt::Key_4));
    action->setCheckable(true);
    connect(action, &QAction::triggered, signalMapper, static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));
    action = subMenu->addAction(tr("100°"));
    actionGroup->addAction(action);
    signalMapper->setMapping(action, 100);
    action->setShortcut(QKeySequence(Qt::Key_5));
    action->setCheckable(true);
    connect(action, &QAction::triggered, signalMapper, static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));
    action = subMenu->addAction(tr("120°"));
    actionGroup->addAction(action);
    signalMapper->setMapping(action, 120);
    action->setShortcut(QKeySequence(Qt::Key_6));
    action->setCheckable(true);
    connect(action, &QAction::triggered, signalMapper, static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));
    qobject_cast<QAction*>(signalMapper->mapping(objectRenderer.getFovY()))->setChecked(true);
  }

  menu->addSeparator();

  {
    QMenu* subMenu = menu->addMenu(tr("&Appearances Rendering"));
    QActionGroup* actionGroup = new QActionGroup(subMenu);
    QSignalMapper* signalMapper = new QSignalMapper(subMenu);
    connect(signalMapper, &QSignalMapper::mappedInt, this, &SimObjectWidget::setSurfaceShadeMode);
    QAction* action = subMenu->menuAction();
    action->setIcon(QIcon(":/Icons/layers.png"));
    action->setStatusTip(tr("Select different shading techniques for displaying the scene"));
    action = subMenu->addAction(tr("&Off"));
    actionGroup->addAction(action);
    action->setCheckable(true);
    signalMapper->setMapping(action, SimRobotCore2::Renderer::noShading);
    connect(action, &QAction::triggered, signalMapper, static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));
    action = subMenu->addAction(tr("&Wire Frame"));
    actionGroup->addAction(action);
    action->setShortcut(QKeySequence(static_cast<int>(Qt::CTRL) + static_cast<int>(Qt::Key_W)));
    action->setCheckable(true);
    signalMapper->setMapping(action, SimRobotCore2::Renderer::wireframeShading);
    connect(action, &QAction::triggered, signalMapper, static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));
    action = subMenu->addAction(tr("&Flat Shading"));
    actionGroup->addAction(action);
    action->setShortcut(QKeySequence(static_cast<int>(Qt::CTRL) + static_cast<int>(Qt::Key_F)));
    action->setCheckable(true);
    signalMapper->setMapping(action, SimRobotCore2::Renderer::flatShading);
    connect(action, &QAction::triggered, signalMapper, static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));
    action = subMenu->addAction(tr("&Smooth Shading"));
    actionGroup->addAction(action);
    action->setShortcut(QKeySequence(static_cast<int>(Qt::CTRL) + static_cast<int>(Qt::Key_M)));
    action->setCheckable(true);
    signalMapper->setMapping(action, SimRobotCore2::Renderer::smoothShading);
    connect(action, &QAction::triggered, signalMapper, static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));
    qobject_cast<QAction*>(signalMapper->mapping(objectRenderer.getSurfaceShadeMode()))->setChecked(true);
  }

  {
    QMenu* subMenu = menu->addMenu(tr("&Physics Rendering"));
    QActionGroup* actionGroup = new QActionGroup(subMenu);
    QSignalMapper* signalMapper = new QSignalMapper(subMenu);
    connect(signalMapper, &QSignalMapper::mappedInt, this, &SimObjectWidget::setPhysicsShadeMode);
    QAction* action = subMenu->menuAction();
    action->setStatusTip(tr("Select different shading techniques for displaying the physical representation of objects"));
    action = subMenu->addAction(tr("&Off"));
    actionGroup->addAction(action);
    action->setCheckable(true);
    signalMapper->setMapping(action, SimRobotCore2::Renderer::noShading);
    connect(action, &QAction::triggered, signalMapper, static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));
    action = subMenu->addAction(tr("&Wire Frame"));
    actionGroup->addAction(action);
    action->setCheckable(true);
    signalMapper->setMapping(action, SimRobotCore2::Renderer::wireframeShading);
    connect(action, &QAction::triggered, signalMapper, static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));
    action = subMenu->addAction(tr("&Flat Shading"));
    actionGroup->addAction(action);
    action->setCheckable(true);
    signalMapper->setMapping(action, SimRobotCore2::Renderer::flatShading);
    connect(action, &QAction::triggered, signalMapper, static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));
    action = subMenu->addAction(tr("&Smooth Shading"));
    actionGroup->addAction(action);
    action->setCheckable(true);
    signalMapper->setMapping(action, SimRobotCore2::Renderer::smoothShading);
    connect(action, &QAction::triggered, signalMapper, static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));
    qobject_cast<QAction*>(signalMapper->mapping(objectRenderer.getPhysicsShadeMode()))->setChecked(true);
  }

  {
    QMenu* subMenu = menu->addMenu(tr("&Drawings Rendering"));
    QActionGroup* actionGroup = new QActionGroup(subMenu);
    QSignalMapper* signalMapper = new QSignalMapper(subMenu);
    connect(signalMapper, &QSignalMapper::mappedInt, this, &SimObjectWidget::setDrawingsShadeMode);
    QAction* action = subMenu->menuAction();
    action->setIcon(QIcon(":/Icons/chart_line.png"));
    action->setStatusTip(tr("Select different shading techniques for displaying controller drawings"));
    action = subMenu->addAction(tr("&Off"));
    actionGroup->addAction(action);
    action->setCheckable(true);
    signalMapper->setMapping(action, SimRobotCore2::Renderer::noShading);
    connect(action, &QAction::triggered, signalMapper, static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));
    action = subMenu->addAction(tr("&Wire Frame"));
    actionGroup->addAction(action);
    action->setCheckable(true);
    signalMapper->setMapping(action, SimRobotCore2::Renderer::wireframeShading);
    connect(action, &QAction::triggered, signalMapper, static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));
    action = subMenu->addAction(tr("&Flat Shading"));
    actionGroup->addAction(action);
    action->setCheckable(true);
    signalMapper->setMapping(action, SimRobotCore2::Renderer::flatShading);
    connect(action, &QAction::triggered, signalMapper, static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));
    action = subMenu->addAction(tr("&Smooth Shading"));
    actionGroup->addAction(action);
    action->setCheckable(true);
    signalMapper->setMapping(action, SimRobotCore2::Renderer::smoothShading);
    connect(action, &QAction::triggered, signalMapper, static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));
    qobject_cast<QAction*>(signalMapper->mapping(objectRenderer.getDrawingsShadeMode()))->setChecked(true);

    subMenu->addSeparator();

    subMenu = subMenu->addMenu(tr("&Occlusion"));
    actionGroup = new QActionGroup(subMenu);
    signalMapper = new QSignalMapper(subMenu);
    connect(signalMapper, &QSignalMapper::mappedInt, this, &SimObjectWidget::setDrawingsOcclusion);
    action = subMenu->menuAction();
    action->setStatusTip(tr("Select different drawings occlusion modes"));

    action = subMenu->addAction(tr("&On"));
    actionGroup->addAction(action);
    action->setCheckable(true);
    signalMapper->setMapping(action, SimRobotCore2::Renderer::enableDrawingsOcclusion);
    connect(action, &QAction::triggered, signalMapper, static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));

    action = subMenu->addAction(tr("O&ff"));
    actionGroup->addAction(action);
    action->setCheckable(true);
    signalMapper->setMapping(action, 0);
    connect(action, &QAction::triggered, signalMapper, static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));

    action = subMenu->addAction(tr("&Transparent"));
    actionGroup->addAction(action);
    action->setCheckable(true);
    signalMapper->setMapping(action, SimRobotCore2::Renderer::enableDrawingsTransparentOcclusion);
    connect(action, &QAction::triggered, signalMapper, static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));

    qobject_cast<QAction*>(signalMapper->mapping(objectRenderer.getRenderFlags() & (SimRobotCore2::Renderer::enableDrawingsOcclusion | SimRobotCore2::Renderer::enableDrawingsTransparentOcclusion)))->setChecked(true);
  }

  menu->addSeparator();

  QAction* action = menu->addAction(tr("Enable &Lights"));
  action->setStatusTip(tr("Enable lightning"));
  action->setCheckable(true);
  action->setChecked(objectRenderer.getRenderFlags() & SimRobotCore2::Renderer::enableLights);
  flagsSignalMapper->setMapping(action, SimRobotCore2::Renderer::enableLights);
  connect(action, &QAction::triggered, flagsSignalMapper, static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));

  action = menu->addAction(tr("Enable &Textures"));
  action->setStatusTip(tr("Enable textures"));
  action->setCheckable(true);
  action->setChecked(objectRenderer.getRenderFlags() & SimRobotCore2::Renderer::enableTextures);
  flagsSignalMapper->setMapping(action, SimRobotCore2::Renderer::enableTextures);
  connect(action, &QAction::triggered, flagsSignalMapper, static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));

  action = menu->addAction(tr("Enable &Multisample"));
  action->setStatusTip(tr("Enable multisampling"));
  action->setCheckable(true);
  action->setChecked(objectRenderer.getRenderFlags() & SimRobotCore2::Renderer::enableMultisample);
  flagsSignalMapper->setMapping(action, SimRobotCore2::Renderer::enableMultisample);
  connect(action, &QAction::triggered, flagsSignalMapper, static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));

  menu->addSeparator();

  action = menu->addAction(tr("Show &Coordinate System"));
  action->setStatusTip(tr("Show the coordinate system of the displayed object"));
  action->setCheckable(true);
  action->setChecked(objectRenderer.getRenderFlags() & SimRobotCore2::Renderer::showCoordinateSystem);
  flagsSignalMapper->setMapping(action, SimRobotCore2::Renderer::showCoordinateSystem);
  connect(action, &QAction::triggered, flagsSignalMapper, static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));

  action = menu->addAction(QIcon(":/Icons/transmit_go.png"), tr("Show &Sensors"));
  action->setStatusTip(tr("Show the values of the sensors in the scene view"));
  action->setCheckable(true);
  action->setChecked(objectRenderer.getRenderFlags() & SimRobotCore2::Renderer::showSensors);
  flagsSignalMapper->setMapping(action, SimRobotCore2::Renderer::showSensors);
  connect(action, &QAction::triggered, flagsSignalMapper, static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));

  menu->addSeparator();

  {
    QMenu* subMenu = menu->addMenu(tr("Export as Image..."));
    QSignalMapper* signalMapper = new QSignalMapper(subMenu);
    connect(signalMapper, &QSignalMapper::mappedInt, this, &SimObjectWidget::exportAsImage);
    action = subMenu->addAction(tr("3840x2160"));
    signalMapper->setMapping(action, (3840 << 16) | 2160);
    connect(action, &QAction::triggered, signalMapper, static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));
    action = subMenu->addAction(tr("2880x1620"));
    signalMapper->setMapping(action, (2880 << 16) | 1620);
    connect(action, &QAction::triggered, signalMapper, static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));
    action = subMenu->addAction(tr("1920x1080"));
    signalMapper->setMapping(action, (1920 << 16) | 1080);
    connect(action, &QAction::triggered, signalMapper, static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));
    action = subMenu->addAction(tr("1280x1024"));
    signalMapper->setMapping(action, (1280 << 16) | 1024);
    connect(action, &QAction::triggered, signalMapper, static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));
  }

  return menu;
}
void SimObjectWidget::copy()
{
  QApplication::clipboard()->clear();
  QApplication::clipboard()->setPixmap(QPixmap::fromImage(grabFramebuffer()));
}

void SimObjectWidget::exportAsImage(int resolution)
{
  int width = resolution >> 16;
  int height = resolution & 0xffff;

  QSettings& settings = CoreModule::application->getSettings();
  QString fileName = QFileDialog::getSaveFileName(this,
                                                  tr("Export as Image"), settings.value("ExportDirectory", "").toString(), tr("Portable Network Graphic (*.png)"));
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
