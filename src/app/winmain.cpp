#include "common/compat.h"
#include "common/clock.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <string>
#include "globals.h"
#include "app/frametimer.h"
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
        if (i > 1)
        {
            arguments += ' ';
        }

        arguments += argv[i];
    }

    g_bIsWindowCreated = GameApp::Initialize(arguments.c_str());

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
