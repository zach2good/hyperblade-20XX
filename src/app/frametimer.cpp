#include "common/compat.h"
#include <SDL3/SDL.h>
#include <chrono>
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
    double dFrameRate;

    timer->nPerfSampleCount++;

    const auto elapsed = Clock::Now() - timer->frameStart;

    if (timer->bFrameTimesOn == 0 || elapsed < std::chrono::seconds{ 5 })
    {
        return 0.0;
    }

    const auto nElapsed = Clock::FractionalMilliseconds(elapsed).count();

    timer->nFrames--;

    const double dFrames = timer->nFrames;

    dFrameRate = 1.0 / ((((nElapsed - timer->nClockReads * timer->dClockReadCost) - timer->nPerfSampleCount * timer->dSampleCost) - dFrames * timer->dFpsTextCost) / dFrames * 0.001);

    g_gameApp.m_nFrameRate = static_cast<int>(floor(static_cast<double>(dFrameRate + 0.5)));

    memset(g_szFpsText, 0, sizeof(g_szFpsText));

    // STRING: HYPERX 0x004c1e58
    sprintf(g_szFpsText, "FPS %02d BLIT", g_gameApp.m_nFrameRate);

    timer->frameStart = timer->Restart();

    return dFrameRate;
}

// FUNCTION: HYPERX 0x00442a70
double FrameTimer::Calibrate()
{
    const auto start = Clock::Now();

    for (int nLoop = 0; nLoop < 10000; nLoop++)
    {
        nClockReads++;

        Clock::Now();
    }

    const auto dResult = Clock::FractionalMilliseconds(Clock::Now() - start).count() / nClockReads;

    Restart();

    return dResult;
}

// FUNCTION: HYPERX 0x00442b50
double FrameTimer::BenchmarkResolution()
{
    const auto start = Clock::Now();

    for (int nLoop = 0; nLoop < 10000; nLoop++)
    {
        nPerfSampleCount++;

        Clock::Now();
    }

    const auto dResult = Clock::FractionalMilliseconds(Clock::Now() - start).count() / nPerfSampleCount;

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

    frameStart = Clock::Now();

    for (int nFrame = 0; nFrame < 2; nFrame++)
    {
        Game::RunFrame();
    }

    FrameTimer::EndFrameUpdate(this);

    bFresh        = 0;
    bFrameTimesOn = 0;
}

// One frame, and how far behind the deadline may slip before it is reset.
constexpr auto kFrame  = std::chrono::nanoseconds{ std::chrono::seconds{ 1 } } / 30;
constexpr auto kResync = std::chrono::seconds{ 1 };

void FrameTimer::Pace()
{
    static auto s_deadline = std::chrono::steady_clock::time_point{};

    const auto now = std::chrono::steady_clock::now();

    if (s_deadline == std::chrono::steady_clock::time_point{})
    {
        s_deadline = now;
    }

    s_deadline += kFrame;

    if (now > s_deadline + kResync)
    {
        s_deadline = now;

        return;
    }

    // SDL's precise delay spins out the last of the wait, which a plain sleep cannot do on Windows.
    if (s_deadline > now)
    {
        SDL_DelayPrecise(static_cast<Uint64>(std::chrono::duration_cast<std::chrono::nanoseconds>(s_deadline - now).count()));
    }
}
