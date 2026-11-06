/**
 * @file Graphics/Metal/MetalCpp.cpp
 *
 * This file compiles the implementation of metal-cpp. It must not use precompiled headers,
 * because the implementation is only generated if the metal-cpp headers are included here first.
 *
 * @author Arne Hasselbring
 */

#ifdef SIMROBOTCORE3_USE_METAL

#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

#endif
