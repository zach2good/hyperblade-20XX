#ifndef HYPERBLADE_SLERPSEG_H
#define HYPERBLADE_SLERPSEG_H

#include "common/compat.h"
#include "math/quat.h"

class TransformObject;

// One rotation step between two transforms: Init reads both into quaternions, Eval walks it.
class SlerpSeg
{
public:
    SlerpSeg();
    ~SlerpSeg();

    void Init(TransformObject* pStartMat, TransformObject* pEndMat);
    bool Eval(float flAt);

#ifdef HYPERX_V100
    // Init from two quaternions, without the small-angle cut-off; flips pQ1 into the near hemisphere.
    void InitFromQuats(const Quat* pQ0, Quat* pQ1);

    // Init on the quaternions already in m_q0Div and m_q1Div, dividing them by sin(theta) in place.
    void InitInPlace();
#endif

    float GetTheta() const
    {
        return m_flTheta;
    }

    float m_flTheta;
    Quat  m_q0;
    Quat  m_q1;
    Quat  m_q0Div;
    Quat  m_q1Div;
};

// FUNCTION: HYPERX100 0x00439600
inline SlerpSeg::~SlerpSeg()
{
}

#endif
