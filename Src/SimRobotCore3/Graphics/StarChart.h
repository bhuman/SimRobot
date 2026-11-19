/**
 * @file Graphics/StarChart.h
 *
 * This file declares a chart of the stars that can be shown in the views of a scene with a sky: the names of the stars of the
 * catalog and the lines of some constellations (see Graphics/StarCatalog.h), as drawings on a sphere around the viewer. It
 * helps to check the sky (and to find a constellation).
 *
 * @author Arne Hasselbring
 */

#pragma once

#include "Graphics/GraphicsContext.h"
#include "Tools/Math/Eigen.h"
#include <memory>
#include <vector>

/**
 * @class StarChart
 * The drawings of the star names and constellations in the frame of the equator.
 */
class StarChart
{
public:
  /** Creates the drawings (including the images of the names, so it needs the Qt GUI). */
  StarChart();

  /**
   * Adds the drawings of the parts that are above the horizon to a list of drawings.
   * @param drawings The list of drawings.
   * @param stars The rotation from directions in the scene to directions in the frame of the equator (see \c GraphicsContext::SkyState).
   * @param viewer The position of the viewer in the scene (the center of the sphere).
   * @param viewerUp The direction that is up in the view (in the scene), so that the names are upright.
   */
  void addTo(GraphicsContext::DrawingList& drawings, const Matrix3f& stars, const Vector3f& viewer, const Vector3f& viewerUp) const;

private:
  /** A part of the chart: a name or a line. */
  struct Entry
  {
    std::shared_ptr<const GraphicsContext::PreparedDrawing> drawing; /**< What is drawn. */
    Vector3f first, second; /**< The directions (in the frame of the equator) to the ends of the part. */
    bool label; /**< Whether the part is a name, which is turned around its direction so that it is upright in the view. */
  };

  std::vector<Entry> entries; /**< All parts. */
};
