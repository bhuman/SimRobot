#pragma once

#ifdef MACOS
#include <OpenGL/OpenGL.h>
#include <OpenGL/glu.h>
#else
#include <qopengl.h>
#undef near
#undef far
#include <GL/glu.h>
#endif
