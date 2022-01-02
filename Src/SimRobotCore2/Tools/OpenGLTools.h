/**
 * @file OpenGLTools.h
 * Utility functions for using OpenGL
 * @author Colin Graf
 */

#pragma once

#include "Tools/Math/Eigen.h"
#include "Tools/Math/Pose3f.h"
#include "Tools/Math/RotationMatrix.h"

class OpenGLTools
{
public:
  /**
   * Converts a pose to the OpenGL format
   * @param pose The pose to convert
   * @param transformation The converted pose
   */
  static void convertTransformation(const Pose3f& pose, Matrix4f& transformation)
  {
    transformation.topLeftCorner<3, 3>() = pose.rotation;
    transformation.topRightCorner<3, 1>() = pose.translation;
    transformation(3, 0) = 0.f;
    transformation(3, 1) = 0.f;
    transformation(3, 2) = 0.f;
    transformation(3, 3) = 1.f;
  }

  /**
   * Converts a pose to the OpenGL format
   * @param rotation The rotational part of the pose to convert
   * @param translation The translational part of the pose to convert
   * @param transformation The converted pose
   */
  static void convertTransformation(const RotationMatrix* rotation, const Vector3f* translation, Matrix4f& transformation)
  {
    if(rotation)
      transformation.topLeftCorner<3, 3>() = *rotation;
    else
      transformation.topLeftCorner<3, 3>() = Matrix3f::Identity();
    if(translation)
      transformation.topRightCorner<3, 1>() = *translation;
    else
      transformation.topRightCorner<3, 1>() = Vector3f::Zero();
    transformation(3, 0) = 0.f;
    transformation(3, 1) = 0.f;
    transformation(3, 2) = 0.f;
    transformation(3, 3) = 1.f;
  }

  /**
   * Computes a camera transformation (basically like gluLookAt)
   */
  static void computeCameraTransformation(const Vector3f& eyePosition3D, const Vector3f& center3D, const Vector3f& upVector3D, Matrix4f& transformation)
  {
    const Vector3f forward = (center3D - eyePosition3D).normalized();
    const Vector3f side = forward.cross(upVector3D).normalized();
    const Vector3f up = side.cross(forward);

    transformation.row(0).head<3>() = side.transpose();
    transformation.row(1).head<3>() = up.transpose();
    transformation.row(2).head<3>() = -forward.transpose();
    transformation.col(3).head<3>() = -transformation.topLeftCorner<3, 3>() * eyePosition3D;
    transformation(3, 0) = 0.f;
    transformation(3, 1) = 0.f;
    transformation(3, 2) = 0.f;
    transformation(3, 3) = 1.f;
  }

  /**
   * Computes a perspective projection matrix (basically like gluPerspective)
   * @param fovY angle of view in y-direction (in radian)
   * @param aspect An aspect ratio to determine the angle of view in x-direction
   * @param near The distance from the viewer to the near clipping plane
   * @param far The distance from the viewer to the far clipping plane
   * @param matrix The resulting matrix
   */
  static void computePerspective(float fovY, float aspect, float near, float far, Matrix4f& matrix);
};
