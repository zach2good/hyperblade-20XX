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

inline SlerpSeg::~SlerpSeg()
{
}

#endif
