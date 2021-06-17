/**
 * @file SimObjectPainter.cpp
 *
 * This file implements a class that paints objects.
 *
 * @author Arne Hasselbring
 */

#include "SimObjectPainter.h"
#include "Simulation/Body.h"
#include "Simulation/PhysicalObject.h"
#include "Simulation/Scene.h"
#include "Simulation/Simulation.h"
#include "Platform/Assert.h"
#include "Platform/System.h"
#include "Tools/Math.h"
#include <box2d/b2_fixture.h>
#include <box2d/b2_world.h>
#include <algorithm>
#include <cmath>

SimObjectPainter::SimObjectPainter(SimObject& simObject) :
  simObject(simObject)
{}

void SimObjectPainter::draw(QPaintDevice* device)
{
  painter.begin(device);

  ASSERT(painter.window().size() == size);
  painter.setTransform(transform);

  const auto* const physicalObject = dynamic_cast<const PhysicalObject*>(&simObject);

  if(physicalObject)
  {
    Simulation::simulation->scene->updateTransformations();

    painter.setTransform(physicalObject->transformation.inverted(nullptr), true);
    physicalObject->drawPhysics(painter);
  }

  painter.end();
}

void SimObjectPainter::zoom(float change, int x, int y)
{
  const b2Vec2 beforeZoom = (x >= 0 && y >= 0) ? windowToWorld(QPointF(x, y)) : b2Vec2_zero;
  zoomFactor += 0.1f * change / 120.f;
  zoomFactor = std::max(0.1f, std::min(zoomFactor, 20.f));
  updateTransform();
  if(x >= 0 && y >= 0)
  {
    const b2Vec2 afterZoom = windowToWorld(QPointF(x, y));
    offset += (afterZoom - beforeZoom);
    updateTransform();
  }
}

void SimObjectPainter::resize(int width, int height)
{
  size = QSize(width, height);
  updateTransform();
}

void SimObjectPainter::startDrag(int x, int y, DragType type)
{
  if(dragging)
    return;
  dragSelection = nullptr;
  dragging = true;
  dragType = type;
  dragStartPos = windowToWorld(QPointF(x, y));

  // Drag objects.
  if(&simObject == Simulation::simulation->scene)
  {
    dragSelection = selectObject(dragStartPos);
    if(dragSelection)
    {
      static_cast<SimRobotCore2D::Body*>(dragSelection)->enablePhysics(false);
      if(dragMode == resetDynamics)
        static_cast<SimRobotCore2D::Body*>(dragSelection)->resetDynamics();
      if(dragMode == adoptDynamics)
        dragStartTime = System::getTime();
    }
  }
}

bool SimObjectPainter::moveDrag(int x, int y, DragType)
{
  if(!dragging)
    return false;

  if(!dragSelection)
  {
    const b2Vec2 pointInWorld = windowToWorld(QPointF(x, y));
    if(dragType == dragRotate)
      rotation = normalize(rotation + b2Atan2(pointInWorld.y, pointInWorld.x) - b2Atan2(dragStartPos.y, dragStartPos.x));
    else
      offset += (pointInWorld - dragStartPos);
    updateTransform();
    dragStartPos = windowToWorld(QPointF(x, y));
  }
  else
  {
    if(dragMode == applyDynamics)
      return true;
    const b2Vec2 pointInWorld = windowToWorld(QPointF(x, y));
    if(dragType == dragRotate)
    {
      const b2Vec2 bodyCenter = dragSelection->body->GetPosition();
      const b2Vec2 pointInBody = pointInWorld - bodyCenter;
      const b2Vec2 dragStartPosInBody = dragStartPos - bodyCenter;
      const float angleOffset = b2Atan2(pointInBody.y, pointInBody.x) - b2Atan2(dragStartPosInBody.y, dragStartPosInBody.x);
      dragSelection->body->SetTransform(dragSelection->body->GetPosition(), normalize(dragSelection->body->GetAngle() + angleOffset));
      if(dragMode == adoptDynamics)
      {
        const unsigned int now = System::getTime();
        const float t = std::max(1U, now - dragStartTime) * 0.001f;
        float velocity = (1.f / t) * normalize(angleOffset);
        const float oldVel = dragSelection->body->GetAngularVelocity();
        velocity = 0.3f * velocity + 0.7f * oldVel;
        dragSelection->body->SetAngularVelocity(velocity);
        dragStartTime = now;
      }
    }
    else
    {
      const b2Vec2 positionOffset = pointInWorld - dragStartPos;
      dragSelection->body->SetTransform(dragSelection->body->GetPosition() + positionOffset, dragSelection->body->GetAngle());
      if(dragMode == adoptDynamics)
      {
        const unsigned int now = System::getTime();
        const float t = std::max(1U, now - dragStartTime) * 0.001f;
        b2Vec2 velocity = (1.f / t) * positionOffset;
        const b2Vec2& oldVel = dragSelection->body->GetLinearVelocity();
        velocity = 0.3f * velocity + 0.7f * oldVel;
        dragSelection->body->SetLinearVelocity(velocity);
        dragStartTime = now;
      }
    }
    dragStartPos = pointInWorld;
  }
  return true;
}

bool SimObjectPainter::releaseDrag(int x, int y)
{
  if(!dragging)
    return false;

  if(!dragSelection)
  {
    dragging = false;
    return true;
  }
  else
  {
    if(dragMode == adoptDynamics)
      moveDrag(x, y, dragType);
    else if(dragMode == applyDynamics)
    {
      const b2Vec2 pointInWorld = windowToWorld(QPointF(x, y));

      if(dragType == dragRotate)
      {
        const b2Vec2 bodyCenter = dragSelection->body->GetPosition();
        const b2Vec2 pointInBody = pointInWorld - bodyCenter;
        const b2Vec2 dragStartPosInBody = dragStartPos - bodyCenter;
        const float angleOffset = b2Atan2(pointInBody.y, pointInBody.x) - b2Atan2(dragStartPosInBody.y, dragStartPosInBody.x);
        const float impulse = dragSelection->body->GetInertia() * normalize(angleOffset);
        dragSelection->body->ApplyAngularImpulse(impulse, true);
      }
      else
      {
        const b2Vec2 impulse = dragSelection->body->GetMass() * (pointInWorld - dragStartPos);
        dragSelection->body->ApplyLinearImpulseToCenter(impulse, true);
      }
    }
    static_cast<SimRobotCore2D::Body*>(dragSelection)->enablePhysics(true);
    dragging = false;
    return true;
  }
}

SimRobotCore2D::Object* SimObjectPainter::getDragSelection()
{
  return dragSelection;
}

void SimObjectPainter::setDragMode(DragAndDropMode mode)
{
  dragMode = mode;
}

SimObjectPainter::DragAndDropMode SimObjectPainter::getDragMode() const
{
  return dragMode;
}

void SimObjectPainter::setView(const float* offset, float zoomFactor, float rotation)
{
  this->offset = b2Vec2(offset[0], offset[1]);
  this->zoomFactor = zoomFactor;
  this->rotation = rotation;
  updateTransform();
}

void SimObjectPainter::getView(float* offset, float* zoomFactor, float* rotation) const
{
  if(offset)
  {
    offset[0] = this->offset.x;
    offset[1] = this->offset.y;
  }
  if(zoomFactor)
    *zoomFactor = this->zoomFactor;
  if(rotation)
    *rotation = this->rotation;
}

void SimObjectPainter::resetView()
{
  offset.SetZero();
  zoomFactor = 1.f;
  rotation = 0.f;
  updateTransform();
}

b2Vec2 SimObjectPainter::windowToWorld(const QPointF& point) const
{
  const QPointF result = transformInv.map(point);
  return b2Vec2(static_cast<float>(result.x()), static_cast<float>(result.y()));
}

QPointF SimObjectPainter::worldToWindow(const b2Vec2& point) const
{
  return transform.map(QPointF(point.x, point.y));
}

Body* SimObjectPainter::selectObject(const b2Vec2& point)
{
  if(&simObject != Simulation::simulation->scene)
    return nullptr;

  class Callback : public b2QueryCallback
  {
  public:
    Callback(const b2Vec2& point) :
        point(point)
    {}

    Body* result = nullptr; /**< The body that has been found. */

  private:
    bool ReportFixture(b2Fixture* fixture) override
    {
      b2Body* const body = fixture->GetBody();
      if(body == Simulation::simulation->staticBody || !fixture->GetShape()->TestPoint(body->GetTransform(), point))
        return true;
      result = reinterpret_cast<Body*>(body->GetUserData().pointer);
      return false;
    }

    b2Vec2 point; /**< The point in the world at which a body is queried. */
  };

  Callback callback(point);
  b2AABB boundingBox;
  boundingBox.lowerBound = point;
  boundingBox.upperBound = point;
  Simulation::simulation->world->QueryAABB(&callback, boundingBox);

  if(!callback.result)
    return nullptr;
  return callback.result->rootBody;
}

void SimObjectPainter::updateTransform()
{
  const float xScale = static_cast<float>(size.width()) / (11.f + 0.2f); // TODO: get the dimensions of the object from somewhere
  const float yScale = static_cast<float>(size.height()) / (8.f + 0.2f);
  const float scale = zoomFactor * std::min(xScale, yScale);
  const float offsetX = static_cast<float>(size.width()) * 0.5f;
  const float offsetY = static_cast<float>(size.height()) * 0.5f;
  transform = QTransform(1, 0, 0, -1, offsetX, offsetY);
  transform.scale(scale, scale);
  transform.rotateRadians(rotation);
  transform.translate(offset.x, offset.y);
  transformInv = transform.inverted(nullptr);
}
