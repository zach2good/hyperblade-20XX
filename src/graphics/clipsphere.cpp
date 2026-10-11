#include "common/compat.h"
#include "globals.h"
#include "graphics/projrender.h"
#include "engine/clipsphere.h"

// Answers two when inside and nought when not. The far test is >= where the others are strict.
// FUNCTION: HYPERX 0x0047fa80
char ClipSphere::FrustumCull(float flRadius)
{
    char cVisible;

    int nLeft   = 0;
    int nRight  = 0;
    int nBottom = 0;
    int nTop    = 0;
    int nNear   = 0;
    int nFar    = 0;

    const float flHalfX = g_projRenderParams.flMaxX * flRadius * 0.5f;

    if (flHalfX + x < 0.0f)
    {
        nLeft = 1;
    }

    if (x - flHalfX > g_projRenderParams.flMaxX * w)
    {
        nRight = 1;
    }

    const float flHalfY = g_projRenderParams.flMaxY * flRadius * 0.5f;

    if (flHalfY + y < 0.0f)
    {
        nBottom = 1;
    }

    if (y - flHalfY > g_projRenderParams.flMaxY * w)
    {
        nTop = 1;
    }

    if (flRadius + z < 0.0f)
    {
        nNear = 1;
    }

    if (z - flRadius >= w)
    {
        nFar = 1;
    }

    const int nOutside = nFar + nNear + nTop + nBottom + nRight + nLeft;

    if (nOutside != 0)
    {
        cVisible = 0;
    }
    else
    {
        cVisible = 2;
    }

    return cVisible;
}
