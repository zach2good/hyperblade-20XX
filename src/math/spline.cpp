#include "common/compat.h"
#include "spline.h"
#include "ui/hudglobals.h"
#include "globals.h"
#include <memory>
#include <cmath>
#include "math/splinecurve.h"
#include "physics/collisionellipsoid.h"

// GLOBAL: HYPERX 0x004b40bc
extern const float g_flSplineReflectScale;

// The two curves point at each other through m_pNext, so the ring is closed before either fills.

// FUNCTION: HYPERX 0x00492e90
Spline::Spline()
: m_pOwnedFirst(std::make_unique<SplineCurve>())
, m_pOwnedSecond(std::make_unique<SplineCurve>())
{
    m_pSecond = m_pOwnedSecond.get();
    m_pCurve  = m_pOwnedFirst.get();

    m_pCurve->m_pNext  = m_pSecond;
    m_pSecond->m_pNext = m_pCurve;
}

// FUNCTION: HYPERX 0x00492ec0
Spline::~Spline() = default;

// Both curves take the upper-half evaluator; the tag is -35 past a second radius of three, else -1.

// FUNCTION: HYPERX 0x00492f10
Spline* Spline::InitClamped(float flRadiusX, float flRadiusY)
{
    m_pCurve->m_pfnEval  = Spline::EvalUpperHalf;
    m_pSecond->m_pfnEval = Spline::EvalUpperHalf;

    if (flRadiusY > 3.0f)
    {
        m_pCurve->m_flTag  = -35.0f;
        m_pSecond->m_flTag = -35.0f;
    }
    else
    {
        m_pCurve->m_flTag  = -1.0f;
        m_pSecond->m_flTag = -1.0f;
    }

    auto* pHalf  = m_pCurve;
    pHalf->m_flA = flRadiusX * flRadiusX;
    pHalf->m_flB = flRadiusY * flRadiusY;
    pHalf->m_flC = g_flZeroSpline;
    pHalf->m_flD = g_flZeroSpline;

    pHalf        = m_pSecond;
    pHalf->m_flA = flRadiusX * flRadiusX;
    pHalf->m_flB = flRadiusY * flRadiusY;
    pHalf->m_flC = g_flZeroSpline;
    pHalf->m_flD = g_flZeroSpline;

    pHalf            = m_pCurve;
    pHalf->m_flLower = -flRadiusY;
    pHalf->m_flUpper = flRadiusY;

    pHalf            = m_pSecond;
    pHalf->m_flLower = -flRadiusY;
    pHalf->m_flUpper = flRadiusY;

    pHalf                 = m_pCurve;
    auto flZero           = g_flZeroSpline;
    pHalf->m_flValueLower = flZero;
    pHalf->m_flValueUpper = flRadiusX;

    pHalf                 = m_pSecond;
    flZero                = g_flZeroSpline;
    pHalf->m_flValueUpper = flRadiusX;
    pHalf->m_flValueLower = flZero;

    return this;
}

// Half an ellipse, reached only through m_pfnEval: the first radius times sqrt(1 - t*t/B).

// FUNCTION: HYPERX 0x00492fe0
float Spline::EvalUpperHalf(float flT, float flHeightSq, float flWidthSq, float, float)
{
    return static_cast<float>(sqrt(static_cast<double>((flHeightSq * flWidthSq - flT * flT * flHeightSq) / flWidthSq)));
}

// FUNCTION: HYPERX 0x00493000
Spline* Spline::InitSymmetric(float flRadiusX, float flRadiusY)
{
    m_pCurve->m_pfnEval  = Spline::EvalLowerHalf;
    m_pSecond->m_pfnEval = Spline::EvalLowerHalf;

    auto flZero        = g_flZeroSpline;
    m_pCurve->m_flTag  = flZero;
    flZero             = g_flZeroSpline;
    m_pSecond->m_flTag = flZero;

    auto* pHalf  = m_pCurve;
    pHalf->m_flA = flRadiusX * flRadiusX;
    pHalf->m_flB = flRadiusY * flRadiusY;
    pHalf->m_flC = g_flZeroSpline;
    pHalf->m_flD = g_flZeroSpline;

    pHalf        = m_pSecond;
    pHalf->m_flA = flRadiusX * flRadiusX;
    pHalf->m_flB = flRadiusY * flRadiusY;
    pHalf->m_flC = g_flZeroSpline;
    pHalf->m_flD = g_flZeroSpline;

    pHalf            = m_pCurve;
    pHalf->m_flLower = -flRadiusY;
    pHalf->m_flUpper = flRadiusY;

    pHalf            = m_pSecond;
    pHalf->m_flLower = -flRadiusY;
    pHalf->m_flUpper = flRadiusY;

    pHalf                 = m_pCurve;
    flZero                = g_flZeroSpline;
    pHalf->m_flValueLower = flZero;
    pHalf->m_flValueUpper = flRadiusX;

    pHalf                 = m_pSecond;
    flZero                = g_flZeroSpline;
    pHalf->m_flValueUpper = flRadiusX;
    pHalf->m_flValueLower = flZero;

    return this;
}

// FUNCTION: HYPERX 0x004930c0
float Spline::EvalLowerHalf(float flT, float flHeightSq, float flWidthSq, float, float)
{
    return -static_cast<float>(sqrt(static_cast<double>((flHeightSq * flWidthSq - flT * flT * flHeightSq) / flWidthSq)));
}

// FUNCTION: HYPERX 0x004930e0
Spline* Spline::InitSegment(float flRadiusX, float flRadiusY)
{
    m_pCurve->m_pfnEval  = Spline::EvalUpperHalf;
    m_pSecond->m_pfnEval = Spline::EvalLowerHalf;

    auto flZero        = g_flZeroSpline;
    m_pCurve->m_flTag  = flZero;
    flZero             = g_flZeroSpline;
    m_pSecond->m_flTag = flZero;

    auto* pHalf  = m_pCurve;
    pHalf->m_flA = flRadiusX * flRadiusX;
    pHalf->m_flB = flRadiusY * flRadiusY;
    pHalf->m_flC = g_flZeroSpline;
    pHalf->m_flD = g_flZeroSpline;

    pHalf        = m_pSecond;
    pHalf->m_flA = flRadiusX * flRadiusX;
    pHalf->m_flB = flRadiusY * flRadiusY;
    pHalf->m_flC = g_flZeroSpline;
    pHalf->m_flD = g_flZeroSpline;

    pHalf            = m_pCurve;
    pHalf->m_flLower = -flRadiusY;
    pHalf->m_flUpper = flRadiusY;

    pHalf            = m_pSecond;
    pHalf->m_flLower = -flRadiusY;
    pHalf->m_flUpper = flRadiusY;

    pHalf                 = m_pCurve;
    flZero                = g_flZeroSpline;
    pHalf->m_flValueLower = flZero;
    pHalf->m_flValueUpper = flRadiusX;

    pHalf                 = m_pSecond;
    flZero                = g_flZeroSpline;
    pHalf->m_flValueUpper = flZero;
    pHalf->m_flValueLower = -flRadiusX;

    return this;
}

// The only initialiser that clears m_pfnEval; it stores the cosine evaluator in m_pfnSpringEval.

// FUNCTION: HYPERX 0x004931a0
Spline* Spline::InitSpringNode(float flAmplitude, float flFrequency, float flOffset, float flPhase)
{
    m_pCurve->m_pfnEval       = nullptr;
    m_pCurve->m_pfnSpringEval = Spline::EvalSpring;

    auto* pNode    = m_pCurve;
    pNode->m_flA   = flAmplitude;
    pNode->m_flB   = flFrequency;
    pNode->m_flC   = flOffset;
    pNode->m_flD   = flPhase;
    pNode->m_flTag = g_flZeroSpline;

    pNode            = m_pCurve;
    pNode->m_flUpper = flAmplitude + flOffset;
    pNode->m_flLower = flOffset - flAmplitude;

    const auto flLower = g_arenaEllipsoid.GetRadiusX();
    const auto flUpper = g_arenaEllipsoid.GetRadiusX();

    // Upper is below lower, so UpdatePositionVelocity's exit test passes on every step.
    pNode                 = m_pCurve;
    pNode->m_flValueUpper = flUpper * -1.5f;
    pNode->m_flValueLower = flLower * 1.5f;

    return this;
}

// FUNCTION: HYPERX 0x00493220
float Spline::EvalSpring(float flT, float flAmplitude, float flFrequency, float flOffset, float flPhase)
{
    return static_cast<float>(cos(static_cast<double>(flT * flFrequency + flPhase))) * flAmplitude + flOffset;
}

// A parameter that leaves the curve moves to the next one, reflected about its nearer limit.

// FUNCTION: HYPERX 0x00493240
void Spline::UpdateTowardTarget(Vec3* pState, Vec3* pTarget)
{
    Vec3 vTangent;

    m_pCurve->ComputeTangent(pState->y, &vTangent);

    Vec3 vDiff;

    vDiff.x = pTarget->x - pState->x;
    vDiff.y = pTarget->y - pState->y;
    vDiff.z = pTarget->z - pState->z;

    const auto flParam = (vTangent.x * vDiff.x + vTangent.y * vDiff.y + vTangent.z * vDiff.z) * vTangent.y * 0.1f + pState->y;

    pState->y = flParam;

    auto* pCurve = m_pCurve;

    if ((flParam >= pCurve->m_flUpper) || (flParam <= pCurve->m_flLower))
    {
        pCurve   = pCurve->m_pNext;
        m_pCurve = pCurve;

        const auto flT = pState->y;

        float flClamped;

        if (flT > pCurve->m_flUpper)
        {
            flClamped = pCurve->m_flUpper;
        }
        else if (flT < pCurve->m_flLower)
        {
            flClamped = pCurve->m_flLower;
        }
        else
        {
            flClamped = flT;
        }

        auto flReflected = flClamped;

        flReflected *= g_flSplineReflectScale;

        pState->y = flReflected - flT;
    }

    auto* const pEval = m_pCurve;

    pState->x = pEval->m_pfnEval(pState->y, pEval->m_flA, pEval->m_flB, pEval->m_flC, pEval->m_flD);
    pState->z = pEval->m_flTag;
}

// UpdateTowardTarget's twin, stepping curves on the value range and with no tenth in its step.

// FUNCTION: HYPERX 0x00493320
void Spline::UpdatePositionVelocity(Vec3* pPosition, Vec3* pVelocity)
{
    auto* const pCurve = m_pCurve;

    const auto flValue = pPosition->x;

    if ((flValue >= pCurve->m_flValueUpper) || (flValue <= pCurve->m_flValueLower))
    {
        m_pCurve = pCurve->m_pNext;
    }

    auto* const pEval = m_pCurve;

    pPosition->x = pEval->m_pfnEval(pPosition->y, pEval->m_flA, pEval->m_flB, pEval->m_flC, pEval->m_flD);
    pPosition->z = pEval->m_flTag;

    Vec3 vTangent;

    m_pCurve->ComputeTangent(pPosition->y, &vTangent);

    const auto flParam = (vTangent.x * pVelocity->x + vTangent.y * pVelocity->y + vTangent.z * pVelocity->z) * vTangent.y + pPosition->y;

    pPosition->y = flParam;

    auto* pStep = m_pCurve;

    if ((flParam >= pStep->m_flUpper) || (flParam <= pStep->m_flLower))
    {
        pStep    = pStep->m_pNext;
        m_pCurve = pStep;

        const auto flT = pPosition->y;

        float flClamped;

        if (flT > pStep->m_flUpper)
        {
            flClamped = pStep->m_flUpper;
        }
        else if (flT < pStep->m_flLower)
        {
            flClamped = pStep->m_flLower;
        }
        else
        {
            flClamped = flT;
        }

        auto flReflected = flClamped;

        flReflected *= g_flSplineReflectScale;

        pPosition->y = flReflected - flT;
    }

    auto* const pFinal = m_pCurve;

    pPosition->x = pFinal->m_pfnEval(pPosition->y, pFinal->m_flA, pFinal->m_flB, pFinal->m_flC, pFinal->m_flD);
    pPosition->z = pFinal->m_flTag;
}
