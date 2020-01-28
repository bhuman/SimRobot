/**
 * @file Tools/Math.h
 * A collection of general purpose math functions.
 * @author Thomas Röfer
 */

#pragma once

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795029
#define undef_M_PI
#endif

/**
 * Normalizes an angle to the range [-pi .. pi[.
 * @tparam T The type of the angle (usually float or double).
 * @param angle The angle that is normalized.
 * @return The angle normalized to the range [-pi .. pi[.
 */
template<typename T> T normalize(T angle)
{
  if(angle < -T(M_PI) || angle > T(M_PI))
  {
    angle = angle - static_cast<int>(angle / T(2 * M_PI)) * T(2 * M_PI);
    if(angle >= T(M_PI))
      angle = T(angle - T(2 * M_PI));
    else if(angle < -T(M_PI))
      angle = T(angle + T(2 * M_PI));
  }
  return angle;
}

#ifdef undef_M_PI
#undef M_PI
#undef undef_M_PI
#endif