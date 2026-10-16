#ifndef HYPERBLADE_MESHBOUNDINGBOX_H
#define HYPERBLADE_MESHBOUNDINGBOX_H

#include "math/vec3.h"

struct MeshBoundingBox
{
    char bValid; // 0x00
    Vec3 vMin;   // 0x04
    Vec3 vMax;   // 0x10

    // Sweeps both boxes in steps under a quarter unit.
    int   StepIntersectTest(const Vec3* pDirOther, const Vec3* pDirOwn, const MeshBoundingBox* pBox);
    Vec3* Min();
    Vec3* Max();
};

static_assert(sizeof(MeshBoundingBox) == 0x1c);

#endif
