#ifdef HYPERX_V100

#include "common/compat.h"
#include "slerpwalk.h"
#include "slerpseg.h"
#include <math.h>

// FUNCTION: HYPERX100 0x004a2c20
void SlerpWalk::StepBack()
{
    SlerpSeg* const pSeg = m_pSeg;

    m_flTime = m_flTime - m_flStep;

    const float flAngle      = m_pflFractions[(short)m_flTime] * pSeg->m_flTheta;
    const float flWeightTo   = (float)sin(flAngle);
    const float flWeightFrom = (float)sin(pSeg->m_flTheta - flAngle);

    m_qOut.w = pSeg->m_q0Div.w * flWeightFrom + pSeg->m_q1Div.w * flWeightTo;
    m_qOut.x = pSeg->m_q1Div.x * flWeightTo + pSeg->m_q0Div.x * flWeightFrom;
    m_qOut.y = pSeg->m_q1Div.y * flWeightTo + pSeg->m_q0Div.y * flWeightFrom;
    m_qOut.z = pSeg->m_q0Div.z * flWeightFrom + pSeg->m_q1Div.z * flWeightTo;
}

#endif
