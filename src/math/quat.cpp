#include "common/compat.h"
#include "quat.h"
#include "globals.h"
#include "ui/hudglobals.h"
#include "transformobject.h"
#include <cmath>
#include "mathglobals.h"

// Returns the conjugate of the textbook result; the fallback path leaves w near zero, not unit.

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

    const auto flW = static_cast<float>(sqrt(static_cast<double>(flTrace)) * 0.5);
    w              = flW;

    if (flW > 0.0f)
    {
        const auto flScale = static_cast<float>(0.25 / flW);
        x                  = (pRows[1].af[2] - pRows[2].af[1]) * flScale;
        y                  = (pRows[2].af[0] - pRows[0].af[2]) * flScale;
        z                  = (pRows[0].af[1] - pRows[1].af[0]) * flScale;
        Quat::Normalize(&w, &x, &y, &z);
        return;
    }

    auto* const pflX = &x;
    auto* const pflY = &y;
    auto* const pflZ = &z;

    *pflX                  = (g_flQuatOne + pRows[0].af[0]) * 0.5f;
    *pflY                  = (g_flQuatOne + pRows[1].af[1]) * 0.5f;
    const auto flDiagonalZ = (g_flQuatOne + pRows[2].af[2]) * 0.5f;
    *pflZ                  = flDiagonalZ;

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

// FUNCTION: HYPERX 0x0042e280
void Quat::ToMatrix(TransformObject* pTarget)
{
    MatrixRow* pRows;

    pRows = pTarget->GetMatrixPtr();

    const auto flSquare = w * w;
    pRows[0].af[0]      = (x * x + flSquare) * g_flQuatTwo - g_flQuatOne;
    pRows[2].af[2]      = (z * z + flSquare) * g_flQuatTwo - g_flQuatOne;
    pRows[1].af[1]      = (y * y + flSquare) * g_flQuatTwo - g_flQuatOne;

    auto flFirst   = y * x;
    auto flSecond  = z * w;
    pRows[0].af[1] = (flSecond + flFirst) * g_flQuatTwo;
    pRows[1].af[0] = (flFirst - flSecond) * g_flQuatTwo;

    flFirst        = z * x;
    flSecond       = y * w;
    pRows[2].af[0] = (flSecond + flFirst) * g_flQuatTwo;
    pRows[0].af[2] = (flFirst - flSecond) * g_flQuatTwo;

    flFirst = y;
    flFirst *= z;
    flSecond = w;
    flSecond *= x;
    pRows[1].af[2] = (flSecond + flFirst) * g_flQuatTwo;
    pRows[2].af[1] = (flFirst - flSecond) * g_flQuatTwo;
}

// FUNCTION: HYPERX 0x00430260
bool Quat::Normalize(float* pW, float* pX, float* pY, float* pZ)
{
    const auto flLength = static_cast<float>(sqrt(static_cast<double>(*pW * *pW + *pX * *pX + *pY * *pY + *pZ * *pZ)));

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
