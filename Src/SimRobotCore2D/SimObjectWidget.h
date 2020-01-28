/**
 * @file SimObjectWidget.h
 *
 * This file declares a widget that represents an object.
 *
 * @author Arne Hasselbring
 */

#pragma once

#include "SimObjectPainter.h"
#include "SimRobot.h"
#include <QPainter>
#include <QWidget>

class SimObject;

class SimObjectWidget : public QWidget, public SimRobot::Widget
{
  Q_OBJECT

public:
  /**
   * Constructor.
   * @param simObject The object to represent.
   */
  explicit SimObjectWidget(SimObject& simObject);

  /** Destructor. Saves settings. */
  ~SimObjectWidget();

private:
  /**
   * Returns the underlying Qt widget.
   * @return The Qt widget.
   */
  QWidget* getWidget() override;

  /** Tells the widget to update itself. */
  void update() override;

  /**
   * Creates a menu for this widget.
   * @return The new menu.
   */
  QMenu* createUserMenu() const override;

  void paintEvent(QPaintEvent* event) override;
  void mouseDoubleClickEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void resizeEvent(QResizeEvent* event) override;
  void wheelEvent(QWheelEvent* event) override;

  SimObjectPainter objectPainter; /**< The painter for the object. */
  SimRobot::Object& object; /**< The object to represent. */
};
