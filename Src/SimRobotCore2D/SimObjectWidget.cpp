/**
 * @file SimObjectWidget.cpp
 *
 * This file implements a widget that represents an object.
 *
 * @author Arne Hasselbring
 */

#include "SimObjectWidget.h"
#include "CoreModule.h"
#include "Simulation/SimObject.h"
#include "Simulation/Scene.h"
#include <QApplication>
#include <QMenu>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QSettings>
#include <QWheelEvent>

SimObjectWidget::SimObjectWidget(SimObject& simObject) :
  objectPainter(simObject),
  object(dynamic_cast<SimRobot::Object&>(simObject))
{
  setFocusPolicy(Qt::StrongFocus);
  grabGesture(Qt::PinchGesture);
  setAttribute(Qt::WA_AcceptTouchEvents);

  QSettings& settings = CoreModule::application->getLayoutSettings();
  settings.beginGroup(object.getFullName());
  objectPainter.setDragMode(static_cast<SimRobotCore2D::Painter::DragAndDropMode>(settings.value("dragMode", objectPainter.getDragMode()).toInt()));
  float offset[2];
  offset[0] = settings.value("viewOffsetX", 0.f).toFloat();
  offset[1] = settings.value("viewOffsetY", 0.f).toFloat();
  const float zoomFactor = settings.value("viewZoomFactor", 1.f).toFloat();
  const float rotation = settings.value("viewRotation", 0.f).toFloat();
  objectPainter.setView(offset, zoomFactor, rotation);
  settings.endGroup();
}

SimObjectWidget::~SimObjectWidget()
{
  QSettings& settings = CoreModule::application->getLayoutSettings();
  settings.beginGroup(object.getFullName());
  settings.setValue("dragMode", objectPainter.getDragMode());
  float offset[2], zoomFactor, rotation;
  objectPainter.getView(offset, &zoomFactor, &rotation);
  settings.setValue("viewOffsetX", offset[0]);
  settings.setValue("viewOffsetY", offset[1]);
  settings.setValue("viewZoomFactor", zoomFactor);
  settings.setValue("viewRotation", rotation);
  settings.endGroup();
}

QWidget* SimObjectWidget::getWidget()
{
  return this;
}

void SimObjectWidget::update()
{
  QWidget::update();
}

QMenu* SimObjectWidget::createUserMenu() const
{
  auto* const menu = new QMenu(tr(&object == Simulation::simulation->scene ? "S&cene" : "&Object"));

  {
    QMenu* const subMenu = menu->addMenu(tr("&Drag and Drop"));
    QAction* const action = subMenu->menuAction();
    action->setIcon(QIcon(":/Icons/dragPlane.png"));
    action->setStatusTip(tr("Select the drag and drop dynamics mode"));

    auto* const actionGroup = new QActionGroup(subMenu);
    auto addDragAndDropModeAction = [this, subMenu, actionGroup](SimObjectPainter::DragAndDropMode mode, const char* description, Qt::Key key)
    {
      QAction* const action = subMenu->addAction(tr(description));
      actionGroup->addAction(action);
      action->setShortcut(QKeySequence(key));
      action->setCheckable(true);
      if(objectPainter.getDragMode() == mode)
        action->setChecked(true);
      connect(action, &QAction::triggered, [this, mode]{ const_cast<SimObjectPainter&>(objectPainter).setDragMode(mode); });
    };

    addDragAndDropModeAction(SimObjectPainter::DragAndDropMode::keepDynamics, "&Keep Dynamics", Qt::Key_7);
    addDragAndDropModeAction(SimObjectPainter::DragAndDropMode::resetDynamics, "&Reset Dynamics", Qt::Key_8);
    addDragAndDropModeAction(SimObjectPainter::DragAndDropMode::adoptDynamics, "A&dopt Dynamics", Qt::Key_9);
    addDragAndDropModeAction(SimObjectPainter::DragAndDropMode::applyDynamics, "&Apply Dynamics", Qt::Key_0);
  }

  menu->addSeparator();

  QAction* const action = menu->addAction(tr("&Reset Camera"));
  action->setIcon(QIcon(":/Icons/camera.png"));
  action->setShortcut(QKeySequence(Qt::Key_R));
  connect(action, &QAction::triggered, [this]{ const_cast<SimObjectPainter&>(objectPainter).resetView(); const_cast<SimObjectWidget&>(*this).update(); });

  return menu;
}

void SimObjectWidget::paintEvent(QPaintEvent* event)
{
  QWidget::paintEvent(event);
  objectPainter.draw(this);
}

void SimObjectWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
  QWidget::mouseDoubleClickEvent(event);

  if(event->button() == Qt::LeftButton)
  {
    SimRobot::Object* selectedObject = objectPainter.getDragSelection();
    if(selectedObject)
      CoreModule::application->selectObject(*selectedObject);
  }
}

void SimObjectWidget::mouseMoveEvent(QMouseEvent* event)
{
  QWidget::mouseMoveEvent(event);

  const Qt::KeyboardModifiers m = QApplication::keyboardModifiers();
  if(objectPainter.moveDrag(event->x(), event->y(), m & Qt::ShiftModifier ? SimObjectPainter::dragRotate : SimObjectPainter::dragNormal))
  {
    event->accept();
    update();
  }
}

void SimObjectWidget::mousePressEvent(QMouseEvent* event)
{
  QWidget::mousePressEvent(event);

  if(event->button() == Qt::LeftButton || event->button() == Qt::MidButton)
  {
    const Qt::KeyboardModifiers m = QApplication::keyboardModifiers();
    objectPainter.startDrag(event->x(), event->y(), m & Qt::ShiftModifier ? SimObjectPainter::dragRotate : SimObjectPainter::dragNormal);
    event->accept();
    update();
  }
}

void SimObjectWidget::mouseReleaseEvent(QMouseEvent* event)
{
  QWidget::mouseReleaseEvent(event);

  if(objectPainter.releaseDrag(event->x(), event->y()))
  {
    event->accept();
    update();
  }
}

void SimObjectWidget::resizeEvent(QResizeEvent* event)
{
  QWidget::resizeEvent(event);

  objectPainter.resize(event->size().width(), event->size().height());
  event->accept();
  update();
}

void SimObjectWidget::wheelEvent(QWheelEvent* event)
{
  QWidget::wheelEvent(event);

  objectPainter.zoom(static_cast<float>(event->delta()), event->x(), event->y());
  event->accept();
  update();
}
