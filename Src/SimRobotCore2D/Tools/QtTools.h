/**
 * @file QtTools.h
 *
 * This file declares a class that contains pose conversion functions between Box2D and Qt.
 *
 * @author Arne Hasselbring
 */

#pragma once

#include "Tools/Math/Constants.h"
#include <box2d/b2_math.h>
#include <QTransform>

class QtTools
{
public:
  /**
   * Converts a Box2D pose to a Qt transformation.
   * @param pose The Box2D pose.
   * @param transformation The resulting Qt transform.
   */
  static void convertTransformation(const b2Transform& pose, QTransform& transformation)
  {
    transformation = QTransform(pose.q.c, -pose.q.s, pose.q.s, pose.q.c, pose.p.x, pose.p.y);
  }

  /**
   * Converts a Box2D rotation and translation to a Qt transformation.
   * @param rotation The rotation of the Box2D pose (or nullptr).
   * @param translation The translation of the Box2D pose (or nullptr).
   * @param transformation The resulting Qt transform.
   */
  static void convertTransformation(const b2Rot* rotation, const b2Vec2* translation, QTransform& transformation)
  {
    transformation = QTransform();
    if(translation)
      transformation.translate(translation->x, translation->y);
    if(rotation)
      transformation.rotate(rotation->GetAngle() * 180.f / pi);
  }

  /**
   * Converts an angle and a Box2D translation to a Qt transformation.
   * @param rotation The angle of the pose.
   * @param translation The translation of the pose.
   * @param transformation The resulting Qt transform.
   */
  static void convertTransformation(const float rotation, const b2Vec2& translation, QTransform& transformation)
  {
    transformation = QTransform().translate(translation.x, translation.y).rotate(rotation * 180.f / pi);
  }
};
