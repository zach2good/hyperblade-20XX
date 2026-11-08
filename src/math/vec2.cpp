// Two dead 2D normalise helpers.

#ifdef HYPERX_V100
#include "common/compat.h"
#include "math/mathglobals.h"
#include "math/vec2.h"
#include <math.h>

// The length is tested before it is rounded to a float, and divided by after.
// FUNCTION: HYPERX100 0x00445370
bool Vec2::Normalize()
{
    const double dLength  = sqrt(y * y + x * x);
    const float  flLength = (float)dLength;

    if (dLength > 1e-07)
    {
        x = x / flLength;
        y = y / flLength;

        return true;
    }

    y = g_flZero;
    x = g_flZero;

    return false;
}

// FUNCTION: HYPERX100 0x00445420
bool Vec2::SetNormalized(float flX, float flY)
{
    const double dLength  = sqrt(flX * flX + flY * flY);
    const float  flLength = (float)dLength;

    if (dLength > 1e-07)
    {
        x = flX / flLength;
        y = flY / flLength;

        return true;
    }

    y = g_flZero;
    x = g_flZero;

    return false;
}
#endif
