/**
 * @file Graphics/Environment.h
 *
 * This file declares the environment element of a scene: its background, its ambient light, how the brightness is
 * mapped to colors and its lights (including the sky, which is a light that follows the time of day).
 *
 * @author Arne Hasselbring
 */

#pragma once

#include "Parser/ElementCore3.h"
#include <list>

class Backdrop;
class Ground;
class Light;
class Sky;

/**
 * @class Environment
 * The environment of a scene
 */
class Environment : public ElementCore3
{
public:
  float color[4] = {0.f, 0.f, 0.f, 1.f}; /**< The background (clear color). */
  float ambient[3] = {0.4f, 0.4f, 0.4f}; /**< The color of the ambient light that comes from above (including its intensity). */
  float ambientGround[3] = {0.4f, 0.4f, 0.4f}; /**< The color of the ambient light that comes from below (including its intensity). */
  float exposure = 1.f; /**< The factor that the brightness of lit surfaces is multiplied with before tone mapping. */
  bool filmicToneMapping = false; /**< Whether a filmic curve is used for tone mapping (otherwise the curve of Reinhard). */
  std::list<Light*> lights; /**< The lights of the environment (including the sky). */
  Sky* sky = nullptr; /**< The sky of the environment (which is also one of its lights) or \c nullptr if there is none. */
  Backdrop* backdrop = nullptr; /**< The backdrop of the environment (a room around the scene) or \c nullptr if there is none. */
  Ground* ground = nullptr; /**< The ground of the environment (an infinite plane around the scene) or \c nullptr if there is none. */
};
