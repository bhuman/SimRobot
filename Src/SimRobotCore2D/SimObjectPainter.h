/**
 * @file SimObjectPainter.h
 *
 * This file declares a class that paints objects.
 *
 * @author Arne Hasselbring
 */

#pragma once

#include "SimRobotCore2D.h"
#include <box2d/b2_math.h>
#include <QPainter>
#include <QPointF>

class Body;
class SimObject;

class SimObjectPainter : public SimRobotCore2D::Painter
{
public:
  /**
   * Constructor.
   * @param simObject The object to paint.
   */
  explicit SimObjectPainter(SimObject& simObject);

  /**
   * Draws the object to a device.
   * @param device The device to which to draw.
   */
  void draw(QPaintDevice* device) override;

  /**
   * Changes the zoom of the painter.
   * @param change The change in zoom.
   * @param x The x coordinate of the point around which the zoom should be added.
   * @param y The y coordinate of the point around which the zoom should be added.
   */
  void zoom(float change, int x, int y) override;

  /**
   * Resizes the painter.
   * @param width The new width.
   * @param height The new height.
   */
  void resize(int width, int height) override;

  /**
   * Starts a dragging process.
   * @param x The x coordinate where the dragging is started.
   * @param y The y coordinate where the dragging is started.
   * @param dragType The drag type.
   */
  void startDrag(int x, int y, DragType dragType) override;

  /**
   * Moves the dragged object.
   * @param x The x coordinate where the drag moved.
   * @param y The y coordinate where the drag moved.
   * @param dragType The drag type.
   * @return Whether there was something to drag.
   */
  bool moveDrag(int x, int y, DragType dragType) override;

  /**
   * Ends a dragging process.
   * @param x The x coordinate where the dragging is ended.
   * @param y The y coordinate where the dragging is ended.
   * @return Whether there was something to drag.
   */
  bool releaseDrag(int x, int y) override;

  /**
   * Returns the currently dragged object.
   * @return The currently dragged object.
   */
  SimRobotCore2D::Object* getDragSelection() override;

  /**
   * Sets the drag mode of the painter.
   * @param dragMode The new drag mode.
   */
  void setDragMode(DragAndDropMode dragMode) override;

  /**
   * Gets the drag mode of the painter.
   * @return The current drag mode.
   */
  DragAndDropMode getDragMode() const override;

  /**
   * Sets the view settings of the painter.
   * @param offset The offset to the center.
   * @param zoomFactor The zoom factor.
   * @param rotation The rotation.
   */
  void setView(const float* offset, float zoomFactor, float rotation) override;

  /**
   * Gets the view settings of the painter.
   * @param offset The offset to the center.
   * @param zoomFactor The zoom factor.
   * @param rotation The rotation.
   */
  void getView(float* offset, float* zoomFactor, float* rotation) const override;

  /** Resets the view of the painter to the default. */
  void resetView() override;

private:
  /**
   * Transforms window coordinates (Qt) to world coordinates (Box2D).
   * @param point A point in Qt window (actually device) coordinates.
   * @return The corresponding point in Box2D world coordinates.
   */
  b2Vec2 windowToWorld(const QPointF& point) const;

  /**
   * Transforms world coordinates (Box2D) to window coordinates (Qt).
   * @param point A point in Box2D world coordinates.
   * @return The corresponding point in Qt window (actually device) coordinates.
   */
  QPointF worldToWindow(const b2Vec2& point) const;

  /**
   * Gets the body that is at a certain point.
   * @param point The point for which the body should be found in Box2D world coordinates.
   * @return The found body or nothing.
   */
  Body* selectObject(const b2Vec2& point);

  /** Updates the transformation matrices derived from \c size, \c offset, \c zoomFactor and \c rotation. */
  void updateTransform();

  SimObject& simObject; /**< The object to paint. */
  QPainter painter; /**< The Qt painter. */

  bool dragging = false; /**< Whether the user is currently dragging something. */
  DragType dragType; /**< The current drag type. */
  Body* dragSelection = nullptr; /**< The object that is being dragged by the user. */
  DragAndDropMode dragMode = keepDynamics; /**< The current drag mode. */
  unsigned int dragStartTime; /**< The timestamp when the current dragging started. */
  b2Vec2 dragStartPos; /**< The point in world coordinates where the dragging started. */

  QSize size; /**< The size of the paint device this painter works on. */
  b2Vec2 offset; /**< The translation between world and window coordinates. */
  float zoomFactor = 1.f; /**< The zoom factor to window coordinates. */
  float rotation = 0.f; /**< The rotation between world and window coordinates. */

  QTransform transform; /**< Transforms world coordinates into window coordinates. */
  QTransform transformInv; /**< Transforms window coordinates into world coordinates. */
};
