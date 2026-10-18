// Two upright cylinders tested per substep; the caller inverts the answer. Compiled at /Od.

#include "common/compat.h"
#include <algorithm>
#include "math/vector.h"
#include "capsule.h"

#include <cmath>

#include "sweptvec.h"

inline float Capsule::Base() const
{
    float flBase;

    flBase = vCentre.y;

    return flBase;
}

inline const Vec3* Capsule::Centre() const
{
    return &vCentre;
}

inline Vec3* Capsule::Centre()
{
    return &vCentre;
}

inline int Capsule::Overlaps(const Capsule* pMine, const Capsule* pOther)
{
    Vec3  vDelta;
    float flRadii;

    vDelta  = pOther->Centre()->Minus(pMine->Centre()).Copy();
    flRadii = pOther->flRadius + pMine->flRadius;

    return (std::min)(pOther->vCentre.y + pOther->flHeight, pMine->vCentre.y + pMine->flHeight) >= (std::max)(pOther->Base(), pMine->Base()) && flRadii * flRadii >= vDelta.x * vDelta.x + vDelta.z * vDelta.z;
}

// FUNCTION: HYPERX 0x00429bc0
int Capsule::CheckCollision(const Vec3* pDirSelf, const Vec3* pDirOther, const Capsule* pOther)
{
    float   flDistA;
    float   flDistB;
    Capsule bodyOther;
    int     nStepsSelf;
    int     i;
    int     nStepsOther;
    int     bIntersect;
    Vec3    vDeltaA;
    Capsule bodyMine;
    Vec3    vDeltaB;
    int     nStepsUsed;

    flDistA     = (float)sqrt(static_cast<double>(pDirSelf->z * pDirSelf->z + (pDirSelf->y * pDirSelf->y + pDirSelf->x * pDirSelf->x)));
    flDistB     = (float)sqrt(static_cast<double>(pDirOther->z * pDirOther->z + (pDirOther->y * pDirOther->y + pDirOther->x * pDirOther->x)));
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

    nStepsUsed = (std::max)(nStepsSelf, nStepsOther);
    vDeltaA    = pDirSelf->Scaled((float)(1.0 / nStepsUsed));
    vDeltaB    = pDirOther->Scaled((float)(1.0 / nStepsUsed));
    bodyMine   = *this;
    bodyOther  = *pOther;
    bIntersect = 0;

    for (i = 0; i < nStepsUsed; i++)
    {
        bIntersect = Capsule::Overlaps(&bodyMine, &bodyOther);

        if (bIntersect != 0)
        {
            break;
        }

        VecAddTo(bodyMine.Centre(), &vDeltaA);
        VecAddTo(bodyOther.Centre(), &vDeltaB);
    }

    return bIntersect;
}
