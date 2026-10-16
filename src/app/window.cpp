#include "common/compat.h"
#include "globals.h"
#include "graphics/ddrawapi.h"
#include "gameapp.h"
#include "app/window.h"
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
#include <SDL3/SDL.h>
#include <array>
#include <span>
#include <algorithm>

// The game's one window, which SDL made and owns.
static SDL_Window* s_pWindow = 0;

#define SWP_MOVE_FLAGS (SWP_SHOWWINDOW | SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOMOVE)

// The second argument is unread.
// FUNCTION: HYPERX 0x0044bd60
void Window::ApplyFullscreenStyle(HWND hWnd, int)
{
    if (g_gameApp.m_hMenu != 0)
    {
        SetMenu(hWnd, 0);
    }

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

// The original swaps X and Y too; SWP_NOMOVE ignores both.
// FUNCTION: HYPERX 0x0044bdb0
unsigned int Window::OnWindowMove(HWND hWnd, int nFlag)
{
    RECT rc;

    GetWindowRect(hWnd, &rc);

    SetWindowPos(hWnd, 0, rc.top, rc.left, rc.right - rc.left, rc.bottom - rc.top, SWP_MOVE_FLAGS);

    if (g_cUseSoftRenderer == 0)
    {
        DDraw::BlitBackToWindow(hWnd, nFlag);
    }

    return 0;
}

// FUNCTION: HYPERX 0x0048f450
void Window::InvalidateAndUpdate(void)
{
    InvalidateRgn(g_gameApp.m_hWnd, 0, 0);

    UpdateWindow(g_gameApp.m_hWnd);
}

// The menus and splashes paint at 640x480. They draw into this canvas, which is scaled to the
// window keeping its shape, and the mouse is mapped back into it.
constexpr auto MENU_WIDTH  = 640;
constexpr auto MENU_HEIGHT = 480;

static HDC     s_hMenuDC     = 0;
static HBITMAP s_hMenuBitmap = 0;
static void*   s_pMenuBits   = 0;

HDC Window::MenuDC()
{
    if (s_hMenuDC == 0)
    {
        BITMAPINFO info = {};

        info.bmiHeader.biSize     = sizeof(info.bmiHeader);
        info.bmiHeader.biWidth    = MENU_WIDTH;
        info.bmiHeader.biHeight   = -MENU_HEIGHT;
        info.bmiHeader.biPlanes   = 1;
        info.bmiHeader.biBitCount = 32;

        // A 32-bit DIB, so SDL can upload what GDI drew without a conversion.
        s_hMenuDC     = CreateCompatibleDC(0);
        s_hMenuBitmap = CreateDIBSection(s_hMenuDC, &info, DIB_RGB_COLORS, &s_pMenuBits, 0, 0);

        SelectObject(s_hMenuDC, s_hMenuBitmap);
        PatBlt(s_hMenuDC, 0, 0, MENU_WIDTH, MENU_HEIGHT, BLACKNESS);
    }

    return s_hMenuDC;
}

// The canvas's place in the client area: as large as fits, centred.
static RECT MenuRect(HWND hWnd)
{
    RECT rcClient;

    GetClientRect(hWnd, &rcClient);

    auto cx = (int)rcClient.right;
    auto cy = cx * MENU_HEIGHT / MENU_WIDTH;

    if (cy > rcClient.bottom)
    {
        cy = rcClient.bottom;
        cx = cy * MENU_WIDTH / MENU_HEIGHT;
    }

    const auto x = (rcClient.right - cx) / 2;
    const auto y = (rcClient.bottom - cy) / 2;

    return RECT{ x, y, x + cx, y + cy };
}

// SDL fits the canvas the same way MenuRect does, so the mouse mapping agrees with the picture.
void Window::PresentMenu(HWND)
{
    Window::MenuDC();

    GdiFlush();

    Presenter::Canvas(s_pMenuBits, MENU_WIDTH * 4, MENU_WIDTH, MENU_HEIGHT);
}

LPARAM Window::ToMenuPoint(HWND hWnd, LPARAM lParam)
{
    const auto rc = MenuRect(hWnd);
    const auto cx = rc.right - rc.left;
    const auto cy = rc.bottom - rc.top;

    if (cx <= 0 || cy <= 0)
    {
        return lParam;
    }

    const auto x = ((short)LOWORD(lParam) - rc.left) * MENU_WIDTH / cx;
    const auto y = ((short)HIWORD(lParam) - rc.top) * MENU_HEIGHT / cy;

    return MAKELPARAM(x, y);
}

// The 3D renders at the client size, so it is never stretched. The width stays a multiple of four,
// which the back buffer needs, and PresentFrame centres what that leaves.
void Window::OnSize(WPARAM nKind, int cx, int cy)
{
    if (nKind == SIZE_MINIMIZED || g_hSoftBackBitmap == 0)
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
void Window::OnSizeMenus(HWND hWnd, WPARAM nKind)
{
    if (nKind != SIZE_MINIMIZED && g_gameApp.m_nEngineState != ESTATE_MATCH_LIVE)
    {
        Window::PresentMenu(hWnd);
    }
}

static bool s_bQuit         = false;
static bool s_bInLiveUpdate = false;

// The input the game reads raw keeps its Win32 form: key codes, button flags and client points.
static bool SDLCALL OnWin32Message(void*, MSG* pMsg)
{
    if (pMsg->message == WM_QUIT)
    {
        s_bQuit = true;

        return true;
    }

    if (pMsg->hwnd == 0 || pMsg->hwnd != g_gameApp.m_hWnd)
    {
        return true;
    }

    switch (pMsg->message)
    {
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MOUSEMOVE:
        case WM_COMMAND:
            WndProc::MainWndProc(pMsg->hwnd, pMsg->message, pMsg->wParam, pMsg->lParam);
            break;

        default:
            break;
    }

    return true;
}

// A watch sees each window event as SDL's window procedure raises it, as the game saw the
// message, and keeps running through the modal loop of a drag.
static bool SDLCALL OnWindowEvent(void*, SDL_Event* pEvent)
{
    const HWND hWnd = g_gameApp.m_hWnd;

    if (hWnd == 0)
    {
        return true;
    }

    switch (pEvent->type)
    {
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
            WndProc::MainWndProc(hWnd, WM_SIZE, SIZE_RESTORED, MAKELPARAM(pEvent->window.data1, pEvent->window.data2));
            break;

        case SDL_EVENT_WINDOW_MINIMIZED:
            WndProc::MainWndProc(hWnd, WM_SIZE, SIZE_MINIMIZED, 0);
            break;

        case SDL_EVENT_WINDOW_FOCUS_GAINED:
            if (g_pActiveScreen != 0)
            {
                g_pActiveScreen->SetRepaintBackground(1);
            }

            WndProc::OnActivate(hWnd, 1, 0);
            break;

        case SDL_EVENT_WINDOW_FOCUS_LOST:
            WndProc::OnActivate(hWnd, 0, 0);
            break;

        case SDL_EVENT_WINDOW_EXPOSED:
            WndProc::HandlePaint(hWnd);

            // A live resize exposes the window from inside the drag, so the game draws a frame.
            if (pEvent->window.data1 != 0 && s_bInLiveUpdate == false)
            {
                s_bInLiveUpdate = true;
                GameApp::MainUpdate(hWnd);
                s_bInLiveUpdate = false;
            }

            break;

        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
            if (g_bShuttingDown == 0)
            {
                WndProc::MainWndProc(hWnd, WM_DESTROY, 0, 0);
            }

            s_bQuit = true;
            break;

        default:
            break;
    }

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

    SDL_SetWindowsMessageHook(OnWin32Message, 0);
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

    WndProc::MainWndProc(g_gameApp.m_hWnd, WM_SIZE, SIZE_RESTORED, MAKELPARAM(nWidth, nHeight));
}

int Window::ShowMessage(const char* pszText, const char* pszCaption, unsigned int nType)
{
    constexpr auto RETURN = SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT;
    constexpr auto ESCAPE = SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT;

    const auto aOK       = std::array{ SDL_MessageBoxButtonData{ RETURN | ESCAPE, IDOK, "OK" } };
    const auto aOKCancel = std::array{ SDL_MessageBoxButtonData{ RETURN, IDOK, "OK" }, SDL_MessageBoxButtonData{ ESCAPE, IDCANCEL, "Cancel" } };
    const auto aYesNo    = std::array{ SDL_MessageBoxButtonData{ RETURN, IDYES, "Yes" }, SDL_MessageBoxButtonData{ ESCAPE, IDNO, "No" } };

    const auto buttons = [&]
    {
        if ((nType & MB_TYPEMASK) == MB_OKCANCEL)
        {
            return std::span<const SDL_MessageBoxButtonData>(aOKCancel);
        }

        if ((nType & MB_TYPEMASK) == MB_YESNO)
        {
            return std::span<const SDL_MessageBoxButtonData>(aYesNo);
        }

        return std::span<const SDL_MessageBoxButtonData>(aOK);
    }();

    const auto flags = [&]
    {
        if ((nType & MB_ICONMASK) == MB_ICONHAND)
        {
            return SDL_MESSAGEBOX_ERROR;
        }

        if ((nType & MB_ICONMASK) == MB_ICONEXCLAMATION)
        {
            return SDL_MESSAGEBOX_WARNING;
        }

        return SDL_MESSAGEBOX_INFORMATION;
    }();

    const auto data = SDL_MessageBoxData{ flags, s_pWindow, pszCaption, pszText, (int)buttons.size(), buttons.data(), nullptr };

    auto nChosen = buttons.back().buttonID;

    SDL_ShowMessageBox(&data, &nChosen);

    return nChosen;
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

    // Everything the game needs was handled as it arrived.
    SDL_FlushEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST);

    return s_bQuit == false;
}

void Window::WaitWhileInactive()
{
    // A tenth of a second, so a message that raises no SDL event cannot leave the loop asleep.
    SDL_WaitEventTimeout(0, 100);
}
