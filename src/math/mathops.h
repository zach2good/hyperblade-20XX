#ifndef HYPERBLADE_MATHOPS_H
#define HYPERBLADE_MATHOPS_H

#include "common/compat.h"

struct Vec3;

namespace Math
{

void __fastcall MatrixMultiplyInPlace(float (*pA)[4], const float (*pB)[4]);
int __cdecl     RandInt(int nMin, int nMax);
float           Vec3Distance(const Vec3* pA, const Vec3* pB);
Vec3            SphericalToCartesian(const Vec3* pIn);
bool __fastcall Vec3Normalize(Vec3* pVec);

#ifdef HYPERX_V100
// SphericalToCartesian's inverse: x the azimuth, y the length and z the polar angle.
Vec3 CartesianToSpherical(const Vec3* pIn);
#endif

} // namespace Math

#endif
