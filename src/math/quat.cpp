#include "common/compat.h"
#include "quat.h"
#include "globals.h"
#include "ui/hudglobals.h"
#include "transformobject.h"
#include <math.h>
#include "mathglobals.h"

// Returns the conjugate of the textbook result; the fallback path leaves w near zero, not unit.

// FUNCTION: HYPERX100 0x004a2900
// FUNCTION: HYPERX 0x0042e130
void Quat::FromMatrix(TransformObject* pSource)
{
    MatrixRow* pRows;

    pRows = pSource->GetMatrixPtr();

    float flTrace;

    if ((flTrace = pRows[0].af[0] + pRows[1].af[1] + pRows[2].af[2] + g_flQuatOne) < g_dNormalizeEpsilon)
    {
        flTrace = 0.0f;
    }

    const float flW = (float)(sqrt(flTrace) * 0.5);
    w               = flW;

    if (flW > 0.0f)
    {
        const float flScale = (float)(0.25 / flW);
        x                   = (pRows[1].af[2] - pRows[2].af[1]) * flScale;
        y                   = (pRows[2].af[0] - pRows[0].af[2]) * flScale;
        z                   = (pRows[0].af[1] - pRows[1].af[0]) * flScale;
        Quat::Normalize(&w, &x, &y, &z);
        return;
    }

    float* pflX = &x;
    float* pflY = &y;
    float* pflZ = &z;

    *pflX                   = (g_flQuatOne + pRows[0].af[0]) * 0.5f;
    *pflY                   = (g_flQuatOne + pRows[1].af[1]) * 0.5f;
    const float flDiagonalZ = (g_flQuatOne + pRows[2].af[2]) * 0.5f;
    *pflZ                   = flDiagonalZ;

    if (*pflX >= *pflY)
    {
        if (flDiagonalZ <= *pflX)
        {
            *pflY = pRows[1].af[0] * 0.5f;
            *pflZ = pRows[2].af[0] * 0.5f;
            Vec3::NormalizeScattered(pflX, pflY, pflZ);
            return;
        }
    }
    else if (flDiagonalZ <= *pflY)
    {
        *pflX = pRows[0].af[1] * 0.5f;
        *pflZ = pRows[2].af[1] * 0.5f;
        Vec3::NormalizeScattered(pflX, pflY, pflZ);
        return;
    }

    *pflX = pRows[0].af[2] * 0.5f;
    *pflY = pRows[1].af[2] * 0.5f;
    Vec3::NormalizeScattered(pflX, pflY, pflZ);
}

// Writes the transpose of the textbook rotation; Quat::FromMatrix inverts it exactly.

// FUNCTION: HYPERX100 0x004a2b10
// FUNCTION: HYPERX 0x0042e280
void Quat::ToMatrix(TransformObject* pTarget)
{
    MatrixRow* pRows;

    pRows = pTarget->GetMatrixPtr();

    const float flSquare = w * w;
    pRows[0].af[0]       = (x * x + flSquare) * g_flQuatTwo - g_flQuatOne;
    pRows[2].af[2]       = (z * z + flSquare) * g_flQuatTwo - g_flQuatOne;
    pRows[1].af[1]       = (y * y + flSquare) * g_flQuatTwo - g_flQuatOne;

    float flFirst  = y * x;
    float flSecond = z * w;
    pRows[0].af[1] = (flSecond + flFirst) * g_flQuatTwo;
    pRows[1].af[0] = (flFirst - flSecond) * g_flQuatTwo;

    flFirst        = z * x;
    flSecond       = y * w;
    pRows[2].af[0] = (flSecond + flFirst) * g_flQuatTwo;
    pRows[0].af[2] = (flFirst - flSecond) * g_flQuatTwo;

    flFirst        = y * z;
    flSecond       = w * x;
    pRows[1].af[2] = (flSecond + flFirst) * g_flQuatTwo;
    pRows[2].af[1] = (flFirst - flSecond) * g_flQuatTwo;
}

// FUNCTION: HYPERX100 0x00445730
// FUNCTION: HYPERX 0x00430260
bool __fastcall Quat::Normalize(float* pW, float* pX, float* pY, float* pZ)
{
    const float flLength = (float)sqrt(*pW * *pW + *pX * *pX + *pY * *pY + *pZ * *pZ);

    if (flLength > g_dNormalizeEpsilon)
    {
        *pW = *pW / flLength;
        *pX = *pX / flLength;
        *pY = *pY / flLength;
        *pZ = *pZ / flLength;

        return true;
    }

    *pZ = g_flZero;
    *pY = g_flZero;
    *pX = g_flZero;
    *pW = g_flZero;

    return false;
}

#ifdef HYPERX_V100
// Equal vector parts take a shortcut that subtracts one from w instead of the dot product.
// FUNCTION: HYPERX100 0x004a3290
void Quat::MultiplyInPlace(const Quat* pRhs)
{
    if (x == pRhs->x && y == pRhs->y && z == pRhs->z)
    {
        w = w * pRhs->w - 1.0f;
        x = x * pRhs->w + w * pRhs->x;
        y = y * pRhs->w + pRhs->y * w;
        z = z * pRhs->w + pRhs->z * w;

        return;
    }

    w = ((w * pRhs->w - y * pRhs->y) - x * pRhs->x) - z * pRhs->z;
    x = x * pRhs->w + pRhs->x * w + (y * pRhs->z - pRhs->y * z);
    y = y * pRhs->w + pRhs->y * w + (z * pRhs->x - x * pRhs->z);
    z = z * pRhs->w + pRhs->z * w + (x * pRhs->y - y * pRhs->x);
}
#endif
