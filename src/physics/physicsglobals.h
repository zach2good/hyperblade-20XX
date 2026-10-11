// Globals only the physics files read; src/globals.cpp defines them in .data order.

#ifndef HYPERBLADE_PHYSICSGLOBALS_H
#define HYPERBLADE_PHYSICSGLOBALS_H
#include "common/compat.h"

#include "globals.h"

// How close to the surface a point has to be to count as on it.
// GLOBAL: HYPERX 0x004b1598
extern const double g_dSurfaceTolerance;

// GLOBAL: HYPERX 0x004b2bd4
extern const float g_flCollisionOne;

// CollisionEllipsoid::NormalAtPoint's gradient over the dominant axis: the divisor that axis
// gives, and each other axis's ratio to it.
struct EllipsoidRatios
{
    float flZ;
    float flX;
    float flDivisor;
    float flY;
};

// A global, as in the original, so CollisionEllipsoid::NormalAtPoint is not re-entrant.
// GLOBAL: HYPERX 0x004cb790
extern EllipsoidRatios g_ellipsoidRatios;

#endif
