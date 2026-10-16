#ifndef HYPERBLADE_MATHOPS_H
#define HYPERBLADE_MATHOPS_H

#include "common/compat.h"
#include <algorithm>
#include <math.h>

struct Vec3;

namespace Math
{

void  MatrixMultiplyInPlace(float (*pA)[4], const float (*pB)[4]);
int   RandInt(int nMin, int nMax);
float Vec3Distance(const Vec3* pA, const Vec3* pB);
Vec3  SphericalToCartesian(const Vec3* pIn);
bool  Vec3Normalize(Vec3* pVec);

// Single-precision dot products of unit vectors can land just outside [-1, 1], which VC6's x87
// intermediates never did, and acos answers NaN there.
inline double Acos(double dCosine)
{
    return acos(std::clamp(dCosine, -1.0, 1.0));
}

inline double Asin(double dSine)
{
    return asin(std::clamp(dSine, -1.0, 1.0));
}

} // namespace Math

#endif
