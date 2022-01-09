/**
 * @file SimObjectRenderer.cpp
 * Declaration of class SimObjectRenderer
 * @author Colin Graf
 */

#include "SimObjectRenderer.h"
#include "Graphics/OpenGL.h"
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

#ifdef WINDOWS
typedef int (*GLBlendColorProc)(GLfloat, GLfloat, GLfloat, GLfloat);
static GLBlendColorProc glBlendColor = nullptr;
static int noBlendColor(GLfloat, GLfloat, GLfloat, GLfloat) {return 0;}
#endif

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
  initialized = true;
  calcDragPlaneVector();
#ifdef WINDOWS
  glBlendColor = reinterpret_cast<GLBlendColorProc>(wglGetProcAddress("glBlendColor"));
  if(!glBlendColor)
    glBlendColor = &noBlendColor;
#endif
}

void SimObjectRenderer::destroy()
{
  ASSERT(initialized);
  Simulation::simulation->graphicsContext.destroyGraphics();
  initialized = false;
}

void SimObjectRenderer::draw()
{
  // make sure transformations of movable bodies are up-to-date
  Simulation::simulation->scene->updateTransformations();

  if(dragging && dragSelection)
  {
    Matrix4f& dragPlaneTransformation = Simulation::simulation->dragPlaneTransformation;
    if(dragType == dragRotate || dragType == dragNormalObject)
      dragPlaneTransformation = dragSelection->transformation;
    else
    {
      dragPlaneTransformation = Matrix4f::Identity();
      dragPlaneTransformation.topRightCorner<3, 1>() = dragSelection->pose.translation;
    }

    switch(dragPlane)
    {
      case xyPlane:
        break; // do nothing
      case xzPlane:
      {
        const Matrix4f dragPlaneRotation = (Matrix4f() << 1.f, 0.f, 0.f, 0.f,
                                                          0.f, 0.f, -1.f, 0.f,
                                                          0.f, 1.f, 0.f, 0.f,
                                                          0.f, 0.f, 0.f, 1.f).finished();
        dragPlaneTransformation *= dragPlaneRotation;
        break;
      }
      case yzPlane:
      {
        const Matrix4f dragPlaneRotation = (Matrix4f() << 0.f, 0.f, 1.f, 0.f,
                                                          0.f, 1.f, 0.f, 0.f,
                                                          -1.f, 0.f, 0.f, 0.f,
                                                          0.f, 0.f, 0.f, 1.f).finished();
        dragPlaneTransformation *= dragPlaneRotation;
        break;
      }
    }
  }

  GraphicsContext& graphicsContext = Simulation::simulation->graphicsContext;
  graphicsContext.updateModelMatrices(dragging && dragSelection);

  if(renderFlags & enableMultisample)
    glEnable(GL_MULTISAMPLE);
  else
    glDisable(GL_MULTISAMPLE);

  // clear
  bool clear = true;

  // since each object will be drawn relative to its parent we need to shift the coordinate system when we want the object to be in the center
  Matrix4f viewMatrix = cameraTransformation;
  if(&simObject != Simulation::simulation->scene && !(renderFlags & showAsGlobalView))
  {
    Pose3f pose(Matrix3f(simObject.transformation.topLeftCorner<3, 3>()), simObject.transformation.topRightCorner<3, 1>());
    Matrix4f invTrans;
    OpenGLTools::convertTransformation(pose.invert(), invTrans);
    viewMatrix *= invTrans;
  }

  // draw origin
  if(renderFlags & showCoordinateSystem)
  {
    graphicsContext.startColorRendering(projection, viewMatrix, -1, -1, -1, -1, clear, false, false, false, false);
    graphicsContext.draw(Simulation::simulation->xAxisMesh, Simulation::simulation->originModelMatrix, Simulation::simulation->xAxisSurface);
    graphicsContext.draw(Simulation::simulation->yAxisMesh, Simulation::simulation->originModelMatrix, Simulation::simulation->yAxisSurface);
    graphicsContext.draw(Simulation::simulation->zAxisMesh, Simulation::simulation->originModelMatrix, Simulation::simulation->zAxisSurface);
    graphicsContext.finishRendering();
    clear = false;
  }

  // draw object / scene appearance
  GraphicalObject* graphicalObject = dynamic_cast<GraphicalObject*>(&simObject);
  if(graphicalObject && surfaceShadeMode != noShading)
  {
    graphicsContext.startColorRendering(projection, viewMatrix, -1, -1, -1, -1, clear, renderFlags & enableLights, renderFlags & enableTextures, surfaceShadeMode == smoothShading, surfaceShadeMode != wireframeShading);
    graphicalObject->drawAppearances(graphicsContext, false);
    graphicsContext.finishRendering();
    clear = false;
  }

  // draw object / scene physics
  PhysicalObject* physicalObject = dynamic_cast<PhysicalObject*>(&simObject);
  if(physicalObject && (physicsShadeMode != noShading || renderFlags & showSensors))
  {
    graphicsContext.startColorRendering(projection, viewMatrix, -1, -1, -1, -1, clear, renderFlags & enableLights, renderFlags & enableTextures, physicsShadeMode == smoothShading, physicsShadeMode != wireframeShading);
    physicalObject->drawPhysics(graphicsContext, (renderFlags | (physicsShadeMode != noShading ? showPhysics : 0)) & ~showControllerDrawings);
    graphicsContext.finishRendering();
    clear = false;
  }

  // draw drag plane
  if(dragging && dragSelection)
  {
    graphicsContext.startColorRendering(projection, viewMatrix, -1, -1, -1, -1, clear, false, false, false, true);
    graphicsContext.draw(Simulation::simulation->dragPlaneMesh, Simulation::simulation->dragPlaneModelMatrix, Simulation::simulation->dragPlaneSurface);
    graphicsContext.finishRendering();
    clear = false;
  }

  // TODO: draw controller drawings
}

void SimObjectRenderer::resize(float fovY, unsigned int width, unsigned int height)
{
  this->fovY = fovY;
  this->width = width;
  this->height = height;

  OpenGLTools::computePerspective(fovY * (pi / 180.f), float(width) / float(height), 0.1f, 500.f, projection);
}

Vector3f SimObjectRenderer::projectClick(int x, int y) const
{
  const Vector4f normalizedPoint(2.f * static_cast<float>(x) / width - 1.f, 2.f * static_cast<float>(height - y) / height - 1.f, 1.f, 1.f);
  const Vector4f unprojectedPoint = (projection * cameraTransformation).inverse() * normalizedPoint;
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
        dragPlaneVector = dragSelection->pose.rotation * dragPlaneVector;
      if(!intersectRayAndPlane(cameraPos, projectedClick - cameraPos, dragSelection->pose.translation, dragPlaneVector, dragStartPos))
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
    if(intersectRayAndPlane(cameraPos, projectedClick - cameraPos, dragSelection->pose.translation, dragPlaneVector, currentPos))
    {
      if(dragType == dragRotate || dragType == dragRotateWorld)
      {
        Vector3f oldV = dragStartPos - dragSelection->pose.translation;
        Vector3f newV = currentPos - dragSelection->pose.translation;

        if(dragType != dragRotateWorld)
        {
          const RotationMatrix invRotation = dragSelection->pose.rotation.inverse();
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
        Vector3f center = dragSelection->pose.translation;
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
      if(intersectRayAndPlane(cameraPos, projectedClick - cameraPos, dragSelection->pose.translation, dragPlaneVector, currentPos))
      {
        if(dragType == dragRotate || dragType == dragRotateWorld)
        {
          Vector3f oldV = dragStartPos - dragSelection->pose.translation;
          Vector3f newV = currentPos - dragSelection->pose.translation;

          if(dragType != dragRotateWorld)
          {
            const RotationMatrix invRotation = dragSelection->pose.rotation.inverse();
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
