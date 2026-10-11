// The swept AABB test, marched in steps under a quarter unit; compiled at /Od.

#include "common/compat.h"

#include <math.h>

#include "graphics/meshboundingbox.h"
#include "physics/sweptvec.h"

#pragma optimize("", off)

inline Vec3* MeshBoundingBox::Min()
{
    return &vMin;
}

inline Vec3* MeshBoundingBox::Max()
{
    return &vMax;
}

// FUNCTION: HYPERX 0x00429740
int MeshBoundingBox::StepIntersectTest(const Vec3* pDirOther, const Vec3* pDirOwn, const MeshBoundingBox* pBox)
{
    float           flDistA;
    float           flDistB;
    MeshBoundingBox bodyOther;
    int             nStepsSelf;
    int             i;
    int             nStepsOther;
    int             bIntersect;
    Vec3            vDeltaA;
    MeshBoundingBox bodyMine;
    Vec3            vDeltaB;
    int             nStepsUsed;

    flDistA     = (float)sqrt(pDirOther->z * pDirOther->z + (pDirOther->y * pDirOther->y + pDirOther->x * pDirOther->x));
    flDistB     = (float)sqrt(pDirOwn->z * pDirOwn->z + (pDirOwn->y * pDirOwn->y + pDirOwn->x * pDirOwn->x));
    nStepsSelf  = 1;
    nStepsOther = 1;

    while (flDistA > 0.25f)
    {
        nStepsSelf <<= 1;
        flDistA /= 2.0f;
    }

    while (flDistB > 0.25f)
    {
        nStepsOther <<= 1;
        flDistB /= 2.0f;
    }

    nStepsUsed = max(nStepsSelf, nStepsOther);
    vDeltaA    = pDirOther->Scaled((float)(1.0 / nStepsUsed));
    vDeltaB    = pDirOwn->Scaled((float)(1.0 / nStepsUsed));
    bodyMine   = *this;
    bodyOther  = *pBox;
    bIntersect = 0;

    for (i = 0; i < nStepsUsed; i++)
    {
        bIntersect = min(bodyOther.vMax.x, bodyMine.vMax.x) >= max(bodyOther.vMin.x, bodyMine.vMin.x) && min(bodyOther.vMax.y, bodyMine.vMax.y) >= max(bodyOther.vMin.y, bodyMine.vMin.y) && min(bodyOther.vMax.z, bodyMine.vMax.z) >= max(bodyOther.vMin.z, bodyMine.vMin.z);

        if (bIntersect != 0)
        {
            break;
        }

        VecAddTo(bodyMine.Min(), &vDeltaA);
        VecAddTo(bodyMine.Max(), &vDeltaA);
        VecAddTo(bodyOther.Min(), &vDeltaB);
        VecAddTo(bodyOther.Max(), &vDeltaB);
    }

    return bIntersect;
}

#pragma optimize("", on)
