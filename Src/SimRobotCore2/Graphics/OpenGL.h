#pragma once

#ifdef MACOS
#include <OpenGL/gl.h>
#else
#include <qopengl.h>
#undef near
#undef far
#endif
