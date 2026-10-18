#include "common/compat.h"
#include <SDL3/SDL.h>
#include <cstdint>
#include "globals.h"
#include "app/gameapp.h"
#include "appglobals.h"
#include "frametimer.h"
#include <cmath>
#include <cstdio>
#include "engine/game.h"
#include "app/window.h"

FrameTimer g_frameTimer;

// Reports every five seconds. Every caller discards the result.
// FUNCTION: HYPERX 0x00442940
double FrameTimer::EndFrameUpdate(FrameTimer* timer)
{
    const unsigned int nStart = timer->nFrameStartTime;
    double             dFrameRate;

    timer->nPerfSampleCount++;

    const auto         nNow     = static_cast<unsigned int>(SDL_GetTicks());
    const unsigned int nElapsed = nNow - nStart;

    if (timer->bFrameTimesOn == 0 || nElapsed < 5000)
    {
        return 0.0;
    }

    timer->nFrames--;

    const double dFrames = timer->nFrames;

    dFrameRate = 1.0 / ((((nElapsed - timer->nClockReads * timer->dClockReadCost) - timer->nPerfSampleCount * timer->dSampleCost) - dFrames * timer->dFpsTextCost) / dFrames * 0.001);

    g_gameApp.m_nFrameRate = static_cast<int>(floor(static_cast<double>(dFrameRate + 0.5)));

    memset(g_szFpsText, 0, sizeof(g_szFpsText));

    // STRING: HYPERX 0x004c1e58
    sprintf(g_szFpsText, "FPS %02d BLIT", g_gameApp.m_nFrameRate);

    timer->nFrameStartTime = timer->Restart();

    return dFrameRate;
}

unsigned int FrameTimer::Now()
{
    return static_cast<unsigned int>(SDL_GetTicks());
}

// FUNCTION: HYPERX 0x00442a70
double FrameTimer::Calibrate()
{
    const auto nStart = static_cast<unsigned int>(SDL_GetTicks());

    for (int nLoop = 0; nLoop < 10000; nLoop++)
    {
        nClockReads++;

        SDL_GetTicks();
    }

    const double dResult = (static_cast<double>(static_cast<unsigned int>(SDL_GetTicks())) - nStart) / nClockReads;

    Restart();

    return dResult;
}

// FUNCTION: HYPERX 0x00442b50
double FrameTimer::BenchmarkResolution()
{
    const auto nStart = static_cast<unsigned int>(SDL_GetTicks());

    for (int nLoop = 0; nLoop < 10000; nLoop++)
    {
        nPerfSampleCount++;

        SDL_GetTicks();
    }

    const double dResult = (static_cast<double>(static_cast<unsigned int>(SDL_GetTicks())) - nStart) / nPerfSampleCount;

    Restart();

    return dResult;
}

// FUNCTION: HYPERX 0x00442ca0
void FrameTimer::ProcessInitialFrameUpdate()
{
    if (bFrameTimesOn == 0)
    {
        return;
    }

    dClockReadCost = Calibrate();
    dSampleCost    = BenchmarkResolution();

    // The original timed a hundred GDI draws of the label here; the label no longer goes through GDI.
    // STRING: HYPERX 0x004c1e78
    g_nFpsTextLength = sprintf(g_szFpsText, "FPS %02d", static_cast<int>(floor(30.5)));
    dFpsTextCost     = 0.0;

    nClockReads++;

    nFrameStartTime = static_cast<unsigned int>(SDL_GetTicks());

    for (int nFrame = 0; nFrame < 2; nFrame++)
    {
        Game::RunFrame();
    }

    FrameTimer::EndFrameUpdate(this);

    bFresh        = 0;
    bFrameTimesOn = 0;
}

// Nanoseconds per frame, and how far behind the deadline may slip before it is reset.
constexpr auto FRAME_NS  = std::uint64_t{ 1000000000 / 30 };
constexpr auto RESYNC_NS = std::uint64_t{ 1000000000 };

void FrameTimer::Pace()
{
    static auto s_nDeadlineNS = std::uint64_t{ 0 };

    const auto nNowNS = SDL_GetTicksNS();

    if (s_nDeadlineNS == 0)
    {
        s_nDeadlineNS = nNowNS;
    }

    s_nDeadlineNS += FRAME_NS;

    if (nNowNS > s_nDeadlineNS + RESYNC_NS)
    {
        s_nDeadlineNS = nNowNS;

        return;
    }

    if (s_nDeadlineNS > nNowNS)
    {
        SDL_DelayPrecise(s_nDeadlineNS - nNowNS);
    }
}
