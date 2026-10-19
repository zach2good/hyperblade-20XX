#include "common/compat.h"
#include "common/clock.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <cstdio>
#include <cstdlib>
#include <string>
#include "globals.h"
#include "app/frametimer.h"
#include "app/simulator.h"
#include "gameapp.h"
#include "window.h"
#include "common/staticinit.h"

float g_flUnusedWinMainA = g_flZeroWinMain;
float g_flUnusedWinMainB = g_flZeroWinMain;
float g_flUnusedWinMainC = g_flZeroWinMain;

// SDL_main supplies the platform's entry point and splits the command line.
// FUNCTION: HYPERX 0x0044c070
int main(int argc, char* argv[])
{
    auto arguments = std::string();

    for (int i = 1; i < argc; i++)
    {
        if (SDL_strcmp(argv[i], "-sim") == 0)
        {
            i += Simulator::TakeArguments(argc - i - 1, argv + i + 1);

            continue;
        }

        if (!arguments.empty())
        {
            arguments += ' ';
        }

        arguments += argv[i];
    }

    // A simulation runs windowed and silent.
    if (Simulator::Requested())
    {
        arguments += " -w -s";
    }

    g_bIsWindowCreated = GameApp::Initialize(arguments.c_str());

    // The game never runs its static destructors, ending in std::_Exit as GameApp::OnExit does.
    if (g_bIsWindowCreated && Simulator::Requested())
    {
        const auto nCode = Simulator::Run();

        std::fflush(stdout);
        std::_Exit(nCode);
    }

    if (g_bIsWindowCreated)
    {
        if (g_frameTimer.bFrameTimesOn)
        {
            g_frameTimer.nClockReads++;
            g_frameTimer.frameStart = Clock::Now();
        }

        while (Window::Pump())
        {
            if (GameApp::MainUpdate())
            {
                Window::WaitWhileInactive();
            }
        }
    }

    return 0;
}
