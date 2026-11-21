/**
 * @file System.cpp
 * Implementation of class System
 */

#ifdef WINDOWS
#include <Windows.h>
#elif defined MACOS
#include <ctime>
#include <unistd.h>
#else
#include <ctime>
#include <unistd.h>
#endif

#include "System.h"

unsigned int System::getTime()
{
#ifdef WINDOWS
  return GetTickCount();
#elif defined MACOS
  return static_cast<unsigned>(clock_gettime_nsec_np(CLOCK_UPTIME_RAW) / 1000000);
#else
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return static_cast<unsigned int>(ts.tv_sec * 1000 + ts.tv_nsec / 1000000l);
#endif
}
