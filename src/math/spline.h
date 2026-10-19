#ifndef HYPERBLADE_SPLINE_H
#define HYPERBLADE_SPLINE_H

#include <memory>

class SplineCurve;
struct Vec3;

using SplineEvalFn = float (*)(float flT, float, float, float, float);

// A ring of two curves and the walker that steps along it.
class Spline
{
public:
    static float EvalUpperHalf(float flT, float flHeightSq, float flWidthSq, float, float);
    static float EvalLowerHalf(float flT, float flHeightSq, float flWidthSq, float, float);
    static float EvalSpring(float flT, float flAmplitude, float flFrequency, float flOffset, float flPhase);

public:
    Spline();

    ~Spline();

    Spline(const Spline&)            = delete;
    Spline& operator=(const Spline&) = delete;
    Spline(Spline&&)                 = delete;
    Spline& operator=(Spline&&)      = delete;

    Spline* InitClamped(float flRadiusX, float flRadiusY);
    Spline* InitSymmetric(float flRadiusX, float flRadiusY);
    Spline* InitSegment(float flRadiusX, float flRadiusY);
    Spline* InitSpringNode(float flAmplitude, float flFrequency, float flOffset, float flPhase);

    void UpdateTowardTarget(Vec3* pState, Vec3* pTarget);
    void UpdatePositionVelocity(Vec3* pPosition, Vec3* pVelocity);

    // The curve the walker is on.
    SplineCurve* m_pCurve; // 0x00
    // The second curve; the spring path never touches it.
    SplineCurve* m_pSecond; // 0x04

private:
    // The ring's two curves.
    std::unique_ptr<SplineCurve> m_pOwnedFirst;
    std::unique_ptr<SplineCurve> m_pOwnedSecond;
};

#endif
