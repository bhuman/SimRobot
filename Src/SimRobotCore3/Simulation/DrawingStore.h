/**
 * @file Simulation/DrawingStore.h
 * Declaration of class DrawingStore
 * @author Arne Hasselbring
 */

#pragma once

#include "SimRobotCore3.h"
#include <mutex>
#include <unordered_map>

/**
 * @class DrawingStore
 * The drawings that controllers attached to a scene graph object. They can be set from any thread while the object is drawn.
 */
class DrawingStore final
{
public:
  /**
   * Adds a drawing or replaces a drawing with the same key.
   * @param key The key of the drawing
   * @param drawing The new drawing
   */
  void set(const void* key, SimRobotCore3::Drawing&& drawing)
  {
    std::lock_guard<std::mutex> lock(mutex);
    drawings[key] = std::move(drawing);
  }

  /**
   * Removes a drawing.
   * @param key The key of the drawing
   * @return Whether a drawing with this key existed
   */
  bool remove(const void* key)
  {
    std::lock_guard<std::mutex> lock(mutex);
    return drawings.erase(key) > 0;
  }

  /**
   * Calls a function for each drawing (while the drawings cannot be changed).
   * @param function The function
   */
  template<typename Function>
  void visit(const Function& function) const
  {
    std::lock_guard<std::mutex> lock(mutex);
    for(const auto& pair : drawings)
      function(pair.second);
  }

private:
  mutable std::mutex mutex; /**< Protects the drawings. */
  std::unordered_map<const void*, SimRobotCore3::Drawing> drawings; /**< The drawings by their keys. */
};
