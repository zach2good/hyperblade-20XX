// The light direction table's 16-byte element.

#ifndef HYPERBLADE_VECTOR_H
#define HYPERBLADE_VECTOR_H

#include "math/vec3.h"
#include "math/mathops.h"

struct LightDir
{
    float flUnused;
    Vec3  dir;
};

#endif
