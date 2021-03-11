/**
 * @file Simulation/Simulation.h
 * Declaration of class Simulation
 * @author Colin Graf
 */

#pragma once

#include "Platform/OffscreenRenderer.h"
#include <string>
#include <list>
#include <ode/common.h>
#ifdef MULTI_THREADING
#include <ode/threading.h>
#endif

class Scene;
class Element;

/**
 * @class Simulation
 * A class for managing the simulation
 */
class Simulation
{
public:
  static Simulation* simulation;

  Scene* scene = nullptr; /**< The root of the scene graph */
  std::list<Element*> elements; /**< All scene graph elements */

  dWorldID physicalWorld = nullptr; /**< The physical world */
  dSpaceID rootSpace = nullptr; /**< The root collision space */
  dSpaceID staticSpace = nullptr; /**< The collision space for static objects */
  dSpaceID movableSpace = nullptr; /**< The collision space for movable objects */
#ifdef MULTI_THREADING
  dThreadingImplementationID threading; /**< Needed for multithreaded physics. */
  dThreadingThreadPoolID pool; /**< The thread pool for physics. */
#endif

  OffscreenRenderer renderer; /**< For rendering OpenGL scenes without a regular window */

  unsigned int currentFrameRate = 0; /**< The current frame rate of the simulation */

  /** Default Constructor. */
  Simulation();

  /** Destructor. */
  virtual ~Simulation();

  /**
   * Loads a file and initializes the simulation
   * @param filename The name of the file
   * @param errors The errors that occured during parsing.
   */
  bool loadFile(const std::string& filename, std::list<std::string>& errors);

  /** Executes one simulation step */
  void doSimulationStep();
  unsigned int simulationStep = 0;
  double simulatedTime = 0;
  unsigned int collisions = 0;
  unsigned int contactPoints = 0;

  /** Registers all objects of the simulation (including children, actuators and sensors) at SimRobot's GUI */
  void registerObjects();

private:
  dJointGroupID contactGroup = nullptr; /**< The joint group for temporary contact joints used for collision handling */

  /** Computes the frame rate of simulation */
  void updateFrameRate();
  unsigned int lastFrameRateComputationTime = 0;
  unsigned int lastFrameRateComputationStep = 0;

  /**
   * Static callback method for handling the collision of two geometries
   * @param simulation The simulation
   * @param geom1 The first geometry object for collision testing
   * @param geom2 The second geometry object for collision testing
   */
  static void staticCollisionCallback(Simulation *simulation, dGeomID geom1, dGeomID geom2);

  /**
   * Static callback method for handling the collision of a static geometry with a movable space
   * @param simulation The simulation
   * @param geom1 The first geometry object for collision testing
   * @param geom2 The second geometry object for collision testing
   */
  static void staticCollisionWithSpaceCallback(Simulation *simulation, dGeomID geom1, dGeomID geom2);

  /**
   * Static callback method for handling the collision of two movable spaces
   * @param simulation The simulation
   * @param geom1 The first geometry object for collision testing
   * @param geom2 The second geometry object for collision testing
   */
  static void staticCollisionSpaceWithSpaceCallback(Simulation *simulation, dGeomID geom1, dGeomID geom2);
};
