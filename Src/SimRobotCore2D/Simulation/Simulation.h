/**
 * @file Simulation.h
 *
 * This file declares a class which executes the actual simulation.
 *
 * @author Arne Hasselbring
 */

#pragma once

#include <box2d/b2_world_callbacks.h>
#include <list>
#include <string>
#include <unordered_map>

class b2Body;
class b2World;
class Element;
class Scene;

class Simulation : public b2ContactListener
{
public:
  /** Constructor. */
  Simulation();

  /** Destructor. */
  ~Simulation() override;

  /**
   * Loads the simulation from a scene description file.
   * @param fileName The name of the file to load.
   * @param errors A list which is filled with messages about errors during loading.
   * @return Whether the file was loaded successfully.
   */
  bool loadFile(const std::string& fileName, std::list<std::string>& errors);

  /** Registers all objects in the scene graph. */
  void registerObjects();

  /** Executes one time step (frame) of the simulation. */
  void doSimulationStep();

  static Simulation* simulation; /**< The only instance of this class. */
  std::list<Element*> elements; /**< All elements in the simulation. */
  Scene* scene = nullptr; /**< The scene that is being simulated. */
  unsigned int simulationStep = 0; /**< The step counter of the simulation. */
  double simulatedTime = 0.0; /**< The time that has elapsed since the start of the simulation. */
  unsigned int currentFrameRate = 0; /**< The average number of simulated frames per second. */
  unsigned int collisions = 0; /**< The number of collisions that started in the most recent frame. */

  b2World* world = nullptr; /**< The Box2D world in which the physics happen. */
  b2Body* staticBody = nullptr; /**< The Box2D body to which compound fixtures are attached. */

protected:
  /**
   * Is called when a contact begins.
   * @param contact The contact.
   */
  void BeginContact(b2Contact* contact) override;

  /**
   * Is called when a contact ends.
   * @param contact The contact.
   */
  void EndContact(b2Contact* contact) override;

private:
  /** Updates the frame rate (if required). */
  void updateFrameRate();

  /**
   * Call collision callbacks for a contact.
   * @param contact The contact.
   */
  static void reportCollisions(b2Contact* contact);

  unsigned int lastFrameRateComputationTime = 0; /**< The (real) time when the frame rate was calculated. */
  unsigned int lastFrameRateComputationStep = 0; /**< The step number when the frame rate was calculated. */
  std::unordered_map<b2Contact*, bool> contacts; /**< The active contacts + whether they should be additionally reported in \c doSimulationStep. */
};
