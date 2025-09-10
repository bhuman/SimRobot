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
#include "Simulation/Scene.h"
#include <mujoco/mujoco.h>
#include <algorithm>
#include <cmath>

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

  if(data)
    mj_deleteData(data);
  if(model)
    mj_deleteModel(model);

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

  spec = mj_makeSpec();
  ASSERT(spec);

  spec->compiler.degree = 0;
  spec->compiler.inertiafromgeom = mjINERTIAFROMGEOM_AUTO;

  spec->option.timestep = scene->stepLength;
  spec->option.apirate = scene->stepLength;
  spec->option.integrator = mjINT_EULER;
  spec->option.iterations = 50;
  spec->option.tolerance = 1e-6;
  spec->option.solver = mjSOL_NEWTON;
  spec->option.jacobian = mjJAC_AUTO;
  spec->option.gravity[0] = mjtNum(0);
  spec->option.gravity[1] = mjtNum(0);
  spec->option.gravity[2] = mjtNum(scene->gravity);

  /*spec->option.enableflags |= mjENBL_OVERRIDE;

  // 2. Reibung (o_friction) setzen:
  spec->option.o_friction[0] = 1.0;     // sliding
  spec->option.o_friction[1] = 1.0;   // torsional
  spec->option.o_friction[2] = 0.005;  // rolling
  // Für volle Kompatibilität:
  spec->option.o_friction[3] = 0.0001;
  spec->option.o_friction[4] = 0.0001;
   */
  // 3. solref setzen (Timeconst, Damping):
  /* spec->option.o_solref[0] = 0.02;
   spec->option.o_solref[1] = 1.0;

   // 4. solimp setzen (Impedance-Koeffizienten):
   spec->option.o_solimp[0] = 0.9;
   spec->option.o_solimp[1] = 0.95;
   spec->option.o_solimp[2] = 0.001;
   spec->option.o_solimp[3] = 0.5;
   spec->option.o_solimp[4] = 2.0;
   spec->option.o_margin = 0;*/

  worldBody = mjs_findBody(spec, "world");

  graphicsContext.pushModelMatrixStack();
  scene->createPhysics(graphicsContext);
  graphicsContext.popModelMatrixStack();

  worldBody = nullptr;

  model = mj_compile(spec, nullptr);

  if(!model)
    errors.push_back(mjs_getError(spec));

  mj_deleteSpec(spec);
  spec = nullptr;

  if(!model)
    return false;

  data = mj_makeData(model);
  ASSERT(data);

  bodyMap.resize(model->nbody);
  geometryMap.resize(model->ngeom);
  for(auto& name : names)
  {
    const int id = mj_name2id(model, name.type, name.name.c_str());
    ASSERT(id >= 0);
    if(name.type == mjOBJ_BODY)
      bodyMap[id] = static_cast<Body*>(name.object);
    else if(name.type == mjOBJ_GEOM)
      geometryMap[id] = static_cast<Geometry*>(name.object);
    if(name.indexPointer)
      *(name.indexPointer) = id;
  }

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

  mj_step1(model, data);

  scene->updateActuators();

  mj_step2(model, data);

  std::memset(data->xfrc_applied, 0, model->nbody * 6 * sizeof(mjtNum));

  collisions = 0;
  contactPoints = data->ncon;
  for(int i = 0; i < data->ncon; ++i)
  {
    const int geom1 = data->contact[i].geom[0];
    const int geom2 = data->contact[i].geom[1];
    // Only report the first contact of each collision. (This assumes that the array is ordered.)
    if(i && geom1 == data->contact[i - 1].geom[0] && geom2 == data->contact[i - 1].geom[1])
      continue;
    ++collisions;
    if(auto* geometry1 = geometryMap[geom1], * geometry2 = geometryMap[geom2]; geometry1 && geometry2)
    {
      if(geometry1->collisionCallbacks)
        for(auto* callback : *geometry1->collisionCallbacks)
          callback->collided(*geometry1, *geometry2);
      if(geometry2->collisionCallbacks)
        for(auto* callback : *geometry2->collisionCallbacks)
          callback->collided(*geometry2, *geometry1);
    }
  }

  updateFrameRate();
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
