/**
 * @file Simulation/DrawingStore.h
 * Declaration of class DrawingStore
 * @author Arne Hasselbring
 */

#pragma once

#include "SimRobotCore3.h"
#include "Graphics/GraphicsContext.h"
#include <memory>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>

/**
 * @class DrawingStore
 * The drawings that controllers attached to a scene graph object. They can be set and removed from any thread while
 * the object is drawn (which only happens in the thread that draws the scenes).
 */
class DrawingStore final
{
public:
  /**
   * Adds a drawing or replaces a drawing with the same key. The drawing is only prepared when it is drawn for the first time.
   * @param key The key of the drawing
   * @param drawing The new drawing
   */
  void set(const void* key, SimRobotCore3::Drawing&& drawing)
  {
    std::shared_ptr<Entry> entry = std::make_shared<Entry>(std::move(drawing));
    {
      std::lock_guard<std::mutex> lock(mutex);
      drawings[key].swap(entry);
    }
    // The previous drawing (if it is not drawn at the moment) is freed here, without the lock.
  }

  /**
   * Removes a drawing.
   * @param key The key of the drawing
   * @return Whether a drawing with this key existed
   */
  bool remove(const void* key)
  {
    std::shared_ptr<Entry> entry; // Declared before the lock, so that the drawing is freed without it.
    std::lock_guard<std::mutex> lock(mutex);
    const auto i = drawings.find(key);
    if(i == drawings.end())
      return false;
    entry.swap(i->second);
    drawings.erase(i);
    return true;
  }

  /**
   * Adds all drawings to the list of drawings of a frame. Drawings that have not been drawn before are prepared.
   * @param list The list of drawings of a frame
   * @param objectTransformation The column-major 4x4 matrix that transforms the coordinate system of the object into the world
   */
  void collect(GraphicsContext::DrawingList& list, const float* objectTransformation) const
  {
    // The drawings are prepared without the lock of the map, so that setting a drawing does not have to wait for that.
    std::vector<std::shared_ptr<Entry>> entries;
    {
      std::lock_guard<std::mutex> lock(mutex);
      if(drawings.empty())
        return;
      entries.reserve(drawings.size());
      for(const auto& pair : drawings)
        entries.push_back(pair.second);
    }
    for(const std::shared_ptr<Entry>& entry : entries)
    {
      if(entry->drawing)
      {
        entry->preparedDrawing = std::make_shared<const GraphicsContext::PreparedDrawing>(std::move(*entry->drawing));
        entry->drawing.reset();
      }
      list.add(entry->preparedDrawing, objectTransformation);
    }
  }

private:
  /** A drawing, which is prepared when it is drawn for the first time. */
  struct Entry
  {
    /**
     * Constructor.
     * @param drawing The drawing
     */
    explicit Entry(SimRobotCore3::Drawing&& drawing) : drawing(std::move(drawing)) {}

    std::optional<SimRobotCore3::Drawing> drawing; /**< The drawing as set by the controller (until it is prepared). */
    std::shared_ptr<const GraphicsContext::PreparedDrawing> preparedDrawing; /**< The prepared drawing (once it has been drawn). */
  };

  mutable std::mutex mutex; /**< Protects the map (but not the drawings, which only the thread that draws the scenes uses). */
  std::unordered_map<const void*, std::shared_ptr<Entry>> drawings; /**< The drawings by their keys. */
};
