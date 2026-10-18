#include "common/compat.h"
#include "vector.h"
#include <cmath>
#include "globals.h"
#include "mathglobals.h"

// FUNCTION: HYPERX 0x00406300
Vec3* Vec3::Subtract(Vec3* pOut, const Vec3* pB) const
{
    Vec3 vDelta;

    vDelta.x = x - pB->x;
    vDelta.y = y - pB->y;
    vDelta.z = z - pB->z;

    *pOut = vDelta;

    return pOut;
}

// FUNCTION: HYPERX 0x00430180
bool Vec3::NormalizeTo(Vec3* pOut) const
{
    const auto dLength = sqrt(static_cast<double>(x * x + y * y + z * z));

    if (dLength > g_dNormalizeEpsilon)
    {
        pOut->x = static_cast<float>(x / dLength);
        pOut->y = static_cast<float>(y / dLength);
        pOut->z = static_cast<float>(z / dLength);

        return true;
    }

    pOut->z = g_flZero;
    pOut->y = g_flZero;
    pOut->x = g_flZero;

    return false;
}

// FUNCTION: HYPERX 0x004301f0
bool Vec3::NormalizeScattered(float* pX, float* pY, float* pZ)
{
    const auto dLength = sqrt(static_cast<double>(*pX * *pX + *pZ * *pZ + *pY * *pY));

    if (dLength > g_dNormalizeEpsilon)
    {
        *pX = static_cast<float>(*pX / dLength);
        *pY = static_cast<float>(*pY / dLength);
        *pZ = static_cast<float>(*pZ / dLength);

        return true;
    }

    *pZ = g_flZero;
    *pY = g_flZero;
    *pX = g_flZero;

    return false;
}

// FUNCTION: HYPERX 0x004302f0
Vec3* Vec3::ScaleInPlace(float flScale)
{
    x = flScale * x;
    y = flScale * y;
    z = flScale * z;

    return this;
}

// FUNCTION: HYPERX 0x00430320
void Vec3::ScaleTo(const Vec3* pIn, float flScale, Vec3* pOut)
{
    pOut->x = flScale * pIn->x;
    pOut->y = flScale * pIn->y;
    pOut->z = flScale * pIn->z;
}

// The second point wins only on a strict improvement.
// FUNCTION: HYPERX 0x00430340
Vec3* Vec3::FindClosest(Vec3* pFrom, Vec3* pFirst, Vec3* pSecond)
{
    auto dx = pFirst->x - pFrom->x;
    auto dy = pFirst->y - pFrom->y;
    auto dz = pFirst->z - pFrom->z;

    const auto flToFirst = dx * dx + dy * dy + dz * dz;

    dx = pSecond->x - pFrom->x;
    dy = pSecond->y - pFrom->y;
    dz = pSecond->z - pFrom->z;

    const auto flToSecond = dx * dx + dy * dy + dz * dz;

    if (flToFirst <= flToSecond)
    {
        pSecond = pFirst;
    }

    return pSecond;
}

// FUNCTION: HYPERX 0x0044b050
float Vec3::Dot(const Vec3* pA, const Vec3* pB)
{
    return DotInline(pA, pB);
}

// FUNCTION: HYPERX 0x00466df0
float Vec3::Length(const Vec3* pVec)
{
    return pVec->Magnitude();
}

// FUNCTION: HYPERX 0x004726b0
void Vec3::CrossProduct(const Vec3* pA, const Vec3* pB, Vec3* pOut)
{
    pOut->x = pA->y * pB->z - pA->z * pB->y;
    pOut->y = pA->z * pB->x - pA->x * pB->z;
    pOut->z = pA->x * pB->y - pA->y * pB->x;
}
