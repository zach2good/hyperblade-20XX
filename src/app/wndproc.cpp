#include "common/compat.h"
#include "graphics/colorpalette.h"
#include "globals.h"
#include "engine/gameoptions.h"
#include "appglobals.h"
#include "input/keyboarddeviceobj.h"
#include "ui/widget.h"
#include "ui/screen.h"

#include "wndproc.h"
#include "app/menuids.h"
#include "gameapp.h"
#include "sound/audiomgr.h"
#include "graphics/renderer.h"
#include "graphics/renderoptions.h"
#include "graphics/display.h"
#include "graphics/gdi.h"
#include "ui/mainwindow.h"
#include "graphics/swrend.h"
#include "engine/gameengine.h"
#include "engine/game.h"
#include "app/window.h"
#include "ui/debugmenu.h"

namespace WndProc
{

// A key down when the focus leaves never sends WM_KEYUP here.
static unsigned char s_abKeyHeld[256];

static void NoteKey(UINT nMessage, WPARAM nKey)
{
    if (nKey >= 256)
    {
        return;
    }

    if (nMessage == WM_KEYDOWN)
    {
        s_abKeyHeld[nKey] = 1;
    }
    else if (nMessage == WM_KEYUP)
    {
        s_abKeyHeld[nKey] = 0;
    }
}

static void ReleaseHeldKeys()
{
    for (unsigned int nKey = 0; nKey < 256; nKey++)
    {
        if (s_abKeyHeld[nKey] == 0)
        {
            continue;
        }

        s_abKeyHeld[nKey] = 0;

        if (g_pInputDevice != 0)
        {
            g_pInputDevice->HandleKeyUp(nKey);
        }
    }
}

} // namespace WndProc

// The game's input, as the window delivers it. Keys are Win32 virtual-key codes, which the key
// bindings store; buttons are the MK_ flags of every button and modifier held, and points are client
// pixels. A match reads a button's flag word as a key, so MK_LBUTTON lands on VK_LBUTTON.

static void Press(UINT uMsg, unsigned int wParam, LPARAM lParam)
{
    const HWND hWnd = g_gameApp.m_hWnd;

    if (g_nSplashFrame != 0)
    {
        GameApp::NextSplash(hWnd);

        return;
    }

    // F2 opens the debug menu anywhere, before a match or a menu sees the key.
    if (uMsg == WM_KEYDOWN && wParam == VK_F2)
    {
        DebugMenu::Toggle();

        return;
    }

    // A key held when the focus leaves never sees its release here.
    WndProc::NoteKey(uMsg, wParam);

    if (g_gameApp.m_nEngineState == ESTATE_MATCH_LIVE)
    {
        if ((short)g_pGameEngine->m_wRunState == ENGINE_RUN_PLAYING)
        {
            // The key goes through a char, so VK_OEM_PLUS and VK_OEM_MINUS never reach their cases.
            if (GameApp::HandleMatchKeypress((int)(char)wParam) == 0 && g_pInputDevice->HandleKeyDown(wParam) == -1)
            {
                GameApp::HandleGlobalKeyEvent(hWnd, (int)wParam);
            }

            return;
        }
    }
    else if (g_pActiveScreen != NULL)
    {
        if (uMsg == WM_KEYDOWN)
        {
            if (wParam == VK_F1)
            {
                GameApp::ShowHelp(hWnd);
                return;
            }

            g_pActiveScreen->HandleKeyMessage(WM_KEYDOWN, wParam, (unsigned int)lParam);
        }
        else
        {
            g_pActiveScreen->OnMouseMessage(uMsg, wParam, (unsigned int)Window::ToMenuPoint(hWnd, lParam));
        }

        return;
    }

    if (wParam == 'Q' || wParam == VK_ESCAPE || wParam == VK_PAUSE)
    {
        GameApp::HandleGlobalKeyEvent(hWnd, (int)wParam);
    }
}

static void Release(UINT uMsg, unsigned int wParam, LPARAM lParam)
{
    WndProc::NoteKey(uMsg, wParam);

    if (g_gameApp.m_nEngineState != ESTATE_MATCH_LIVE)
    {
        if (uMsg != WM_KEYUP && g_pActiveScreen != NULL)
        {
            g_pActiveScreen->OnMouseMessage(uMsg, wParam, (unsigned int)Window::ToMenuPoint(g_gameApp.m_hWnd, lParam));
        }
    }
    else if ((short)g_pGameEngine->m_wRunState == ENGINE_RUN_PLAYING)
    {
        g_pInputDevice->HandleKeyUp(wParam);
    }
}

void WndProc::OnKeyDown(unsigned int nKey)
{
    Press(WM_KEYDOWN, nKey, 0);
}

void WndProc::OnKeyUp(unsigned int nKey)
{
    Release(WM_KEYUP, nKey, 0);
}

void WndProc::OnMouseButton(unsigned int nMessage, unsigned int nButtons, int nX, int nY)
{
    if (nMessage == WM_LBUTTONDOWN || nMessage == WM_RBUTTONDOWN)
    {
        Press(nMessage, nButtons, MAKELPARAM(nX, nY));
    }
    else
    {
        Release(nMessage, nButtons, MAKELPARAM(nX, nY));
    }
}

void WndProc::OnMouseMove(unsigned int nButtons, int nX, int nY)
{
    if (g_pActiveScreen != NULL && g_gameApp.m_nEngineState != ESTATE_MATCH_LIVE)
    {
        g_pActiveScreen->OnMouseMessage(WM_MOUSEMOVE, nButtons, (unsigned int)Window::ToMenuPoint(g_gameApp.m_hWnd, MAKELPARAM(nX, nY)));
    }
}

// FUNCTION: HYPERX 0x0044c330
LRESULT CALLBACK WndProc::MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_PAINT:
            WndProc::HandlePaint(hWnd);
            return 0;

        case WM_ERASEBKGND:
            return 1;

        case WM_ACTIVATE:
            if (g_pActiveScreen != NULL)
            {
                g_pActiveScreen->SetRepaintBackground(1);
            }
            // Falls through

        default:
            break;

        case WM_SIZE:
            SWRend::RefreshBltDestRect(hWnd, wParam, (short)LOWORD(lParam), (short)HIWORD(lParam));
            Window::OnSize(wParam, (short)LOWORD(lParam), (short)HIWORD(lParam));
            Window::OnSizeMenus(hWnd, wParam);
            return 0;

        case WM_CREATE:
            if (MainWindow::OnCreate(hWnd, lParam) != 0)
            {
                return 0;
            }

            return -1;

        case WM_DESTROY:

            if (g_bHelpOpen != 0)
            {
                WinHelpA(hWnd, NULL, HELP_QUIT, 0);
            }

            GameApp::Shutdown(hWnd);
            g_bShuttingDown = 1;
            return 0;

        case WM_ACTIVATEAPP:
            WndProc::OnActivate(hWnd, (unsigned int)wParam, lParam);
            return 0;

        case WM_COMMAND:
            MainWindow::OnCommand(hWnd, LOWORD(wParam), lParam, HIWORD(wParam));
            return 0;
    }

    return DefWindowProcA(hWnd, uMsg, wParam, lParam);
}

// FUNCTION: HYPERX 0x0044c910
int WndProc::HandlePaint(HWND hWnd)
{
    if (g_nSplashFrame != 0)
    {
        ValidateRgn(hWnd, NULL);
        Window::PresentMenu(hWnd);

        return 1;
    }

    if (g_pActiveScreen == NULL || g_gameApp.m_nEngineState == ESTATE_MATCH_LIVE)
    {
        ValidateRgn(hWnd, NULL);

        if (!g_bIsWindowCreated)
        {
            return 1;
        }

        if (g_gameApp.m_nEngineState == ESTATE_MATCH_LIVE && g_bCursorHidden != 0)
        {
            Game::RunFrame();
        }
    }
    else
    {
        g_pActiveScreen->PaintToWindow();
    }

    if (g_bIsWindowCreated && g_gameApp.m_nEngineState == ESTATE_MATCH_LIVE)
    {
        return 0;
    }

    return 1;
}

// FUNCTION: HYPERX 0x0044d190
int WndProc::OnActivate(HWND hWnd, unsigned int bActive, LPARAM)
{
    if (g_bShuttingDown == 0 && bActive != g_gameApp.m_bIsActive)
    {
        g_gameApp.m_bIsActive = (unsigned char)bActive;

        if ((unsigned char)bActive != 0)
        {
            SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
            SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

            if (g_bFullScreen != 0)
            {
                GDI::QueryDisplayCaps();

                if (g_gameApp.m_nEngineState != ESTATE_MATCH_LIVE)
                {
                    Display::ChangeColorDepth(2);
                }
                else
                {
                    Display::ChangeColorDepth(0);
                }
            }

            if (g_gameApp.m_hWndActiveDialog != 0)
            {
                SendMessageA(g_gameApp.m_hWndActiveDialog, WM_ERASEBKGND, 0, 0);
                return 0;
            }

            if (g_bSingleStepActive != 0)
            {
                g_gameApp.m_bCursorWasVisible = GameApp::SetCursorVisible(0);
                Game::RunFrame();
                return 0;
            }

            if (g_pGameEngine != 0)
            {
                if (g_gameApp.m_wRunStateOnDeactivate == ENGINE_RUN_PLAYING)
                {
                    g_pGameEngine->SetRunState(1, 0);
                }

                if (g_pGameEngine != 0 && g_pGameEngine->m_wRunState == ENGINE_RUN_PLAYING)
                {
                    g_gameApp.m_bCursorWasVisible = GameApp::SetCursorVisible(g_gameApp.WasCursorVisible());
                    return 0;
                }
            }

            GameEngine::TogglePause(hWnd, 0);
            SendMessageA(hWnd, WM_PAINT, 0, 0);
            GameEngine::TogglePause(hWnd, 0);
        }
        else
        {
            // The WM_KEYUP for a key held across the focus change goes to another window.
            WndProc::ReleaseHeldKeys();

            SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS);

            if (g_bSingleStepActive == 0 && g_pGameEngine != 0)
            {
                g_gameApp.m_wRunStateOnDeactivate = g_pGameEngine->m_wRunState;

                if (g_gameApp.m_wRunStateOnDeactivate == ENGINE_RUN_PLAYING)
                {
                    g_pGameEngine->SetRunState(0, 0);
                }
            }

            g_gameApp.m_bCursorWasVisible = GameApp::SetCursorVisible(1);

            if (g_bFullScreen != 0)
            {
                Display::ChangeColorDepth(1);
            }
        }
    }

    return 0;
}

// FUNCTION: HYPERX 0x004939b0
unsigned int WndProc::HandleRenderMenuCommand(HMENU hMenu, unsigned short nCommand)
{
    RenderOptions opt = Renderer::GetRenderOptions();

    if (opt.bTextured != 0)
    {
        switch (nCommand)
        {
            case ID_RENDER_PERSPECTIVE:
                g_bPerspectiveTexturing = (g_bPerspectiveTexturing == 0);

                unsigned int uPerspectiveCheck;

                if (g_bPerspectiveTexturing)
                {
                    uPerspectiveCheck = MF_CHECKED;
                }
                else
                {
                    uPerspectiveCheck = MF_UNCHECKED;
                }

                CheckMenuItem(hMenu, ID_RENDER_PERSPECTIVE, uPerspectiveCheck);

                return g_bPerspectiveTexturing;

            case ID_RENDER_BILINEAR_FILTER:
                g_bBilinearFilter = (g_bBilinearFilter == 0);

                unsigned int uFilterCheck;

                if (g_bBilinearFilter)
                {
                    uFilterCheck = MF_CHECKED;
                }
                else
                {
                    uFilterCheck = MF_UNCHECKED;
                }

                CheckMenuItem(hMenu, ID_RENDER_BILINEAR_FILTER, uFilterCheck);

                return g_bBilinearFilter;

            case ID_RENDER_MDM_TEXTURES:
                g_cMdmTexLoaded = (g_cMdmTexLoaded == 0);
                g_bLoadingMode  = 1;

                unsigned int uMdmCheck;

                if (g_cMdmTexLoaded)
                {
                    uMdmCheck = MF_CHECKED;
                }
                else
                {
                    uMdmCheck = MF_UNCHECKED;
                }

                CheckMenuItem(hMenu, ID_RENDER_MDM_TEXTURES, uMdmCheck);

                EnableMenuItem(hMenu, ID_RENDER_PERSPECTIVE, !g_cMdmTexLoaded);

                if (opt.bFiltered != 0)
                {
                    EnableMenuItem(hMenu, ID_RENDER_BILINEAR_FILTER, !g_cMdmTexLoaded);
                }

                return g_cMdmTexLoaded;
        }
    }

    return 0;
}
