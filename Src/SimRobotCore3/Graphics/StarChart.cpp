/**
 * @file Graphics/StarChart.cpp
 *
 * This file implements a chart of the stars that can be shown in the views of a scene with a sky.
 *
 * @author Arne Hasselbring
 */

#include "StarChart.h"
#include "Graphics/StarCatalog.h"
#include "SimRobotCore3.h"
#include <QColor>
#include <QFont>
#include <QFontMetrics>
#include <QImage>
#include <QPainter>
#include <QString>
#include <cmath>
#include <cstring>
#include <map>
#include <string>

namespace
{
  constexpr float radius = 300.f; /**< The radius of the sphere in meters (the far plane of the views is at 500 m). */
  constexpr float metersPerPixel = 0.3f; /**< The size of a pixel of a name on the sphere (about the size of a pixel of the view). */

  Vector3f direction(const StarCatalog::Star& star)
  {
    double x, y, z;
    star.direction(x, y, z);
    return Vector3f(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z));
  }

  /** Creates the drawing of a text next to a direction (to the right of it, if the pole is up). */
  std::shared_ptr<const GraphicsContext::PreparedDrawing> createLabel(const std::string& text, const Vector3f& direction, const QColor& color, bool centered)
  {
    QFont font;
    font.setPixelSize(15);
    font.setBold(true);
    const QFontMetrics metrics(font);
    const QString string = QString::fromStdString(text);
    const int width = metrics.horizontalAdvance(string) + 6;
    const int height = metrics.height() + 4;
    QImage image(width, height, QImage::Format_RGBA8888);
    image.fill(Qt::transparent);
    {
      QPainter painter(&image);
      painter.setRenderHint(QPainter::TextAntialiasing);
      painter.setFont(font);
      const QRect rectangle(0, 0, width, height);
      painter.setPen(QColor(0, 0, 0, 200));
      for(int dx = -1; dx <= 1; ++dx)
        for(int dy = -1; dy <= 1; ++dy)
          if(dx || dy)
            painter.drawText(rectangle.translated(dx, dy), Qt::AlignCenter, string);
      painter.setPen(color);
      painter.drawText(rectangle, Qt::AlignCenter, string);
    }

    // The image is seen from the center of the sphere, its top is towards the pole.
    Vector3f up = Vector3f::UnitZ() - direction * direction.z();
    if(up.norm() < 1e-3f)
      up = Vector3f::UnitX() - direction * direction.x();
    up.normalize();
    const Vector3f left = up.cross(direction);
    SimRobotCore3::Drawing drawing;
    SimRobotCore3::Drawing::Image& result = drawing.images.emplace_back();
    Eigen::Map<Matrix3f>(result.rotation) << direction, left, up;
    result.width = static_cast<float>(width) * metersPerPixel;
    result.height = static_cast<float>(height) * metersPerPixel;
    // A name of a star is to the right of it, a name of a constellation is below its center.
    const Vector3f center = direction * radius - (centered ? Vector3f(-8.f * up) : (0.5f * result.width + 2.f) * left);
    Eigen::Map<Vector3f>(result.point) = center;
    result.pixelWidth = static_cast<unsigned int>(width);
    result.pixelHeight = static_cast<unsigned int>(height);
    result.rgba.resize(static_cast<std::size_t>(width) * height * 4);
    for(int y = 0; y < height; ++y)
      std::memcpy(result.rgba.data() + static_cast<std::size_t>(y) * width * 4, image.constScanLine(y), static_cast<std::size_t>(width) * 4);
    drawing.renderOptions = SimRobotCore3::Drawing::disableDepth;
    return std::make_shared<const GraphicsContext::PreparedDrawing>(std::move(drawing));
  }
}

StarChart::StarChart()
{
  std::map<std::string, Vector3f> directions;
  for(const StarCatalog::Star& star : StarCatalog::stars)
  {
    const Vector3f d = direction(star);
    directions[star.name] = d;
    entries.push_back({createLabel(star.name, d, QColor(255, 235, 170), false), d, d, true});
  }

  std::map<std::string, std::pair<Vector3f, int>> centers;
  for(const StarCatalog::Line& line : StarCatalog::lines)
  {
    const Vector3f from = directions.at(line.from);
    const Vector3f to = directions.at(line.to);
    SimRobotCore3::Drawing drawing;
    SimRobotCore3::Drawing::Line& result = drawing.lines.emplace_back();
    Eigen::Map<Vector3f>(result.start) = from * radius;
    Eigen::Map<Vector3f>(result.end) = to * radius;
    result.color = {130, 170, 255, 170};
    drawing.renderOptions = SimRobotCore3::Drawing::disableDepth;
    entries.push_back({std::make_shared<const GraphicsContext::PreparedDrawing>(std::move(drawing)), from, to, false});
    auto& center = centers[line.constellation];
    center.first += from + to;
    center.second += 2;
  }

  // The name of a constellation is at the mean of the directions of its lines.
  for(const auto& [name, center] : centers)
  {
    const Vector3f d = center.first.normalized();
    entries.push_back({createLabel(name, d, QColor(140, 190, 255), true), d, d, true});
  }
}

void StarChart::addTo(GraphicsContext::DrawingList& drawings, const Matrix3f& stars, const Vector3f& viewer, const Vector3f& viewerUp) const
{
  // The rotation from the frame of the equator to the scene is the transpose of the one of the stars.
  const Matrix3f toScene = stars.transpose();
  for(const Entry& entry : entries)
  {
    if((toScene * entry.first).z() <= -0.01f && (toScene * entry.second).z() <= -0.01f)
      continue;
    Matrix3f rotation = toScene;
    if(entry.label)
    {
      // The top of a name points to the pole, so it is turned around its direction until the top points up in the view.
      const Vector3f direction = entry.first;
      Vector3f top = Vector3f::UnitZ() - direction * direction.z();
      if(top.norm() < 1e-3f)
        top = Vector3f::UnitX() - direction * direction.x();
      top = (toScene * top).normalized();
      const Vector3f directionInScene = toScene * direction;
      const Vector3f wanted = viewerUp - directionInScene * viewerUp.dot(directionInScene);
      if(wanted.norm() > 1e-3f)
        rotation = toScene * Eigen::AngleAxisf(std::atan2(directionInScene.dot(top.cross(wanted)), top.dot(wanted)), direction).toRotationMatrix();
    }
    const Matrix4f transformation = (Matrix4f() << rotation, viewer, Eigen::RowVector3f::Zero(), 1.f).finished();
    drawings.add(entry.drawing, transformation.data());
  }
}
