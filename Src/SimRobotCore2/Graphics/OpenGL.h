#pragma once

#ifdef MACOS
#include <OpenGL/OpenGL.h>
#else
#include <qopengl.h>
#undef near
#undef far
#endif
