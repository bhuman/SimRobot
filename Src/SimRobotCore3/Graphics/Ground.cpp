/**
 * @file Graphics/Ground.cpp
 *
 * This file implements the ground of a scene.
 *
 * @author Arne Hasselbring
 */

#include "Ground.h"
#include "Graphics/Environment.h"
#include "Platform/Assert.h"

void Ground::addParent(Element& element)
{
  Environment* environment = dynamic_cast<Environment*>(&element);
  ASSERT(environment);
  ASSERT(!environment->ground);
  environment->ground = this;
}
