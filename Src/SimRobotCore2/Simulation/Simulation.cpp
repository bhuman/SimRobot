/**
 * @file Simulation/Simulation.cpp
 * Implementation of class Simulation
 * @author Colin Graf
 */

#include "Simulation.h"
#include "CoreModule.h"
#include "Graphics/Primitives.h"
#include "Parser/ElementCore2.h"
#include "Parser/ParserCore2.h"
#include "Platform/Assert.h"
#include "Platform/System.h"
#include "Simulation/Body.h"
#include "Simulation/Geometries/Geometry.h"
#include "Simulation/Geometries/TorusGeometry.h"
#include "Simulation/Scene.h"
#include "Tools/ODETools.h"
#include <ode/collision.h>
#include <ode/collision_space.h>
#include <ode/objects.h>
#include <ode/odeinit.h>
#include <algorithm>
#include <cmath>
#ifdef MULTI_THREADING
#include <ode/threading_impl.h>
#include <thread>
#endif

Simulation* Simulation::simulation = nullptr;

Simulation::Simulation()
{
  ASSERT(!simulation);
  simulation = this;
}

Simulation::~Simulation()
{
  for(ElementCore2* element : elements)
    delete element;

  if(contactGroup)
    dJointGroupDestroy(contactGroup);
  if(rootSpace)
    dSpaceDestroy(rootSpace);
  if(physicalWorld)
  {
#ifdef MULTI_THREADING
    dThreadingImplementationShutdownProcessing(threading);
    dThreadingThreadPoolWaitIdleState(pool);
    dThreadingFreeThreadPool(pool);
    dWorldSetStepThreadingImplementation(physicalWorld, nullptr, nullptr);
    dThreadingFreeImplementation(threading);
#endif
    dWorldDestroy(physicalWorld);
    dCloseODE();
  }

  ASSERT(simulation == this);
  simulation = nullptr;
}

bool Simulation::loadFile(const std::string& filename, std::list<std::string>& errors)
{
  ASSERT(!scene);
  ASSERT(elements.empty());

  ParserCore2 parser;
  if(!parser.parse(filename, errors))
  {
    if(scene)
    {
      for(ElementCore2* element : elements)
        delete element;
      elements.clear();
      scene = nullptr;
    }
    return false;
  }

  ASSERT(scene);

  dInitODE();
  physicalWorld = dWorldCreate();
  rootSpace = dHashSpaceCreate(nullptr);
  staticSpace = dHashSpaceCreate(rootSpace);
  movableSpace = dHashSpaceCreate(rootSpace);
  contactGroup = dJointGroupCreate(0);

  TorusGeometry::registerGeometryClass();
  dWorldSetGravity(physicalWorld, REAL(0.), REAL(0.), static_cast<dReal>(scene->gravity));
  if(scene->erp != -1.f)
    dWorldSetERP(physicalWorld, scene->erp);
  if(scene->cfm != -1.f)
    dWorldSetCFM(physicalWorld, scene->cfm);
  if(scene->quickSolverIterations != -1)
    dWorldSetQuickStepNumIterations(physicalWorld, scene->quickSolverIterations);
#ifdef MULTI_THREADING
  threading = dThreadingAllocateMultiThreadedImplementation();
  pool = dThreadingAllocateThreadPool(std::thread::hardware_concurrency(), 0, dAllocateFlagBasicData, nullptr);
  dThreadingThreadPoolServeMultiThreadedImplementation(pool, threading);
  dWorldSetStepThreadingImplementation(physicalWorld, dThreadingImplementationGetFunctions(threading), threading);
#endif

  graphicsContext.pushModelMatrixStack();
  scene->createPhysics(graphicsContext);
  graphicsContext.popModelMatrixStack();

  graphicsContext.pushModelMatrixStack();
  scene->createGraphics(graphicsContext);
  graphicsContext.popModelMatrixStack();

  xAxisMesh = Primitives::createLine(graphicsContext, Vector3f::Zero(), Vector3f(1.f, 0.f, 0.f));
  yAxisMesh = Primitives::createLine(graphicsContext, Vector3f::Zero(), Vector3f(0.f, 1.f, 0.f));
  zAxisMesh = Primitives::createLine(graphicsContext, Vector3f::Zero(), Vector3f(0.f, 0.f, 1.f));
  dragPlaneMesh = Primitives::createDisk(graphicsContext, 0.003f, 0.5f, 30);
  bodyComSphereMesh = Primitives::createSphere(graphicsContext, 0.003f, 10, 10, false);
  static const float redColor[] = {1.f, 0.f, 0.f, 1.f};
  static const float greenColor[] = {0.f, 1.f, 0.f, 1.f};
  static const float blueColor[] = {0.f, 0.f, 1.f, 1.f};
  static const float dragPlaneColor[] = {0.5f, 0.5f, 0.5f, 0.5f};
  static const float bodyComSphereColor[] = {0.8f, 0.f, 0.f, 1.f};
  xAxisSurface = graphicsContext.requestSurface(redColor, redColor);
  yAxisSurface = graphicsContext.requestSurface(greenColor, greenColor);
  zAxisSurface = graphicsContext.requestSurface(blueColor, blueColor);
  dragPlaneSurface = graphicsContext.requestSurface(dragPlaneColor, dragPlaneColor);
  bodyComSphereSurface = graphicsContext.requestSurface(bodyComSphereColor, bodyComSphereColor);


  graphicsContext.pushModelMatrixStack();
  graphicsContext.pushModelMatrixByReference(originPose);
  originModelMatrix = graphicsContext.requestModelMatrix(GraphicsContext::ModelMatrix::origin);
  graphicsContext.popModelMatrix();
  graphicsContext.pushModelMatrixByReference(dragPlanePose);
  dragPlaneModelMatrix = graphicsContext.requestModelMatrix(GraphicsContext::ModelMatrix::dragPlane);
  graphicsContext.popModelMatrix();
  graphicsContext.popModelMatrixStack();

  graphicsContext.compile();

  return true;
}

void Simulation::doSimulationStep()
{
  ++simulationStep;
  simulatedTime += scene->stepLength;

  scene->updateActuators();

  collisions = contactPoints = 0;

  dSpaceCollide2(reinterpret_cast<dGeomID>(staticSpace), reinterpret_cast<dGeomID>(movableSpace), this, reinterpret_cast<dNearCallback*>(&staticCollisionWithSpaceCallback));
  if(scene->detectBodyCollisions)
    dSpaceCollide(movableSpace, this, reinterpret_cast<dNearCallback*>(&staticCollisionSpaceWithSpaceCallback));

  if(scene->useQuickSolver && (simulationStep % scene->quickSolverSkip) == 0)
    dWorldQuickStep(physicalWorld, scene->stepLength);
  else
    dWorldStep(physicalWorld, scene->stepLength);
  dJointGroupEmpty(contactGroup);

  updateFrameRate();
}

void Simulation::staticCollisionWithSpaceCallback(Simulation* simulation, dGeomID geomId1, dGeomID geomId2)
{
  ASSERT(!dGeomIsSpace(geomId1));
  ASSERT(dGeomIsSpace(geomId2));
  dSpaceCollide2(geomId1, geomId2, simulation, reinterpret_cast<dNearCallback*>(&staticCollisionCallback));
}

void Simulation::staticCollisionSpaceWithSpaceCallback(Simulation* simulation, dGeomID geomId1, dGeomID geomId2)
{
  ASSERT(dGeomIsSpace(geomId1));
  ASSERT(dGeomIsSpace(geomId2));
  dSpaceCollide2(geomId1, geomId2, simulation, reinterpret_cast<dNearCallback*>(&staticCollisionCallback));
}

void Simulation::staticCollisionCallback(Simulation* simulation, dGeomID geomId1, dGeomID geomId2)
{
  ASSERT(!dGeomIsSpace(geomId1));
  ASSERT(!dGeomIsSpace(geomId2));

#ifndef NDEBUG
  {
    dBodyID bodyId1 = dGeomGetBody(geomId1);
    dBodyID bodyId2 = dGeomGetBody(geomId2);
    ASSERT(bodyId1 || bodyId2);

    Body* body1 = bodyId1 ? static_cast<Body*>(dBodyGetData(bodyId1)) : nullptr;
    Body* body2 = bodyId2 ? static_cast<Body*>(dBodyGetData(bodyId2)) : nullptr;
    ASSERT(!body1 || !body2 || body1->rootBody != body2->rootBody);
  }
#endif

  dContact contact[32];
  int collisions = dCollide(geomId1, geomId2, 32, &contact[0].geom, sizeof(dContact));
  if(collisions <= 0)
    return;

  Geometry* geometry1 = static_cast<Geometry*>(dGeomGetData(geomId1));
  Geometry* geometry2 = static_cast<Geometry*>(dGeomGetData(geomId2));

  if(geometry1->collisionCallbacks && !geometry2->immaterial)
  {
    for(SimRobotCore2::CollisionCallback* callback : *geometry1->collisionCallbacks)
      callback->collided(*geometry1, *geometry2);
    if(geometry1->immaterial)
      return;
  }
  if(geometry2->collisionCallbacks && !geometry1->immaterial)
  {
    for(SimRobotCore2::CollisionCallback* callback : *geometry2->collisionCallbacks)
      callback->collided(*geometry2, *geometry1);
    if(geometry2->immaterial)
      return;
  }

  dBodyID bodyId1 = dGeomGetBody(geomId1);
  dBodyID bodyId2 = dGeomGetBody(geomId2);
  ASSERT(bodyId1 || bodyId2);

  float friction = 1.f;
  if(geometry1->material && geometry2->material)
  {
    if(!geometry1->material->getFriction(*geometry2->material, friction))
      friction = 1.f;

    float rollingFriction;
    if(bodyId1)
      switch(dGeomGetClass(geomId1))
      {
        case dSphereClass:
        case dCapsuleClass:
        case dCylinderClass:
          if(geometry1->material->getRollingFriction(*geometry2->material, rollingFriction))
          {
            dBodySetAngularDamping(bodyId1, 0.2f);
            Vector3f linearVel;
            ODETools::convertVector(dBodyGetLinearVel(bodyId1), linearVel);
            linearVel -= linearVel.normalized(std::min(linearVel.norm(), rollingFriction * simulation->scene->stepLength));
            dBodySetLinearVel(bodyId1, linearVel.x(), linearVel.y(), linearVel.z());
          }
          break;
      }
    if(bodyId2)
      switch(dGeomGetClass(geomId2))
      {
        case dSphereClass:
        case dCapsuleClass:
        case dCylinderClass:
          if(geometry2->material->getRollingFriction(*geometry1->material, rollingFriction))
          {
            dBodySetAngularDamping(bodyId2, 0.2f);
            Vector3f linearVel;
            ODETools::convertVector(dBodyGetLinearVel(bodyId2), linearVel);
            linearVel -= linearVel.normalized(std::min(linearVel.norm(), rollingFriction * simulation->scene->stepLength));
            dBodySetLinearVel(bodyId2, linearVel.x(), linearVel.y(), linearVel.z());
          }
          break;
      }
  }

  for(dContact* cont = contact, * end = contact + collisions; cont < end; ++cont)
  {
    cont->surface.mode = simulation->scene->contactMode | dContactApprox1;
    cont->surface.mu = friction;

    /*
    cont->surface.bounce = 0.f;
    cont->surface.bounce_vel = 0.001f;
    cont->surface.slip1 = 0.f;
    cont->surface.slip2 = 0.f;
    */
    cont->surface.soft_erp = simulation->scene->contactSoftERP;
    cont->surface.soft_cfm = simulation->scene->contactSoftCFM;

    dJointID c = dJointCreateContact(simulation->physicalWorld, simulation->contactGroup, cont);
    ASSERT(bodyId1 == dGeomGetBody(cont->geom.g1));
    ASSERT(bodyId2 == dGeomGetBody(cont->geom.g2));
    dJointAttach(c, bodyId1, bodyId2);
  }
  ++simulation->collisions;
  simulation->contactPoints += collisions;
}

void Simulation::updateFrameRate()
{
  const unsigned int currentTime = System::getTime();
  const unsigned int timeDiff = currentTime - lastFrameRateComputationTime;
  //Only update frame rate once in two seconds
  if(timeDiff > 2000)
  {
    const float frameRate = static_cast<float>(simulationStep - lastFrameRateComputationStep) / (static_cast<float>(timeDiff) * 0.001f);
    currentFrameRate = static_cast<int>(frameRate + 0.5f);
    lastFrameRateComputationTime = currentTime;
    lastFrameRateComputationStep = simulationStep;
  }
}

void Simulation::registerObjects()
{
  scene->fullName = scene->name.c_str();
  CoreModule::application->registerObject(*CoreModule::module, *scene, 0);
  scene->registerObjects();
}
