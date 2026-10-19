#ifndef HYPERBLADE_RANDROLL_H
#define HYPERBLADE_RANDROLL_H

#include "math/squirrel5.h"

namespace Math
{

// A uniform roll in [-1, 1], rounded to a float before it is doubled.
inline float RandSigned()
{
    const float flUnit = static_cast<float>(Math::Rand()) * (1.0f / static_cast<float>(Math::kRandMax));

    return flUnit * 2.0f - 1.0f;
}

// The roll's magnitude; nought itself is negated, as in the original.
inline float RandMagnitude()
{
    float flRoll = RandSigned();

    if (flRoll <= 0.0f)
    {
        flRoll = -flRoll;
    }

    return flRoll;
}

} // namespace Math

#endif
