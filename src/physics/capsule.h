// The capsule the two swept tests share: a valid flag, a centre, a radius and a half height.

#ifndef HYPERBLADE_CAPSULE_H
#define HYPERBLADE_CAPSULE_H

#include "math/vec3.h"

struct Capsule
{
    char  bValid;
    Vec3  vCentre;
    float flRadius;
    float flHeight;

    float       Base() const;
    const Vec3* Centre() const;
    Vec3*       Centre();

    int CheckCollision(const Vec3* pDirSelf, const Vec3* pDirOther, const Capsule* pOther);

    static int Overlaps(const Capsule* pMine, const Capsule* pOther);
};

#endif
