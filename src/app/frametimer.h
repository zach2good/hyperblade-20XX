#ifndef HYPERBLADE_FRAMETIMER_H
#define HYPERBLADE_FRAMETIMER_H

#include "common/compat.h"
#include "globals.h"
#include "common/clock.h"

struct FrameTimer
{
    FrameTimer()
    : nFrames(0)
    , nClockReads(0)
    , nPerfSampleCount(0)
    , frameStart()
    , frameStartTicks()
    , dClockReadCost(0.0)
    , dSampleCost(0.0)
    , dFpsTextCost(0.0)
    , accRender()
    , accGameLogic()
    , bFrameTimesOn(1)
    , bFresh(1)
    {
    }

    ~FrameTimer() = default;

    unsigned int        nFrames;          // 0x00
    unsigned int        nClockReads;      // 0x04
    unsigned int        nPerfSampleCount; // 0x08
    Clock::Milliseconds frameStart;       // 0x0c
    Clock::Milliseconds frameStartTicks;  // 0x10
    unsigned int        nUnused;          // 0x14
    // What a clock read, a sample and the FPS text cost; the frame rate takes them out.
    double                        dClockReadCost; // 0x18
    double                        dSampleCost;    // 0x20
    double                        dFpsTextCost;   // 0x28
    Clock::FractionalMilliseconds accRender;      // 0x30
    Clock::FractionalMilliseconds accGameLogic;   // 0x38
    unsigned char                 bFrameTimesOn;  // 0x40
    unsigned char                 bFresh;         // 0x41

    // Starts a new report period and returns its start time; the benchmarks discard it.
    Clock::Milliseconds Restart()
    {
        nPerfSampleCount = 0;
        nClockReads      = 0;
        nFrames          = 0;
        frameStart       = {};
        accGameLogic     = {};
        accRender        = {};

        return Clock::Now();
    }

    double Calibrate();
    double BenchmarkResolution();
    void   ProcessInitialFrameUpdate();

    static double EndFrameUpdate(FrameTimer* timer);

    // Holds the frame rate at thirty a second.
    static void Pace();
};

#endif
