// v1.0.0.0's walker over a SlerpSeg, timed through a lookup table. Nothing constructs one.

#ifndef HYPERBLADE_SLERPWALK_H
#define HYPERBLADE_SLERPWALK_H

#ifdef HYPERX_V100

#include "math/quat.h"

class SlerpSeg;

class SlerpWalk
{
public:
    // Steps the time back and writes the blend the table gives for it into m_qOut.
    void StepBack();

    SlerpSeg* m_pSeg;   // 0x00
    float     m_flTime; // 0x04
    float     m_flStep; // 0x08
    // The blend fraction for each whole time step.
    float* m_pflFractions; // 0x0c
    Quat   m_qOut;         // 0x10
};

#endif

#endif
