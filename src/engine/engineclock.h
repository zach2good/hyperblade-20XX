#ifndef HYPERBLADE_ENGINECLOCK_H
#define HYPERBLADE_ENGINECLOCK_H

#include "common/compat.h"
#include "common/staticassert.h"

// The frame deltas the clock averages.
#define CLOCK_RING_LENGTH 5

// The engine clock: smoothed frame time, its ring, the tick, and the period and pause clocks.
struct EngineClock
{
    float GetInvDelta() const
    {
        return flInvDelta;
    }

    float GetDelta() const
    {
        return flDelta;
    }

    float GetDeltaSquared() const
    {
        return flDeltaSquared;
    }

    float        flInvDelta;                      // 0x00
    float        flPrevInvDelta;                  // 0x04
    float        flInvDeltaSquared;               // 0x08
    float        flPrevInvDeltaSquared;           // 0x0c
    float        flDelta;                         // 0x10
    float        flPrevDelta;                     // 0x14
    float        flDeltaSquared;                  // 0x18
    float        flPrevDeltaSquared;              // 0x1c
    float        aflDeltaRing[CLOCK_RING_LENGTH]; // 0x20
    unsigned int nTickMs;                         // 0x34
    unsigned int nPrevTickMs;                     // 0x38
    short        nRingIndex;                      // 0x3c
    short        nPeriod;                         // 0x3e
    unsigned int nPeriodStartMs;                  // 0x40
    unsigned int nPauseStartMs;                   // 0x44
    int          nPauseMark;                      // 0x48
};

STATIC_ASSERT(sizeof(EngineClock) == 0x4c);

#endif
