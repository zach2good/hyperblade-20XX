// The tangent: two samples a tenth apart, stepping back where forward would leave the range.

#include "common/compat.h"
#include "ui/hudglobals.h"
#include "vector.h"
#include "spline.h"
#include "math/splinecurve.h"

// GLOBAL: HYPERX 0x004b40c8
extern const float g_flSplineTangentStep;

// FUNCTION: HYPERX100 0x004362e0
// FUNCTION: HYPERX 0x00492df0
void SplineCurve::ComputeTangent(float flT, Vec3* pTangent)
{
    float flStep;

    if ((g_flSplineTangentStep + flT >= m_flUpper) || (g_flSplineTangentStep + flT <= m_flLower))
    {
        flStep = flT - g_flSplineTangentStep;
    }
    else
    {
        flStep = g_flSplineTangentStep + flT;
    }

    const float flAhead = m_pfnEval(flStep, m_flA, m_flB, m_flC, m_flD);

    pTangent->x = flAhead - m_pfnEval(flT, m_flA, m_flB, m_flC, m_flD);
    pTangent->y = flStep - flT;
    pTangent->z = g_flZeroSpline;
    Math::Vec3Normalize(pTangent);
}

#ifdef HYPERX_V100
#include <math.h>

// FUNCTION: HYPERX100 0x004363c0
void SplineCurve::ComputeValueTangent(float flValue, Vec3* pTangent)
{
    float flStep = g_flSplineTangentStep + flValue;

    if ((m_flValueUpper <= flStep) || (flStep <= m_flValueLower))
    {
        flStep = flValue - g_flSplineTangentStep;
    }

    const float flAhead = m_pfnSpringEval(flStep, m_flA, m_flB, m_flC, m_flD);

    pTangent->z = flAhead - m_pfnSpringEval(flValue, m_flA, m_flB, m_flC, m_flD);
    pTangent->x = flStep - flValue;
    pTangent->y = g_flZeroSpline;
    Math::Vec3Normalize(pTangent);
}

// A near-vertical direction takes a slope of 1e8.
// FUNCTION: HYPERX100 0x004361a0
void SplineCurve::IntersectLine(const Vec3* pDir, const Vec3* pPoint, Vec3* pOut)
{
    float flSlope;

    if (fabs(pDir->x) > 1e-05)
    {
        flSlope = pDir->y / pDir->x;
    }
    else
    {
        flSlope = 1e8f;
    }

    const float flSlopeSq = flSlope * flSlope;

    pOut->z = m_flTag;

    const float flCross = (pPoint->x * flSlopeSq - pPoint->y * 2.0f * flSlope) * pPoint->x;
    const float flRatio = m_flB / m_flA;
    const float flTerm  = (pPoint->y * pPoint->y + flCross - m_flB) / m_flA;

    const float flX = (pPoint->x * flSlope - pPoint->y) * (float)sqrt((flSlopeSq - flTerm) * m_flB) / ((flRatio + flSlopeSq) * 2.0f) * flSlope * 4.0f;

    pOut->x = flX;
    pOut->y = (flX - pPoint->x) * flSlope + pPoint->y;
}

#endif
