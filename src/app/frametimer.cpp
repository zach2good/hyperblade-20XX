#include "common/compat.h"
#include <SDL3/SDL.h>
#include <cstdint>
#include "globals.h"
#include "app/gameapp.h"
#include "appglobals.h"
#include "frametimer.h"
#include <math.h>
#include <stdio.h>
#include "engine/game.h"
#include "app/window.h"

FrameTimer g_frameTimer;

// Reports every five seconds. Every caller discards the result.
// FUNCTION: HYPERX 0x00442940
double FrameTimer::EndFrameUpdate(FrameTimer* timer)
{
    const unsigned int dwStart = timer->dwFrameStartTime;
    double             dFrameRate;

    timer->nPerfSampleCount++;

    const unsigned int dwNow     = (unsigned int)SDL_GetTicks();
    const unsigned int dwElapsed = dwNow - dwStart;

    if (timer->bFrameTimesOn == 0 || dwElapsed < 5000)
    {
        return 0.0;
    }

    timer->nFrames--;

    const double dFrames = timer->nFrames;

    dFrameRate = 1.0 / ((((dwElapsed - timer->nClockReads * timer->dClockReadCost) - timer->nPerfSampleCount * timer->dSampleCost) - dFrames * timer->dFpsTextCost) / dFrames * 0.001);

    g_gameApp.m_nFrameRate = (int)floor(dFrameRate + 0.5);

    memset(g_szFpsText, 0, sizeof(g_szFpsText));

    // STRING: HYPERX 0x004c1e58
    sprintf(g_szFpsText, "FPS %02d BLIT", g_gameApp.m_nFrameRate);

    timer->dwFrameStartTime = timer->Restart();

    return dFrameRate;
}

unsigned int FrameTimer::Now()
{
    return (unsigned int)SDL_GetTicks();
}

// FUNCTION: HYPERX 0x00442a70
double FrameTimer::Calibrate(void)
{
    unsigned int nTotal = 0;

    const unsigned int dwStart = (unsigned int)SDL_GetTicks();

    for (int nLoop = 0; nLoop < 10000; nLoop++)
    {
        nClockReads++;

        nTotal += (unsigned int)SDL_GetTicks();
    }

    const double dResult = ((double)(unsigned int)SDL_GetTicks() - dwStart) / nClockReads;

    Restart();

    return dResult;
}

// FUNCTION: HYPERX 0x00442b50
double FrameTimer::BenchmarkResolution(void)
{
    unsigned int nTotal = 0;

    const unsigned int dwStart = (unsigned int)SDL_GetTicks();

    for (int nLoop = 0; nLoop < 10000; nLoop++)
    {
        nPerfSampleCount++;

        nTotal += ((unsigned int)SDL_GetTicks() - dwStart);
    }

    const double dResult = ((double)(unsigned int)SDL_GetTicks() - dwStart) / nPerfSampleCount;

    Restart();

    return dResult;
}

// FUNCTION: HYPERX 0x00442ca0
void FrameTimer::ProcessInitialFrameUpdate(HWND hWnd, HDC)
{
    if (bFrameTimesOn == 0)
    {
        return;
    }

    dClockReadCost = Calibrate();
    dSampleCost    = BenchmarkResolution();

    // The original timed a hundred GDI draws of the label here; the label no longer goes through GDI.
    // STRING: HYPERX 0x004c1e78
    g_nFpsTextLength = sprintf(g_szFpsText, "FPS %02d", (int)floor(30.5));
    dFpsTextCost     = 0.0;

    nClockReads++;

    dwFrameStartTime = (unsigned int)SDL_GetTicks();

    for (int nFrame = 0; nFrame < 2; nFrame++)
    {
        ValidateRgn(hWnd, 0);
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
