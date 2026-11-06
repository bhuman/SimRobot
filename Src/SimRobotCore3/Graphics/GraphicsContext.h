#ifdef SIMROBOTCORE3_USE_OPENGL
#include "Graphics/OpenGL/GraphicsContext.h"
#elif defined SIMROBOTCORE3_USE_METAL
#include "Graphics/Metal/GraphicsContext.h"
#else
#error "No graphics API selected."
#endif
