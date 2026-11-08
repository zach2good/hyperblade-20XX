#ifndef HYPERBLADE_SPLINECURVE_H
#define HYPERBLADE_SPLINECURVE_H

struct Vec3;

#include "common/compat.h"
#include "math/spline.h"

class SplineCurve
{
public:
    void ComputeTangent(float flT, Vec3* pTangent);

#ifdef HYPERX_V100
    // ComputeTangent along the value range, through the second evaluator, with x and z swapped.
    void ComputeValueTangent(float flValue, Vec3* pTangent);

    // Where the line through pPoint along pDir meets the curve, as the original works it out.
    void IntersectLine(const Vec3* pDir, const Vec3* pPoint, Vec3* pOut);
#endif

    SplineEvalFn m_pfnEval; // 0x00
    // Spline::InitSpringNode stores its evaluator here and clears m_pfnEval.
    SplineEvalFn m_pfnSpringEval; // 0x04
    float        m_flA;           // 0x08
    float        m_flB;           // 0x0c
    float        m_flC;           // 0x10
    float        m_flD;           // 0x14
    float        m_flTag;         // 0x18
    float        m_flUpper;       // 0x1c
    float        m_flLower;       // 0x20
    float        m_flValueUpper;  // 0x24
    float        m_flValueLower;  // 0x28
    SplineCurve* m_pNext;         // 0x2c
};

#endif
