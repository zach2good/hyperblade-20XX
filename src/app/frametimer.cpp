#include "common/compat.h"
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
double __fastcall FrameTimer::EndFrameUpdate(FrameTimer* timer)
{
    const unsigned int dwStart = timer->dwFrameStartTime;
    double             dFrameRate;

    timer->nPerfSampleCount++;

    const unsigned int dwNow     = timeGetTime();
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

    if (g_bHWFlipAvailable != 0)
    {
        // STRING: HYPERX 0x004c1e68
        sprintf(g_szFpsText, "FPS %02d FLIP", g_gameApp.m_nFrameRate);
    }
    else
    {
        // STRING: HYPERX 0x004c1e58
        sprintf(g_szFpsText, "FPS %02d BLIT", g_gameApp.m_nFrameRate);
    }

    timer->dwFrameStartTime = timer->Restart();

    return dFrameRate;
}

// Passes an uninitialised period to timeEndPeriod when timeGetDevCaps fails.
// FUNCTION: HYPERX 0x00442a70
double FrameTimer::Calibrate(void)
{
    unsigned int uPeriod;
    unsigned int nTotal = 0;

    {
        TIMECAPS tc;

        if (timeGetDevCaps(&tc, sizeof(tc)) == TIMERR_NOERROR)
        {
            const unsigned int uClamped = min(max(tc.wPeriodMin, 1), tc.wPeriodMax);

            uPeriod = uClamped;

            timeBeginPeriod(uClamped);
        }
    }

    const unsigned int dwStart = timeGetTime();

    for (int nLoop = 0; nLoop < 10000; nLoop++)
    {
        nClockReads++;

        nTotal += timeGetTime();
    }

    const double dResult = ((double)timeGetTime() - dwStart) / nClockReads;

    timeEndPeriod(uPeriod);

    Restart();

    return dResult;
}

// FUNCTION: HYPERX 0x00442b50
double FrameTimer::BenchmarkResolution(void)
{
    unsigned int nTotal = 0;

    const unsigned int dwStart = timeGetTime();

    for (int nLoop = 0; nLoop < 10000; nLoop++)
    {
        nPerfSampleCount++;

        nTotal += (timeGetTime() - dwStart);
    }

    const double dResult = ((double)timeGetTime() - dwStart) / nPerfSampleCount;

    Restart();

    return dResult;
}

// FUNCTION: HYPERX 0x00442be0
double FrameTimer::BenchmarkTextSpeed(HDC hDC)
{
    // STRING: HYPERX 0x004c1e78
    g_nFpsTextLength = sprintf(g_szFpsText, "FPS %02d", (int)floor(30.5));

    const unsigned int dwStart = timeGetTime();

    for (int nLoop = 0; nLoop < 100; nLoop++)
    {
        ExtTextOutA(hDC, 10, 0, 0, 0, g_szFpsText, g_nFpsTextLength, 0);
    }

    const unsigned int dwEnd = timeGetTime();

    const double dResult = ((double)dwEnd - (double)dwStart) * 0.001;

    Restart();

    return dResult;
}

// FUNCTION: HYPERX 0x00442ca0
void FrameTimer::ProcessInitialFrameUpdate(HWND hWnd, HDC hDC)
{
    if (bFrameTimesOn == 0)
    {
        return;
    }

    dClockReadCost = Calibrate();
    dSampleCost    = BenchmarkResolution();
    dFpsTextCost   = BenchmarkTextSpeed(hDC);

    nClockReads++;

    dwFrameStartTime = timeGetTime();

    for (int nFrame = 0; nFrame < 2; nFrame++)
    {
        ValidateRgn(hWnd, 0);
        Game::RunFrame();
    }

    FrameTimer::EndFrameUpdate(this);

    bFresh        = 0;
    bFrameTimesOn = 0;
}

// Microseconds per frame, and how far behind the deadline may slip before it is reset.
constexpr auto FRAME_MICROS = 33333u;
constexpr auto RESYNC_MS    = 1000l;

void FrameTimer::Pace()
{
    static auto s_bStarted   = 0;
    static auto s_dwDeadline = DWORD{ 0 };
    static auto s_nCarry     = 0u;

    const auto dwNow = timeGetTime();

    if (s_bStarted == 0)
    {
        timeBeginPeriod(1);

        s_bStarted   = 1;
        s_dwDeadline = dwNow;
    }

    if (Window::InSizeMove() != 0)
    {
        s_dwDeadline = dwNow;

        return;
    }

    s_nCarry += FRAME_MICROS;

    const auto dwStep = s_nCarry / 1000;

    s_nCarry -= dwStep * 1000;
    s_dwDeadline += dwStep;

    auto lWait = (long)(s_dwDeadline - dwNow);

    if (lWait < -RESYNC_MS)
    {
        s_dwDeadline = dwNow;

        return;
    }

    while (lWait > 0)
    {
        Sleep((DWORD)lWait);
        lWait = (long)(s_dwDeadline - timeGetTime());
    }
}
