#ifndef HYPERBLADE_SPLINE_H
#define HYPERBLADE_SPLINE_H

class SplineCurve;
struct Vec3;

typedef float(__cdecl* SplineEvalFn)(float flT, float, float, float, float);

// A ring of two curves and the walker that steps along it.
class Spline
{
public:
    static float __cdecl EvalUpperHalf(float flT, float flHeightSq, float flWidthSq, float, float);
    static float __cdecl EvalLowerHalf(float flT, float flHeightSq, float flWidthSq, float, float);
    static float __cdecl EvalSpring(float flT, float flAmplitude, float flFrequency, float flOffset, float flPhase);

public:
    Spline();

    ~Spline();

    Spline* InitClamped(float flRadiusX, float flRadiusY);
    Spline* InitSymmetric(float flRadiusX, float flRadiusY);
    Spline* InitSegment(float flRadiusX, float flRadiusY);
    Spline* InitSpringNode(float flAmplitude, float flFrequency, float flOffset, float flPhase);

    void UpdateTowardTarget(Vec3* pState, Vec3* pTarget);
    void UpdatePositionVelocity(Vec3* pPosition, Vec3* pVelocity);

#ifdef HYPERX_V100
    // Only the radii and the parameter range of both curves; nothing else is touched.
    void SetRadii(float flRadiusX, float flRadiusY);

    // UpdateTowardTarget along the value range, with the rate as a parameter.
    void StepTowardOnValue(Vec3* pState, Vec3* pTarget, float flRate);
#endif

    // The curve the walker is on.
    SplineCurve* m_pCurve; // 0x00
    // The second curve; the spring path never touches it.
    SplineCurve* m_pSecond; // 0x04
};

#endif
