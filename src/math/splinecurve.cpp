// The tangent: two samples a tenth apart, stepping back where forward would leave the range.

#include "common/compat.h"
#include "ui/hudglobals.h"
#include "vector.h"
#include "spline.h"
#include "math/splinecurve.h"

// GLOBAL: HYPERX 0x004b40c8
extern const float g_flSplineTangentStep;

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

    const auto flAhead = m_pfnEval(flStep, m_flA, m_flB, m_flC, m_flD);

    pTangent->x = flAhead - m_pfnEval(flT, m_flA, m_flB, m_flC, m_flD);
    pTangent->y = flStep - flT;
    pTangent->z = g_flZeroSpline;
    Math::Vec3Normalize(pTangent);
}
