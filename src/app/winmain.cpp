#include "common/compat.h"
#include "globals.h"
#include "app/frametimer.h"
#include "gameapp.h"
#include "window.h"
#include "common/staticinit.h"

float g_flUnusedWinMainA = g_flZeroWinMain;
float g_flUnusedWinMainB = g_flZeroWinMain;
float g_flUnusedWinMainC = g_flZeroWinMain;

// FUNCTION: HYPERX 0x0044c070
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR, int nShowCmd)
{
    LPSTR      lpCmdLineArgs = GetCommandLineA();
    const HWND hWnd          = GameApp::Initialize(hInstance, hPrevInstance, nShowCmd, lpCmdLineArgs);

    g_bIsWindowCreated = (hWnd != 0);

    if (g_bIsWindowCreated)
    {
        if (g_frameTimer.bFrameTimesOn)
        {
            g_frameTimer.nClockReads++;
            g_frameTimer.dwFrameStartTime = timeGetTime();
        }

        while (Window::Pump())
        {
            if (GameApp::MainUpdate(g_gameApp.GetMainWindow()))
            {
                Window::WaitWhileInactive();
            }
        }
    }

    return 0;
}
