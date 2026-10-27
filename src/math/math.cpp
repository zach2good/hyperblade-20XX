// Vector distance and normalising, a random integer in a range, and a spherical conversion.

#include "common/compat.h"
#include "globals.h"
#include "mathglobals.h"
#include "math/vector.h"
#include <math.h>
#include "math/mathops.h"
#include "math/randroll.h"

// The bias that makes the range inclusive.
// GLOBAL: HYPERX 0x004b39f0
extern const float g_flRandRangeBias;

// A = A * B for four by four matrices, with B read only.
// FUNCTION: HYPERX 0x0042ff20
void __fastcall Math::MatrixMultiplyInPlace(float (*pA)[4], const float (*pB)[4])
{
    const float flRow0X = pA[0][0];
    const float flRow0Y = pA[0][1];
    const float flRow0Z = pA[0][2];
    const float flRow0W = pA[0][3];

    pA[0][0] = ((flRow0W * pB[3][0] + flRow0Z * pB[2][0]) + flRow0X * pB[0][0]) + flRow0Y * pB[1][0];
    pA[0][1] = (((flRow0X * pB[0][1]) + flRow0Y * pB[1][1]) + flRow0W * pB[3][1]) + flRow0Z * pB[2][1];
    pA[0][2] = (((flRow0X * pB[0][2]) + flRow0Y * pB[1][2]) + flRow0W * pB[3][2]) + flRow0Z * pB[2][2];
    pA[0][3] = ((flRow0W * pB[3][3] + flRow0Z * pB[2][3]) + flRow0X * pB[0][3]) + flRow0Y * pB[1][3];

    const float flRow1X = pA[1][0];
    const float flRow1Y = pA[1][1];
    const float flRow1Z = pA[1][2];
    const float flRow1W = pA[1][3];

    pA[1][0] = ((flRow1W * pB[3][0] + flRow1Z * pB[2][0]) + flRow1X * pB[0][0]) + flRow1Y * pB[1][0];
    pA[1][1] = (((flRow1X * pB[0][1]) + flRow1Y * pB[1][1]) + flRow1W * pB[3][1]) + flRow1Z * pB[2][1];
    pA[1][2] = (((flRow1X * pB[0][2]) + flRow1Y * pB[1][2]) + flRow1W * pB[3][2]) + flRow1Z * pB[2][2];
    pA[1][3] = ((flRow1W * pB[3][3] + flRow1Z * pB[2][3]) + flRow1X * pB[0][3]) + flRow1Y * pB[1][3];

    const float flRow2X = pA[2][0];
    const float flRow2Y = pA[2][1];
    const float flRow2Z = pA[2][2];
    const float flRow2W = pA[2][3];

    pA[2][0] = ((flRow2W * pB[3][0] + flRow2Z * pB[2][0]) + flRow2X * pB[0][0]) + flRow2Y * pB[1][0];
    pA[2][1] = (((flRow2X * pB[0][1]) + flRow2Y * pB[1][1]) + flRow2W * pB[3][1]) + flRow2Z * pB[2][1];
    pA[2][2] = (((flRow2X * pB[0][2]) + flRow2Y * pB[1][2]) + flRow2W * pB[3][2]) + flRow2Z * pB[2][2];
    pA[2][3] = ((flRow2W * pB[3][3] + flRow2Z * pB[2][3]) + flRow2X * pB[0][3]) + flRow2Y * pB[1][3];

    const float flRow3X = pA[3][0];
    const float flRow3Y = pA[3][1];
    const float flRow3Z = pA[3][2];
    const float flRow3W = pA[3][3];

    pA[3][0] = ((flRow3W * pB[3][0] + flRow3Z * pB[2][0]) + flRow3X * pB[0][0]) + flRow3Y * pB[1][0];
    pA[3][1] = (((flRow3X * pB[0][1]) + flRow3Y * pB[1][1]) + flRow3W * pB[3][1]) + flRow3Z * pB[2][1];
    pA[3][2] = (((flRow3X * pB[0][2]) + flRow3Y * pB[1][2]) + flRow3W * pB[3][2]) + flRow3Z * pB[2][2];
    pA[3][3] = ((flRow3W * pB[3][3] + flRow3Z * pB[2][3]) + flRow3X * pB[0][3]) + flRow3Y * pB[1][3];
}

// FUNCTION: HYPERX 0x00430110
bool __fastcall Math::Vec3Normalize(Vec3* pVec)
{
    const double dLength = sqrt(Vec3::DotInline(pVec, pVec));

    if (dLength > g_dNormalizeEpsilon)
    {
        pVec->x = (float)(pVec->x / dLength);
        pVec->y = (float)(pVec->y / dLength);
        pVec->z = (float)(pVec->z / dLength);

        return true;
    }

    pVec->z = g_flZero;
    pVec->y = g_flZero;
    pVec->x = g_flZero;

    return false;
}

// FUNCTION: HYPERX 0x004753e0
int __cdecl Math::RandInt(int nMin, int nMax)
{
    float flRoll = Math::RandSigned();

    if (flRoll <= 0.0f)
    {
        flRoll = -flRoll;
    }

    nMax -= nMin;

    return (int)(flRoll * (nMax + g_flRandRangeBias * 2.0f) + (nMin - g_flRandRangeBias) + 0.5);
}

// FUNCTION: HYPERX 0x0047ab30
Vec3 Math::SphericalToCartesian(const Vec3* pIn)
{
    Vec3 vDir;

    vDir.x = (float)((cos(pIn->x) * sin(pIn->z)) * pIn->y);

    const float flSinZ = (float)sin(pIn->z);

    vDir.y = (float)((sin(pIn->x) * flSinZ) * pIn->y);

    vDir.z = (float)(cos(pIn->z) * pIn->y);

    Math::Vec3Normalize(&vDir);

    return vDir;
}
