#ifndef HYPERBLADE_SPLINECURVE_H
#define HYPERBLADE_SPLINECURVE_H

struct Vec3;

#include "common/compat.h"
#include "math/spline.h"

class SplineCurve
{
public:
    void ComputeTangent(float flT, Vec3* pTangent);

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
