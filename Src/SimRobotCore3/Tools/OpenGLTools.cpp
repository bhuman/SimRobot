/**
 * @file OpenGLTools.cpp
 * Utility functions for using OpenGL
 * @author Colin Graf
 */

#include "OpenGLTools.h"
#include <cmath>

void OpenGLTools::computePerspective(float fovY, float aspect, float near, float far, Matrix4f& matrix)
{
  matrix = Matrix4f::Zero();
  matrix(1, 1) = 1.f / std::tan(fovY * 0.5f);
  matrix(0, 0) = matrix(1, 1) / aspect;
  const float nearMFarInv = 1.f / (near - far);
  matrix(2, 2) = (far + near) * nearMFarInv;
  matrix(3, 2) = -1.f;
  matrix(2, 3) = 2.f * far * near * nearMFarInv;
}
