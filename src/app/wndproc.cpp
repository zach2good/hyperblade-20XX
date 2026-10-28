#include "common/compat.h"
#include "engine/spectator.h"
#include "input/keycodes.h"
#include "input/inputevent.h"
#include <SDL3/SDL.h>
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

// A key down when the focus leaves never sends kInputKeyUp here.
static unsigned char s_abKeyHeld[256];

static void NoteKey(unsigned int nMessage, unsigned int nKey)
{
    if (nKey >= 256)
    {
        return;
    }

    if (nMessage == kInputKeyDown)
    {
        s_abKeyHeld[nKey] = 1;
    }
    else if (nMessage == kInputKeyUp)
    {
        s_abKeyHeld[nKey] = 0;
    }
}

void ReleaseHeldKeys()
{
    for (unsigned int nKey = 0; nKey < 256; nKey++)
    {
        if (s_abKeyHeld[nKey] == 0)
        {
            continue;
        }

        s_abKeyHeld[nKey] = 0;

        if (g_pInputDevice != nullptr)
        {
            g_pInputDevice->HandleKeyUp(nKey);
        }
    }
}

} // namespace WndProc

// The game's input, as the window delivers it. Keys are Win32 virtual-key codes, which the key
// bindings store; buttons are the MOUSE_ flags of every button and modifier held, and points are
// client pixels. A match reads a button's flag word as a key, so kMouseLeft lands on Key::kLButton.

static void Press(unsigned int nMessage, unsigned int nCode, int nX, int nY)
{
    if (g_nSplashFrame != 0)
    {
        GameApp::NextSplash();

        return;
    }

    // kF2 opens the debug menu anywhere, before a match or a menu sees the key.
    if (nMessage == kInputKeyDown && nCode == Key::kF2)
    {
        DebugMenu::Toggle();

        return;
    }

    // kF3 flies the camera free in a match.
    if (nMessage == kInputKeyDown && nCode == Key::kF3 && g_gameApp.m_nEngineState == kEstateMatchLive)
    {
        Spectator::SetFreeCamera(!Spectator::FreeCamera());

        return;
    }

    // A key held when the focus leaves never sees its release here.
    WndProc::NoteKey(nMessage, nCode);

    if (g_gameApp.m_nEngineState == kEstateMatchLive)
    {
        if (static_cast<short>(g_pGameEngine->m_wRunState) == kEngineRunPlaying)
        {
            // The key goes through a char, so Key::kOEMPlus and Key::kOEMMinus never reach their cases.
            if (GameApp::HandleMatchKeypress(static_cast<int>(static_cast<char>(nCode))) == 0 && (Spectator::HoldsInput() || g_pInputDevice->HandleKeyDown(nCode) == -1))
            {
                GameApp::HandleGlobalKeyEvent(static_cast<int>(nCode));
            }

            return;
        }
    }
    else if (g_pActiveScreen != nullptr)
    {
        if (nMessage == kInputKeyDown)
        {
            // kF1 opened HELP.HLP, which Windows no longer reads.
            if (nCode == Key::kF1)
            {
                return;
            }

            g_pActiveScreen->HandleKeyMessage(kInputKeyDown, nCode, 0);
        }
        else
        {
            const auto pt = Window::ToMenuPoint(nX, nY);

            g_pActiveScreen->OnMouseMessage(nMessage, nCode, pt.x, pt.y);
        }

        return;
    }

    // Return answers the quit prompt, which pauses the match.
    if (nCode == 'Q' || nCode == Key::kEscape || nCode == Key::kPause || nCode == Key::kReturn)
    {
        GameApp::HandleGlobalKeyEvent(static_cast<int>(nCode));
    }
}

static void Release(unsigned int nMessage, unsigned int nCode, int nX, int nY)
{
    WndProc::NoteKey(nMessage, nCode);

    if (g_gameApp.m_nEngineState != kEstateMatchLive)
    {
        if (nMessage != kInputKeyUp && g_pActiveScreen != nullptr)
        {
            const auto pt = Window::ToMenuPoint(nX, nY);

            g_pActiveScreen->OnMouseMessage(nMessage, nCode, pt.x, pt.y);
        }
    }
    else if (static_cast<short>(g_pGameEngine->m_wRunState) == kEngineRunPlaying && !Spectator::HoldsInput())
    {
        g_pInputDevice->HandleKeyUp(nCode);
    }
}

void WndProc::OnKeyDown(unsigned int nKey)
{
    Press(kInputKeyDown, nKey, 0, 0);
}

void WndProc::OnKeyUp(unsigned int nKey)
{
    Release(kInputKeyUp, nKey, 0, 0);
}

void WndProc::OnMouseButton(unsigned int nMessage, unsigned int nButtons, int nX, int nY)
{
    if (nMessage == kInputLeftDown || nMessage == kInputRightDown)
    {
        Press(nMessage, nButtons, nX, nY);
    }
    else
    {
        Release(nMessage, nButtons, nX, nY);
    }
}

void WndProc::OnMouseMove(unsigned int nButtons, int nX, int nY)
{
    if (g_pActiveScreen != nullptr && g_gameApp.m_nEngineState != kEstateMatchLive)
    {
        const auto pt = Window::ToMenuPoint(nX, nY);

        g_pActiveScreen->OnMouseMessage(kInputMouseMove, nButtons, pt.x, pt.y);
    }
}

// The window procedure's WM_SIZE.
// FUNCTION: HYPERX 0x0044c330
void WndProc::OnResize(int cx, int cy)
{
    Window::OnSize(cx, cy);
    Window::OnSizeMenus();
}

// The window procedure's WM_DESTROY.
void WndProc::OnDestroy()
{
    GameApp::Shutdown();
    g_bShuttingDown = 1;
}

// FUNCTION: HYPERX 0x0044c910
int WndProc::HandlePaint()
{
    if (g_nSplashFrame != 0)
    {
        Window::PresentMenu();

        return 1;
    }

    if (g_pActiveScreen == nullptr || g_gameApp.m_nEngineState == kEstateMatchLive)
    {
        if (!g_bIsWindowCreated)
        {
            return 1;
        }

        if (g_gameApp.m_nEngineState == kEstateMatchLive && g_bCursorHidden != 0)
        {
            Game::RunFrame();
        }
    }
    else
    {
        g_pActiveScreen->PaintToWindow();
    }

    if (g_bIsWindowCreated && g_gameApp.m_nEngineState == kEstateMatchLive)
    {
        return 0;
    }

    return 1;
}

// FUNCTION: HYPERX 0x0044d190
int WndProc::OnActivate(unsigned int bActive)
{
    if (g_bShuttingDown == 0 && bActive != g_gameApp.m_bIsActive)
    {
        g_gameApp.m_bIsActive = static_cast<unsigned char>(bActive);

        if (static_cast<unsigned char>(bActive) != 0)
        {
            SDL_SetCurrentThreadPriority(SDL_THREAD_PRIORITY_HIGH);

            if (g_bFullScreen != 0)
            {
                if (g_gameApp.m_nEngineState != kEstateMatchLive)
                {
                    Display::ChangeColorDepth(2);
                }
                else
                {
                    Display::ChangeColorDepth(0);
                }
            }

            if (g_bSingleStepActive != 0)
            {
                g_gameApp.m_bCursorWasVisible = GameApp::SetCursorVisible(0);
                Game::RunFrame();
                return 0;
            }

            if (g_pGameEngine != nullptr)
            {
                if (g_gameApp.m_wRunStateOnDeactivate == kEngineRunPlaying)
                {
                    g_pGameEngine->SetRunState(1, 0);
                }

                if (g_pGameEngine != nullptr && g_pGameEngine->m_wRunState == kEngineRunPlaying)
                {
                    g_gameApp.m_bCursorWasVisible = GameApp::SetCursorVisible(g_gameApp.WasCursorVisible());
                    return 0;
                }
            }

            GameEngine::TogglePause(0);
            WndProc::HandlePaint();
            GameEngine::TogglePause(0);
        }
        else
        {
            // The kInputKeyUp for a key held across the focus change goes to another window.
            WndProc::ReleaseHeldKeys();

            SDL_SetCurrentThreadPriority(SDL_THREAD_PRIORITY_LOW);

            if (g_bSingleStepActive == 0 && g_pGameEngine != nullptr)
            {
                g_gameApp.m_wRunStateOnDeactivate = g_pGameEngine->m_wRunState;

                if (g_gameApp.m_wRunStateOnDeactivate == kEngineRunPlaying)
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
unsigned int WndProc::HandleRenderMenuCommand(unsigned short nCommand)
{
    const auto opt = Renderer::GetRenderOptions();

    if (opt.bTextured != 0)
    {
        switch (nCommand)
        {
            case kIDRenderPerspective:
                g_bPerspectiveTexturing = (g_bPerspectiveTexturing == 0);

                return g_bPerspectiveTexturing;

            case kIDRenderBilinearFilter:
                g_bBilinearFilter = (g_bBilinearFilter == 0);

                return g_bBilinearFilter;

            case kIDRenderMDMTextures:
                g_cMdmTexLoaded = (g_cMdmTexLoaded == 0);
                g_bLoadingMode  = 1;

                return g_cMdmTexLoaded;
        }
    }

    return 0;
}
