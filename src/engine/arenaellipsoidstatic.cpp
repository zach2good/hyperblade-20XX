// The arena's collision ellipsoid and the three semi-axes it is built from.

#include "common/compat.h"
#include "physics/collisionellipsoid.h"
#include "common/staticinit.h"

// A hundredth and nineteen twentieths.
// GLOBAL: HYPERX 0x004b345c
extern float g_flArenaEllipsoidFriction;
// GLOBAL: HYPERX 0x004b3460
extern float g_flArenaEllipsoidElasticity;

float g_flArenaEllipsoidZ = 70.0f * g_flTrackingEllipsoidScale;
float g_flArenaEllipsoidY = 125.0f * g_flTrackingEllipsoidScale;
float g_flArenaEllipsoidX = 70.0f * g_flTrackingEllipsoidScale;

CollisionEllipsoid g_arenaEllipsoid(g_flArenaEllipsoidX, g_flArenaEllipsoidY, g_flArenaEllipsoidZ, g_flArenaEllipsoidFriction, g_flArenaEllipsoidElasticity);
