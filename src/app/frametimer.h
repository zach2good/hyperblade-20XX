#ifndef HYPERBLADE_FRAMETIMER_H
#define HYPERBLADE_FRAMETIMER_H

#include "common/compat.h"
#include "globals.h"

struct FrameTimer
{
    FrameTimer()
    : nFrames(0)
    , nClockReads(0)
    , nPerfSampleCount(0)
    , dwFrameStartTime(0)
    , dwFrameStartTicks(0)
    , dClockReadCost(0.0)
    , dSampleCost(0.0)
    , dFpsTextCost(0.0)
    , dAccRenderMs(0.0)
    , dAccGameLogicMs(0.0)
    , bFrameTimesOn(1)
    , bFresh(1)
    {
    }

    ~FrameTimer()
    {
    }

    unsigned int nFrames;           // 0x00
    unsigned int nClockReads;       // 0x04
    unsigned int nPerfSampleCount;  // 0x08
    unsigned int dwFrameStartTime;  // 0x0c
    unsigned int dwFrameStartTicks; // 0x10
    unsigned int nUnused;           // 0x14
    // What a clock read, a sample and the FPS text cost; the frame rate takes them out.
    double        dClockReadCost;  // 0x18
    double        dSampleCost;     // 0x20
    double        dFpsTextCost;    // 0x28
    double        dAccRenderMs;    // 0x30
    double        dAccGameLogicMs; // 0x38
    unsigned char bFrameTimesOn;   // 0x40
    unsigned char bFresh;          // 0x41

    // Starts a new report period and returns its start time; the benchmarks discard it.
    unsigned int Restart()
    {
        nPerfSampleCount = 0;
        nClockReads      = 0;
        nFrames          = 0;
        dwFrameStartTime = 0;
        dAccGameLogicMs  = 0.0;
        dAccRenderMs     = 0.0;

        return timeGetTime();
    }

    double Calibrate(void);
    double BenchmarkResolution(void);
    double BenchmarkTextSpeed(HDC hDC);
    void   ProcessInitialFrameUpdate(HWND hWnd, HDC hDC);

    static double __fastcall EndFrameUpdate(FrameTimer* timer);
};

#endif
