// The trackers' ellipsoid sits 2.2 inside the arena's, so the camera steers inside the wall.

#include "common/compat.h"
#include "physics/collisionellipsoid.h"
#include "common/staticinit.h"

extern const float g_flZeroTracking    = 0.0f;
float              g_flUnusedTrackingA = g_flZeroTracking;
float              g_flUnusedTrackingB = g_flZeroTracking;
float              g_flUnusedTrackingC = g_flZeroTracking;

CollisionEllipsoid g_camTrackEllipsoid(70.0f * g_flTrackingEllipsoidScale - 2.2f,
                                       125.0f * g_flTrackingEllipsoidScale - 2.2f,
                                       70.0f * g_flTrackingEllipsoidScale - 2.2f,
                                       0.01f,
                                       0.5f);
