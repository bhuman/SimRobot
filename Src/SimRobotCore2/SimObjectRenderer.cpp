/**
 * @file SimObjectRenderer.cpp
 * Declaration of class SimObjectRenderer
 * @author Colin Graf
 */

#include "SimObjectRenderer.h"
#include "Platform/Assert.h"
#include "Platform/System.h"
#include "Simulation/Body.h"
#include "Simulation/Scene.h"
#include "Simulation/Simulation.h"
#include "Tools/Math.h"
#include "Tools/Math/Constants.h"
#include "Tools/Math/Rotation.h"
#include "Tools/OpenGLTools.h"
#include <ode/collision.h>
#include <ode/objects.h>
#include <QOpenGLFunctions_3_3_Core>

SimObjectRenderer::SimObjectRenderer(SimObject& simObject) :
  simObject(simObject),
  cameraMode(targetCam), defaultCameraPos(3.f, 6.f, 4.f), cameraPos(defaultCameraPos), cameraTarget(Vector3f::Zero())
{
}

SimObjectRenderer::~SimObjectRenderer()
{
  ASSERT(!initialized);
}

void SimObjectRenderer::resetCamera()
{
  cameraPos = defaultCameraPos;
  cameraTarget = Vector3f::Zero();
  updateCameraTransformation();
}

void SimObjectRenderer::updateCameraTransformation()
{
  static const Vector3f cameraUpVector(0.f, 0.f, 1.f);
  OpenGLTools::computeCameraTransformation(cameraPos, cameraTarget, cameraUpVector, cameraTransformation);
}

void SimObjectRenderer::init()
{
  ASSERT(!initialized);
  Simulation::simulation->graphicsContext.createGraphics();
  if(Simulation::simulation->scene->drawingManager)
  {
    Simulation::simulation->scene->drawingManager->registerContext();
    registeredAtManager = true;
  }
  initialized = true;
  calcDragPlaneVector();
}

void SimObjectRenderer::destroy()
{
  if(initialized)
  {
    if(registeredAtManager)
    {
      ASSERT(Simulation::simulation->scene->drawingManager);
      Simulation::simulation->scene->drawingManager->unregisterContext();
      registeredAtManager = false;
    }
    Simulation::simulation->graphicsContext.destroyGraphics();
    initialized = false;
  }
}

void SimObjectRenderer::draw()
{
  // make sure transformations of movable bodies are up-to-date
  Simulation::simulation->scene->updateTransformations();

  if(dragging && dragSelection)
  {
    Pose3f& dragPlanePose = Simulation::simulation->dragPlanePose;
    if(dragType == dragRotate || dragType == dragNormalObject)
      dragPlanePose = dragSelection->poseInParent;
    else
      dragPlanePose = Pose3f(dragSelection->poseInParent.translation);

    switch(dragPlane)
    {
      case xyPlane:
        break; // do nothing
      case xzPlane:
      {
        const Matrix3f dragPlaneRotation = (Matrix3f() << 1.f, 0.f, 0.f,
                                                          0.f, 0.f, -1.f,
                                                          0.f, 1.f, 0.f).finished();
        dragPlanePose.rotation *= dragPlaneRotation;
        break;
      }
      case yzPlane:
      {
        const Matrix3f dragPlaneRotation = (Matrix3f() << 0.f, 0.f, 1.f,
                                                          0.f, 1.f, 0.f,
                                                          -1.f, 0.f, 0.f).finished();
        dragPlanePose.rotation *= dragPlaneRotation;
        break;
      }
    }
  }

  PhysicalObject* physicalObject = dynamic_cast<PhysicalObject*>(&simObject);
  GraphicalObject* graphicalObject = dynamic_cast<GraphicalObject*>(&simObject);

  const bool drawAppearances = graphicalObject && surfaceShadeMode != noShading;
  const bool drawPhysics = physicalObject && (physicsShadeMode != noShading);
  const bool drawSensors = physicalObject && (renderFlags & showSensors);
  const bool drawDragPlane = dragging && dragSelection;
  const bool drawCoordinateSystem = renderFlags & showCoordinateSystem;
  const bool drawControllerDrawings = (physicalObject || graphicalObject) && drawingsShadeMode != noShading && Simulation::simulation->scene->drawingManager;

  GraphicsContext& graphicsContext = Simulation::simulation->graphicsContext;
  if(drawAppearances || drawControllerDrawings)
    graphicsContext.updateModelMatrices(GraphicsContext::ModelMatrix::appearance, dragging && dragSelection);
  if(drawPhysics || drawControllerDrawings)
    graphicsContext.updateModelMatrices(GraphicsContext::ModelMatrix::physicalDrawing, dragging && dragSelection);
  if(drawSensors || drawControllerDrawings)
    graphicsContext.updateModelMatrices(GraphicsContext::ModelMatrix::sensorDrawing, dragging && dragSelection);
  if(drawControllerDrawings)
    graphicsContext.updateModelMatrices(GraphicsContext::ModelMatrix::controllerDrawing, dragging && dragSelection);
  if(drawDragPlane)
    graphicsContext.updateModelMatrices(GraphicsContext::ModelMatrix::dragPlane, true);

  Pose3f invCameraPose = cameraTransformation;
  // Since each object will be drawn globally we need to shift the coordinate system.
  // Also, the origin should be at the parent object's pose.
  // Since the scene is at the global origin, it doesn't need this shift.
  // If the object is neither a physical nor a graphical object, nothing happens, but in that case, nothing (except for a coordinate system) will be drawn anyway.
  if(&simObject != Simulation::simulation->scene && (physicalObject || graphicalObject))
  {
    const auto* modelMatrix = physicalObject ? physicalObject->modelMatrix : graphicalObject->modelMatrix;
    ASSERT(modelMatrix);
    Eigen::Map<const Matrix4f> objectInWorldMatrix(modelMatrix->getPointer());
    const Pose3f objectInWorld(RotationMatrix(objectInWorldMatrix.topLeftCorner<3, 3>()), objectInWorldMatrix.topRightCorner<3, 1>());
    if(renderFlags & showAsGlobalView)
      invCameraPose *= simObject.poseInParent * objectInWorld.inverse(); // center on the object's parent
    else
      invCameraPose *= objectInWorld.inverse(); // center on the object
    Simulation::simulation->originPose = objectInWorld * simObject.poseInParent.inverse();
  }
  else
    Simulation::simulation->originPose = Pose3f();

  if(drawCoordinateSystem)
    graphicsContext.updateModelMatrices(GraphicsContext::ModelMatrix::origin, true);

  const Matrix4f viewMatrix = (Matrix4f() << invCameraPose.rotation, invCameraPose.translation, Eigen::RowVector3f::Zero(), 1.f).finished();

  QOpenGLFunctions_3_3_Core* f = graphicsContext.getOpenGLFunctions();

  if(renderFlags & enableMultisample)
    f->glEnable(GL_MULTISAMPLE);
  else
    f->glDisable(GL_MULTISAMPLE);

  // clear
  bool clear = true;

  // draw origin
  if(drawCoordinateSystem)
  {
    graphicsContext.startColorRendering(projection, viewMatrix, -1, -1, -1, -1, clear, false, false, false, false);
    graphicsContext.draw(Simulation::simulation->xAxisMesh, Simulation::simulation->originModelMatrix, Simulation::simulation->xAxisSurface);
    graphicsContext.draw(Simulation::simulation->yAxisMesh, Simulation::simulation->originModelMatrix, Simulation::simulation->yAxisSurface);
    graphicsContext.draw(Simulation::simulation->zAxisMesh, Simulation::simulation->originModelMatrix, Simulation::simulation->zAxisSurface);
    graphicsContext.finishRendering();
    clear = false;
  }

  // draw object / scene appearance
  if(drawAppearances)
  {
    graphicsContext.startColorRendering(projection, viewMatrix, -1, -1, -1, -1, clear, renderFlags & enableLights, renderFlags & enableTextures, surfaceShadeMode == smoothShading, surfaceShadeMode != wireframeShading);
    graphicalObject->drawAppearances(graphicsContext);
    graphicsContext.finishRendering();
    clear = false;
  }

  // draw object / scene physics
  if(drawPhysics || drawSensors)
  {
    graphicsContext.startColorRendering(projection, viewMatrix, -1, -1, -1, -1, clear, renderFlags & enableLights, renderFlags & enableTextures, physicsShadeMode == smoothShading, physicsShadeMode != wireframeShading);
    physicalObject->drawPhysics(graphicsContext, (renderFlags | (physicsShadeMode != noShading ? showPhysics : 0)) & ~showControllerDrawings);
    graphicsContext.finishRendering();
    clear = false;
  }

  // draw drag plane
  if(drawDragPlane)
  {
    graphicsContext.startColorRendering(projection, viewMatrix, -1, -1, -1, -1, clear, false, false, false, true);
    graphicsContext.draw(Simulation::simulation->dragPlaneMesh, Simulation::simulation->dragPlaneModelMatrix, Simulation::simulation->dragPlaneSurface);
    graphicsContext.finishRendering();
    clear = false;
  }

  // draw controller drawings
  if(drawControllerDrawings)
  {
    // If the manager registered later, it must be done now.
    if(!registeredAtManager)
    {
      Simulation::simulation->scene->drawingManager->registerContext();
      registeredAtManager = true;
    }

    f->glPolygonMode(GL_FRONT_AND_BACK, drawingsShadeMode == wireframeShading ? GL_LINE : GL_FILL);

    f->glEnable(GL_BLEND);
    f->glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);
    f->glBlendColor(1.0f, 1.0f, 1.0f, 1.0f);

    Simulation::simulation->scene->drawingManager->beforeFrame();

    if(physicalObject)
      physicalObject->beforeControllerDrawings(projection.data(), viewMatrix.data());
    if(graphicalObject)
      graphicalObject->beforeControllerDrawings(projection.data(), viewMatrix.data());

    Simulation::simulation->scene->drawingManager->uploadData();

    if(renderFlags & enableDrawingsTransparentOcclusion)
    {
      Simulation::simulation->scene->drawingManager->beforeDraw();

      if(physicalObject)
        physicalObject->drawControllerDrawings();
      if(graphicalObject)
        graphicalObject->drawControllerDrawings();
    }

    if((renderFlags & enableDrawingsTransparentOcclusion) ||
       !(renderFlags & enableDrawingsOcclusion))
      f->glClear(GL_DEPTH_BUFFER_BIT);

    if(renderFlags & enableDrawingsTransparentOcclusion)
      f->glBlendColor(0.5f, 0.5f, 0.5f, 0.5f);

    Simulation::simulation->scene->drawingManager->beforeDraw();

    if(physicalObject)
      physicalObject->drawControllerDrawings();
    if(graphicalObject)
      graphicalObject->drawControllerDrawings();

    if(physicalObject)
      physicalObject->afterControllerDrawings();
    if(graphicalObject)
      graphicalObject->afterControllerDrawings();

    Simulation::simulation->scene->drawingManager->afterFrame();

    f->glDisable(GL_BLEND);
  }
}

void SimObjectRenderer::resize(float fovY, unsigned int width, unsigned int height)
{
  this->fovY = fovY;
  this->width = width;
  this->height = height;

  OpenGLTools::computePerspective(fovY * (pi / 180.f), float(width) / float(height), 0.1f, 500.f, projection);

  // This is needed for the exportAsImage function of the SimObjectWidget.
  Simulation::simulation->graphicsContext.getOpenGLFunctions()->glViewport(0, 0, width, height);
}

Vector3f SimObjectRenderer::projectClick(int x, int y) const
{
  const Vector4f normalizedPoint(2.f * static_cast<float>(x) / width - 1.f, 2.f * static_cast<float>(height - y) / height - 1.f, 1.f, 1.f);
  const Vector4f unprojectedPoint = (projection * (Matrix4f() << cameraTransformation.rotation, cameraTransformation.translation, Eigen::RowVector3f::Zero(), 1.f).finished()).inverse() * normalizedPoint;
  return unprojectedPoint.head<3>() / unprojectedPoint.w();
}

void SimObjectRenderer::getSize(unsigned int& width, unsigned int& height) const
{
  width = this->width;
  height = this->height;
}

void SimObjectRenderer::zoom(float change, float x, float y)
{
  Vector3f v = cameraTarget - cameraPos;
  if(x < 0 || y < 0)
  {
    v.normalize(v.norm() * change * 0.0005f);
    cameraPos -= v;
  }
  else
  {
    Vector3f translationVector;
    if(intersectClickAndCoordinatePlane(static_cast<int>(x), static_cast<int>(y), dragPlane, translationVector))
    {
      translationVector = translationVector - cameraPos;
      cameraPos += translationVector * change * 0.0005f;
      intersectRayAndPlane(cameraPos, v, cameraTarget, dragPlaneVector, cameraTarget);
    }
  }
  updateCameraTransformation();
}

void SimObjectRenderer::fitCamera()
{
  /*
  if(simObject)
    simulation->fitCamera(cameraTarget, cameraPos, width, height, simObject);
  calculateUpVector();
   */
}

void SimObjectRenderer::setDragPlane(DragAndDropPlane plane)
{
  dragPlane = plane;
  calcDragPlaneVector();
}

bool SimObjectRenderer::intersectRayAndPlane(const Vector3f& point, const Vector3f& v,
                                             const Vector3f& plane, const Vector3f& n,
                                             Vector3f& intersection) const
{
  Vector3f p = plane - point;
  float denominator = n.dot(v);
  if(denominator == 0.f)
    return false;
  float r = n.dot(p) / denominator;
  if(r < 0.f)
    return false;
  intersection = v;
  intersection *= r;
  intersection += point;
  return true;
}

bool SimObjectRenderer::intersectClickAndCoordinatePlane(int x, int y, DragAndDropPlane, Vector3f& point) const
{
  return intersectRayAndPlane(cameraPos, projectClick(x, y) - cameraPos, cameraTarget, dragPlaneVector, point);
}

void SimObjectRenderer::calcDragPlaneVector()
{
  switch(dragPlane)
  {
    case xyPlane:
      dragPlaneVector = Vector3f(0.f, 0.f, 1.f);
      break;
    case xzPlane:
      dragPlaneVector = Vector3f(0.f, 1.f, 0.f);
      break;
    case yzPlane:
      dragPlaneVector = Vector3f(1.f, 0.f, 0.f);
      break;
  }
}

Body* SimObjectRenderer::selectObject(const Vector3f& projectedClick)
{
  if(&simObject != Simulation::simulation->scene)
    return nullptr;

  class Callback
  {
  public:
    Body* closestBody;
    float closestSqrDistance;
    const Vector3f& cameraPos;

    Callback(const Vector3f& cameraPos) : closestBody(0), cameraPos(cameraPos) {}

    static void staticCollisionCallback(Callback* callback, dGeomID geom1, dGeomID geom2)
    {
      ASSERT(!dGeomIsSpace(geom1));
      ASSERT(!dGeomIsSpace(geom2));
      ASSERT(dGeomGetBody(geom1) || dGeomGetBody(geom2));
      dContact contact[1];
      if(dCollide(geom1, geom2, 1, &contact[0].geom, sizeof(dContact)) < 1)
        return;

      dGeomID geom = geom2;
      dBodyID bodyId = dGeomGetBody(geom2);
      if(!bodyId)
      {
        bodyId = dGeomGetBody(geom1);
        geom = geom1;
      }
      const dReal* pos = dGeomGetPosition(geom);
      float sqrDistance = (Vector3f(static_cast<float>(pos[0]), static_cast<float>(pos[1]), static_cast<float>(pos[2])) - callback->cameraPos).squaredNorm();
      if(!callback->closestBody || sqrDistance < callback->closestSqrDistance)
      {
        callback->closestBody = static_cast<Body*>(dBodyGetData(bodyId));
        callback->closestSqrDistance = sqrDistance;
      }
    }

    static void staticCollisionWithSpaceCallback(Callback* callback, dGeomID geom1, dGeomID geom2)
    {
      ASSERT(!dGeomIsSpace(geom1));
      ASSERT(dGeomIsSpace(geom2));
      dSpaceCollide2(geom1, geom2, callback, reinterpret_cast<dNearCallback*>(&staticCollisionCallback));
    }
  };

  Callback callback(cameraPos);
  dGeomID ray = dCreateRay(Simulation::simulation->staticSpace, 10000.f);
  Vector3f dir = projectedClick - cameraPos;
  dGeomRaySet(ray, cameraPos.x(), cameraPos.y(), cameraPos.z(), dir.x(), dir.y(), dir.z());
  dSpaceCollide2(ray, reinterpret_cast<dGeomID>(Simulation::simulation->movableSpace), &callback, reinterpret_cast<dNearCallback*>(&Callback::staticCollisionWithSpaceCallback));
  dGeomDestroy(ray);

  if(!callback.closestBody)
    return nullptr;
  Body* body = callback.closestBody;
  return body->rootBody;
}

bool SimObjectRenderer::startDrag(int x, int y, DragType type)
{
  if(dragging)
    return true;

  // look if the user clicked on an object
  dragSelection = 0;
  if(&simObject == Simulation::simulation->scene)
  {
    Vector3f projectedClick = projectClick(x, y);
    dragSelection = selectObject(projectedClick);

    if(dragSelection)
    {
      calcDragPlaneVector();
      if(type == dragRotate || type == dragNormalObject)
        dragPlaneVector = dragSelection->poseInWorld.rotation * dragPlaneVector;
      if(!intersectRayAndPlane(cameraPos, projectedClick - cameraPos, dragSelection->poseInWorld.translation, dragPlaneVector, dragStartPos))
        dragSelection = 0;
      else
      {
        static_cast<SimRobotCore2::Body*>(dragSelection)->enablePhysics(false);
        if(dragMode == resetDynamics)
          static_cast<SimRobotCore2::Body*>(dragSelection)->resetDynamics();

        dragging = true;
        dragType = type;
        if(dragMode == adoptDynamics)
          dragStartTime = System::getTime();
        return true;
      }
    }
  }

  if(!dragSelection) // camera control
  {
    dragStartPos.x() = x;
    dragStartPos.y() = y;
    interCameraPos = cameraPos;
    dragging = true;
    dragType = type;
    return true;
  }
  return false;
}

SimRobotCore2::Object* SimObjectRenderer::getDragSelection()
{
  return dragSelection;
}

bool SimObjectRenderer::moveDrag(int x, int y, DragType type)
{
  if(!dragging)
    return false;

  dragType = type;

  if(!dragSelection) // camera control
  {
    if(dragType == dragRotate || dragType == dragRotateWorld)
    {
      Vector3f v = (dragType == dragRotate ? cameraPos : interCameraPos) - cameraTarget;
      const RotationMatrix rotateY = RotationMatrix::aroundY((y - dragStartPos.y()) * -0.01f);
      const RotationMatrix rotateZ = RotationMatrix::aroundZ((x - dragStartPos.x()) * -0.01f);
      const float hypoLength = std::sqrt(v.x() * v.x() + v.y() * v.y());
      Vector3f v2(hypoLength, 0.f, v.z());
      v2 = rotateY * v2;
      if(v2.x() < 0.001f)
      {
        v2.x() = 0.001f;
        v2.normalize(v.norm());
      }
      Vector3f v3(v.x(), v.y(), 0.f);
      v3.normalize(v2.x());
      v3.z() = v2.z();
      v = rotateZ * v3;
      interCameraPos = cameraTarget + v;
      if(dragType == dragRotate)
        cameraPos = cameraTarget + v;
      else
      {
        float angleZ = std::atan2(v.y(), v.x()) * (180.f / pi);
        float angleY = ((pi / 2.f) - std::atan2(v.z(), hypoLength)) * (180.f / pi);
        int zRounded = ((static_cast<int>(angleZ) + degreeSteps / 2) / degreeSteps) * degreeSteps;
        int yRounded = ((static_cast<int>(angleY) + degreeSteps / 2) / degreeSteps) * degreeSteps;
        angleZ = zRounded * (pi / 180.f);
        angleY = yRounded * (pi / 180.f);
        if(angleY == 0)
          angleY = 0.00001f;
        cameraPos = cameraTarget + Vector3f(std::sin(angleY) * std::cos(angleZ), std::sin(angleY) * std::sin(angleZ), std::cos(angleY)).normalize(v.norm());
      }
    }
    else // if(dragType == dragNormal)
    {
      Vector3f start;
      Vector3f end;
      if(intersectClickAndCoordinatePlane(static_cast<int>(dragStartPos.x()), static_cast<int>(dragStartPos.y()), dragPlane, start))
        if(intersectClickAndCoordinatePlane(x, y, dragPlane, end))
        {
          Vector3f translate = end - start;
          cameraPos -= translate;
          cameraTarget -= translate;
        }
    }

    dragStartPos.x() = x;
    dragStartPos.y() = y;
    updateCameraTransformation();
    return true;
  }
  else // object control
  {
    if(dragMode == applyDynamics)
      return true;
    Vector3f projectedClick = projectClick(x, y);
    Vector3f currentPos;
    if(intersectRayAndPlane(cameraPos, projectedClick - cameraPos, dragSelection->poseInWorld.translation, dragPlaneVector, currentPos))
    {
      if(dragType == dragRotate || dragType == dragRotateWorld)
      {
        Vector3f oldV = dragStartPos - dragSelection->poseInWorld.translation;
        Vector3f newV = currentPos - dragSelection->poseInWorld.translation;

        if(dragType != dragRotateWorld)
        {
          const RotationMatrix invRotation = dragSelection->poseInWorld.rotation.inverse();
          oldV = invRotation * oldV;
          newV = invRotation * newV;
        }

        float angle = 0.f;
        if(dragPlane == yzPlane)
          angle = normalize(std::atan2(newV.z(), newV.y()) - std::atan2(oldV.z(), oldV.y()));
        else if(dragPlane == xzPlane)
          angle = normalize(std::atan2(newV.x(), newV.z()) - std::atan2(oldV.x(), oldV.z()));
        else
          angle = normalize(std::atan2(newV.y(), newV.x()) - std::atan2(oldV.y(), oldV.x()));

        const Vector3f offset = dragPlaneVector * angle;
        const RotationMatrix rotation = Rotation::AngleAxis::unpack(offset);
        Vector3f center = dragSelection->poseInWorld.translation;
        dragSelection->rotate(rotation, center);
        if(dragMode == adoptDynamics)
        {
          const unsigned int now = System::getTime();
          const float t = std::max(1U, now - dragStartTime) * 0.001f;
          Vector3f velocity = offset / t;
          const dReal* oldVel = dBodyGetAngularVel(dragSelection->body);
          velocity = velocity * 0.3f + Vector3f(static_cast<float>(oldVel[0]), static_cast<float>(oldVel[1]), static_cast<float>(oldVel[2])) * 0.7f;
          dBodySetAngularVel(dragSelection->body, velocity.x(), velocity.y(), velocity.z());
          dragStartTime = now;
        }
        dragStartPos = currentPos;
      }
      else
      {
        const Vector3f offset = currentPos - dragStartPos;
        dragSelection->move(offset);
        if(dragMode == adoptDynamics)
        {
          const unsigned int now = System::getTime();
          const float t = std::max(1U, now - dragStartTime) * 0.001f;
          Vector3f velocity = offset / t;
          const dReal* oldVel = dBodyGetLinearVel(dragSelection->body);
          velocity = velocity * 0.3f + Vector3f(static_cast<float>(oldVel[0]), static_cast<float>(oldVel[1]), static_cast<float>(oldVel[2])) * 0.7f;
          dBodySetLinearVel(dragSelection->body, velocity.x(), velocity.y(), velocity.z());
          dragStartTime = now;
        }
        dragStartPos = currentPos;
      }
    }
    return true;
  }
}

bool SimObjectRenderer::releaseDrag(int x, int y)
{
  if(!dragging)
    return false;

  if(!dragSelection) // camera control
  {
    dragging = false;
    return true;
  }
  else // object control
  {
    if(dragMode == adoptDynamics)
      moveDrag(x, y, dragType);
    else if(dragMode == applyDynamics)
    {
      Vector3f projectedClick = projectClick(x, y);
      Vector3f currentPos;
      if(intersectRayAndPlane(cameraPos, projectedClick - cameraPos, dragSelection->poseInWorld.translation, dragPlaneVector, currentPos))
      {
        if(dragType == dragRotate || dragType == dragRotateWorld)
        {
          Vector3f oldV = dragStartPos - dragSelection->poseInWorld.translation;
          Vector3f newV = currentPos - dragSelection->poseInWorld.translation;

          if(dragType != dragRotateWorld)
          {
            const RotationMatrix invRotation = dragSelection->poseInWorld.rotation.inverse();
            oldV = invRotation * oldV;
            newV = invRotation * newV;
          }

          float angle = 0.f;
          if(dragPlane == yzPlane)
            angle = normalize(std::atan2(newV.z(), newV.y()) - std::atan2(oldV.z(), oldV.y()));
          else if(dragPlane == xzPlane)
            angle = normalize(std::atan2(newV.x(), newV.z()) - std::atan2(oldV.x(), oldV.z()));
          else
            angle = normalize(std::atan2(newV.y(), newV.x()) - std::atan2(oldV.y(), oldV.x()));

          const Vector3f offset = dragPlaneVector * angle;
          const Vector3f torque = offset * static_cast<float>(dragSelection->mass.mass) * 50.f;
          dBodyAddTorque(dragSelection->body, torque.x(), torque.y(), torque.z());
        }
        else
        {
          const Vector3f offset = currentPos - dragStartPos;
          const Vector3f force = offset * static_cast<float>(dragSelection->mass.mass) * 500.f;
          dBodyAddForce(dragSelection->body, force.x(), force.y(), force.z());
        }
      }
    }

    static_cast<SimRobotCore2::Body*>(dragSelection)->enablePhysics(true);

    dragging = false;
    return true;
  }
}

void SimObjectRenderer::setCamera(const float* pos, const float* target)
{
  cameraPos = Vector3f(pos[0], pos[1], pos[2]);
  cameraTarget = Vector3f(target[0], target[1], target[2]);
  updateCameraTransformation();
}

void SimObjectRenderer::getCamera(float* pos, float* target)
{
  pos[0] = cameraPos.x();
  pos[1] = cameraPos.y();
  pos[2] = cameraPos.z();
  target[0] = cameraTarget.x();
  target[1] = cameraTarget.y();
  target[2] = cameraTarget.z();
}
