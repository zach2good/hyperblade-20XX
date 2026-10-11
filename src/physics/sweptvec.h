// The step the two /Od swept-collision tests share.

#ifndef HYPERBLADE_PHYSICS_SWEPTVEC_H
#define HYPERBLADE_PHYSICS_SWEPTVEC_H

#include "math/vec3.h"

inline void VecAddTo(Vec3* pDst, const Vec3* pDelta)
{
    pDst->x = pDelta->x + pDst->x;
    pDst->y = pDelta->y + pDst->y;
    pDst->z = pDelta->z + pDst->z;
}

#endif
