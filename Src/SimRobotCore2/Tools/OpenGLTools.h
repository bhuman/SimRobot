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
  static void convertTransformation(const RotationMatrix* rotation, const Vector3f* translation, Pose3f& transformation)
  {
    if(rotation)
      transformation.rotation = *rotation;
    else
      transformation.rotation.setIdentity();
    if(translation)
      transformation.translation = *translation;
    else
      transformation.translation.setZero();
  }

  /**
   * Computes a camera transformation (basically like gluLookAt)
   */
  static void computeCameraTransformation(const Vector3f& eyePosition3D, const Vector3f& center3D, const Vector3f& upVector3D, Pose3f& transformation)
  {
    const Vector3f forward = (center3D - eyePosition3D).normalized();
    const Vector3f side = forward.cross(upVector3D).normalized();
    const Vector3f up = side.cross(forward);

    transformation.rotation.row(0) = side.transpose();
    transformation.rotation.row(1) = up.transpose();
    transformation.rotation.row(2) = -forward.transpose();
    transformation.translation = -transformation.rotation * eyePosition3D;
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
