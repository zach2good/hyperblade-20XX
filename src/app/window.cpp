#include "common/compat.h"
#include "input/keycodes.h"
#include "input/inputevent.h"
#include "globals.h"
#include "gameapp.h"
#include "app/window.h"
#include <vector>
#include "graphics/presenter.h"
#include "wndproc.h"
#include "engine/gameengine.h"
#include "engine/objectclasses.h"
#include "engine/gameoptions.h"
#include "graphics/graphicsglobals.h"
#include "graphics/softrenderer.h"
#include "ui/mainwindow.h"
#include "app/appglobals.h"
#include "ui/screen.h"
#include "ui/debugmenu.h"
#include "input/inputlog.h"
#include "input/keymap.h"
#include <SDL3/SDL.h>
#include <array>
#include <utility>
#include <span>
#include <algorithm>

// The game's one window, which SDL made and owns.
static SDL_Window* s_pWindow = 0;

// FUNCTION: HYPERX 0x0044bd60
void Window::ApplyFullscreenStyle()
{
    // SDL made the window framed and resizable. Every pause toggle comes here, and only the first
    // sets the size, so the window keeps the one the player gave it.
    static auto s_bSized = false;

    if (s_bSized)
    {
        return;
    }

    s_bSized = true;

    constexpr auto START_WIDTH  = 1920;
    constexpr auto START_HEIGHT = 1080;

    SDL_SetWindowSize(s_pWindow, START_WIDTH, START_HEIGHT);
    SDL_SetWindowPosition(s_pWindow, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
}

// FUNCTION: HYPERX 0x0048f450
void Window::InvalidateAndUpdate(void)
{
    WndProc::HandlePaint();
}

// The menus and splashes paint at 640x480. They draw into this canvas, which is scaled to the
// window keeping its shape, and the mouse is mapped back into it.
constexpr auto MENU_WIDTH  = 640;
constexpr auto MENU_HEIGHT = 480;

static std::vector<std::uint32_t> s_menuPixels((size_t)(MENU_WIDTH * MENU_HEIGHT));

Window::MenuCanvas Window::MenuPixels()
{
    return MenuCanvas{ s_menuPixels.data(), MENU_WIDTH, MENU_HEIGHT };
}

// The canvas's place in the client area: as large as fits, centred.
static SDL_Rect MenuRect()
{
    auto nClientWidth  = 0;
    auto nClientHeight = 0;

    SDL_GetWindowSizeInPixels(s_pWindow, &nClientWidth, &nClientHeight);

    auto cx = nClientWidth;
    auto cy = cx * MENU_HEIGHT / MENU_WIDTH;

    if (cy > nClientHeight)
    {
        cy = nClientHeight;
        cx = cy * MENU_WIDTH / MENU_HEIGHT;
    }

    return SDL_Rect{ (nClientWidth - cx) / 2, (nClientHeight - cy) / 2, cx, cy };
}

// SDL fits the canvas the same way MenuRect does, so the mouse mapping agrees with the picture.
void Window::PresentMenu()
{
    Presenter::Canvas(s_menuPixels.data(), MENU_WIDTH * 4, MENU_WIDTH, MENU_HEIGHT);
}

Point Window::ToMenuPoint(int nX, int nY)
{
    const auto rc = MenuRect();

    if (rc.w <= 0 || rc.h <= 0)
    {
        return Point{ nX, nY };
    }

    return Point{ (nX - rc.x) * MENU_WIDTH / rc.w, (nY - rc.y) * MENU_HEIGHT / rc.h };
}

// The 3D renders at the client size, so it is never stretched. The width stays a multiple of four,
// which the back buffer needs, and PresentFrame centres what that leaves.
void Window::OnSize(int cx, int cy)
{
    if (g_pBackBufferBitsCopy == 0)
    {
        return;
    }

    auto nWidth  = std::clamp(cx, 320, 4096) & ~3;
    auto nHeight = std::clamp(cy, 200, 2160);

    if (nWidth == g_nBackBufferWidth && nHeight == g_nBackBufferHeight)
    {
        return;
    }

    // The resize lays out the main camera, which an engine still loading has not made yet.
    if (g_pGameEngine != 0 && (g_pGameEngine->m_ppSlots == 0 || g_pGameEngine->GetCamera(CAMERA_MAIN_SLOT) == 0))
    {
        return;
    }

    g_dwDisplayWidth  = (unsigned int)nWidth;
    g_dwDisplayHeight = (unsigned int)nHeight;

    // ResizeBackBuffer compares against the camera's viewport, which can be smaller than the buffer.
    g_nPrevWidth  = 0;
    g_nPrevHeight = 0;

    SoftRenderer::ResizeBackBuffer(&nWidth, &nHeight);

    if (g_pGameEngine == 0)
    {
        return;
    }

    GameApp::SetupViewports();

    // SetScreenSize skips an unchanged step, and SetupViewports has just undone its margins.
    g_pGameEngine->m_nScreenSize = -1;
    g_pGameEngine->SetScreenSize(g_pGameOptions->ReadScreenSize());
}

// Menus redraw from the canvas at the new size; a match redraws itself next frame.
void Window::OnSizeMenus()
{
    if (g_gameApp.m_nEngineState != ESTATE_MATCH_LIVE)
    {
        Window::PresentMenu();
    }
}

static bool s_bQuit         = false;
static bool s_bInLiveUpdate = false;

// The MOUSE_ flags of a mouse event: the buttons held once it is handled, with Shift and Ctrl.
static unsigned int MouseFlags(SDL_MouseButtonFlags buttons)
{
    constexpr std::pair<SDL_MouseButtonFlags, unsigned int> BUTTONS[] = {
        { SDL_BUTTON_LMASK, MOUSE_LEFT },
        { SDL_BUTTON_RMASK, MOUSE_RIGHT },
        { SDL_BUTTON_MMASK, MOUSE_MIDDLE },
        { SDL_BUTTON_X1MASK, MOUSE_X1 },
        { SDL_BUTTON_X2MASK, MOUSE_X2 },
    };

    auto nFlags = 0u;

    for (const auto& [mask, nFlag] : BUTTONS)
    {
        if ((buttons & mask) != 0)
        {
            nFlags |= nFlag;
        }
    }

    const auto mod = SDL_GetModState();

    if ((mod & SDL_KMOD_SHIFT) != 0)
    {
        nFlags |= MOUSE_SHIFT;
    }

    if ((mod & SDL_KMOD_CTRL) != 0)
    {
        nFlags |= MOUSE_CONTROL;
    }

    return nFlags;
}

// SDL's window coordinates as the client pixels Windows sent.
static std::pair<int, int> ClientPixels(float flX, float flY)
{
    const auto flDensity = SDL_GetWindowPixelDensity(s_pWindow);

    return { (int)(flX * flDensity), (int)(flY * flDensity) };
}

static void HandleKey(const SDL_KeyboardEvent& key)
{
    // Windows sent Alt and anything held with it as system keys, which never reached the game.
    if ((key.mod & SDL_KMOD_ALT) != 0)
    {
        return;
    }

    const auto nKey = KeyMap::ToVirtualKey(key.scancode, key.key);

    if (nKey == 0)
    {
        return;
    }

    // Releases always reach the game, so no key is left held; F2 opens and closes the debug menu.
    if (!key.down)
    {
        InputLog::Add("Keyboard", nKey, false, nullptr);
        WndProc::OnKeyUp(nKey);
    }
    else if (!DebugMenu::WantsKeyboard() || nKey == Key::F2)
    {
        if (!key.repeat)
        {
            InputLog::Add("Keyboard", nKey, true, nullptr);
        }

        WndProc::OnKeyDown(nKey);
    }
}

static void HandleMouseButton(const SDL_MouseButtonEvent& button)
{
    // The game only ever saw the left and right buttons.
    const auto nMessage = [&]() -> unsigned int
    {
        if (button.button == SDL_BUTTON_LEFT)
        {
            if (button.down)
            {
                return INPUT_LEFT_DOWN;
            }

            return INPUT_LEFT_UP;
        }

        if (button.button == SDL_BUTTON_RIGHT)
        {
            if (button.down)
            {
                return INPUT_RIGHT_DOWN;
            }

            return INPUT_RIGHT_UP;
        }

        return 0;
    }();

    if (nMessage == 0 || (button.down && DebugMenu::WantsMouse()))
    {
        return;
    }

    auto buttons = SDL_GetMouseState(0, 0);

    if (button.down)
    {
        buttons |= SDL_BUTTON_MASK(button.button);
    }
    else
    {
        buttons &= ~SDL_BUTTON_MASK(button.button);
    }

    const auto [nX, nY] = ClientPixels(button.x, button.y);

    const auto nButton = [&]() -> unsigned int
    {
        if (button.button == SDL_BUTTON_LEFT)
        {
            return Key::LBUTTON;
        }

        return Key::RBUTTON;
    }();

    InputLog::Add("Mouse", nButton, button.down, nullptr);
    WndProc::OnMouseButton(nMessage, MouseFlags(buttons), nX, nY);
}

// A watch sees each window event as SDL's window procedure raises it, as the game saw the
// message, and keeps running through the modal loop of a drag.
static void HandleWindowEvent(SDL_Event* pEvent)
{
    switch (pEvent->type)
    {
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
            HandleKey(pEvent->key);
            break;

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
            HandleMouseButton(pEvent->button);
            break;

        case SDL_EVENT_MOUSE_MOTION:
            if (!DebugMenu::WantsMouse())
            {
                const auto [nX, nY] = ClientPixels(pEvent->motion.x, pEvent->motion.y);

                WndProc::OnMouseMove(MouseFlags(pEvent->motion.state), nX, nY);
            }

            break;

        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            WndProc::OnResize(pEvent->window.data1, pEvent->window.data2);
            break;

        case SDL_EVENT_WINDOW_FOCUS_GAINED:
            if (g_pActiveScreen != 0)
            {
                g_pActiveScreen->SetRepaintBackground(1);
            }

            WndProc::OnActivate(1);
            break;

        case SDL_EVENT_WINDOW_FOCUS_LOST:
            WndProc::OnActivate(0);
            break;

        case SDL_EVENT_WINDOW_EXPOSED:
            WndProc::HandlePaint();

            // A live resize exposes the window from inside the drag, so the game draws a frame.
            if (pEvent->window.data1 != 0 && s_bInLiveUpdate == false)
            {
                s_bInLiveUpdate = true;
                GameApp::MainUpdate();
                s_bInLiveUpdate = false;
            }

            break;

        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            if (g_bShuttingDown == 0)
            {
                WndProc::OnDestroy();
            }

            s_bQuit = true;
            break;

        case SDL_EVENT_QUIT:
            s_bQuit = true;
            break;

        default:
            break;
    }
}

// SDL's renderer can deliver window messages from inside its own waits, and the game must not
// draw again from there, so they wait here until the frame is out.
// ponytail: a frame defers a handful at most; past this many the rest are dropped.
static std::array<SDL_Event, 64> s_aDeferred;
static size_t                    s_nDeferred = 0;

static void Defer(const SDL_Event& deferred)
{
    if (s_nDeferred < s_aDeferred.size())
    {
        s_aDeferred[s_nDeferred++] = deferred;
    }
}

static void ReplayDeferred()
{
    // Copied out first, as a replayed message can present a frame that defers more.
    auto       aPending = s_aDeferred;
    const auto nPending = s_nDeferred;

    s_nDeferred = 0;

    for (size_t i = 0; i < nPending; i++)
    {
        HandleWindowEvent(&aPending[i]);
    }
}

static bool SDLCALL OnWindowEvent(void*, SDL_Event* pEvent)
{
    if (g_gameApp.m_hWnd == 0)
    {
        return true;
    }

    // ImGui queues what it is given, so it can take events mid-frame.
    DebugMenu::OnEvent(DebugMenu::Event{ pEvent });

    if (Presenter::Busy())
    {
        Defer(*pEvent);

        return true;
    }

    ReplayDeferred();
    HandleWindowEvent(pEvent);

    return true;
}

HWND Window::Create(const char* pszTitle)
{
    if (SDL_Init(SDL_INIT_VIDEO) == false)
    {
        return 0;
    }

    s_pWindow = SDL_CreateWindow(pszTitle, 640, 480, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN);

    if (s_pWindow == 0)
    {
        return 0;
    }

    const auto hWnd = (HWND)SDL_GetPointerProperty(SDL_GetWindowProperties(s_pWindow), SDL_PROP_WINDOW_WIN32_HWND_POINTER, 0);

    // STRING: HYPERX 0x004c8ccc
    const auto hIcon = LoadIconA(GetModuleHandleA(0), "APPICON");

    SendMessageA(hWnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
    SendMessageA(hWnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

    SDL_AddEventWatch(OnWindowEvent, 0);

    return hWnd;
}

Window::NativeWindow Window::Native()
{
    return NativeWindow(s_pWindow);
}

void Window::Show()
{
    SDL_ShowWindow(s_pWindow);
}

void Window::ApplyCurrentSize()
{
    auto nWidth  = 0;
    auto nHeight = 0;

    SDL_GetWindowSizeInPixels(s_pWindow, &nWidth, &nHeight);

    WndProc::OnResize(nWidth, nHeight);
}

// FUNCTION: HYPERX 0x0048f470
bool Window::AskYesNo(const char* pszText, const char* pszCaption)
{
    return ShowMessage(pszText, pszCaption, MessageButtons::YES_NO, MessageIcon::INFO) == MessageAnswer::YES;
}

Window::MessageAnswer Window::ShowMessage(const char* pszText, const char* pszCaption, MessageButtons buttons, MessageIcon icon)
{
    constexpr auto RETURN = SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT;
    constexpr auto ESCAPE = SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT;

    const auto aOK       = std::array{ SDL_MessageBoxButtonData{ RETURN | ESCAPE, (int)MessageAnswer::OK, "OK" } };
    const auto aOKCancel = std::array{ SDL_MessageBoxButtonData{ RETURN, (int)MessageAnswer::OK, "OK" }, SDL_MessageBoxButtonData{ ESCAPE, (int)MessageAnswer::CANCEL, "Cancel" } };
    const auto aYesNo    = std::array{ SDL_MessageBoxButtonData{ RETURN, (int)MessageAnswer::YES, "Yes" }, SDL_MessageBoxButtonData{ ESCAPE, (int)MessageAnswer::NO, "No" } };

    const auto choices = [&]
    {
        if (buttons == MessageButtons::OK_CANCEL)
        {
            return std::span<const SDL_MessageBoxButtonData>(aOKCancel);
        }

        if (buttons == MessageButtons::YES_NO)
        {
            return std::span<const SDL_MessageBoxButtonData>(aYesNo);
        }

        return std::span<const SDL_MessageBoxButtonData>(aOK);
    }();

    const auto flags = [&]
    {
        if (icon == MessageIcon::STOP)
        {
            return SDL_MESSAGEBOX_ERROR;
        }

        if (icon == MessageIcon::WARNING)
        {
            return SDL_MESSAGEBOX_WARNING;
        }

        return SDL_MESSAGEBOX_INFORMATION;
    }();

    const auto data = SDL_MessageBoxData{ flags, s_pWindow, pszCaption, pszText, (int)choices.size(), choices.data(), nullptr };

    auto nChosen = choices.back().buttonID;

    SDL_ShowMessageBox(&data, &nChosen);

    return (MessageAnswer)nChosen;
}

void Window::SetBusyCursor(bool bBusy)
{
    static SDL_Cursor* s_pBusy = 0;

    if (bBusy == false)
    {
        SDL_SetCursor(SDL_GetDefaultCursor());

        return;
    }

    if (s_pBusy == 0)
    {
        s_pBusy = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_WAIT);
    }

    SDL_SetCursor(s_pBusy);
}

void Window::Destroy()
{
    SDL_DestroyWindow(s_pWindow);
    SDL_Quit();

    s_pWindow = 0;
}

bool Window::Pump()
{
    SDL_PumpEvents();

    ReplayDeferred();

    // Everything the game needs was handled as it arrived.
    SDL_FlushEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST);

    return s_bQuit == false;
}

void Window::WaitWhileInactive()
{
    // A tenth of a second, so a message that raises no SDL event cannot leave the loop asleep.
    SDL_WaitEventTimeout(0, 100);
}
