#include "common/compat.h"
#include "globals.h"
#include "graphics/ddrawapi.h"
#include "gameapp.h"
#include "app/window.h"
#include "wndproc.h"
#include "app/bringup.h"
#include "engine/gameengine.h"
#include "engine/objectclasses.h"
#include "engine/gameoptions.h"
#include "graphics/graphicsglobals.h"
#include "graphics/softrenderer.h"
#include <algorithm>

#define SWP_MOVE_FLAGS (SWP_SHOWWINDOW | SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOMOVE)

// The second argument is unread.
// FUNCTION: HYPERX 0x0044bd60
void Window::ApplyFullscreenStyle(HWND hWnd, int)
{
    if (g_gameApp.m_hMenu != 0)
    {
        SetMenu(hWnd, 0);
    }

    // Every pause toggle comes here, and a framed window keeps the size the player gave it.
    if ((GetWindowLongA(hWnd, GWL_STYLE) & WS_OVERLAPPEDWINDOW) == WS_OVERLAPPEDWINDOW)
    {
        return;
    }

    // A full-screen popup never becomes visible on a modern desktop, so the game runs framed.
    RECT rcFrame = { 0, 0, (LONG)g_dwDisplayWidth, (LONG)g_dwDisplayHeight };

    SetWindowLongA(hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);
    SetWindowLongA(hWnd, GWL_ID, 0);
    AdjustWindowRect(&rcFrame, WS_OVERLAPPEDWINDOW, FALSE);
    SetWindowPos(hWnd, 0, 0, 0, rcFrame.right - rcFrame.left, rcFrame.bottom - rcFrame.top, SWP_NOZORDER | SWP_NOMOVE | SWP_FRAMECHANGED);

    PostMessageA(hWnd, WM_NCPAINT, 0, 0);
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

// GLOBAL: HYPERX 0x004d2b88
WNDCLASSA g_wndClass;

// GLOBAL: HYPERX 0x004d2bb0
WNDCLASSA* g_pWndClass;

// FUNCTION: HYPERX 0x0048f450
void Window::InvalidateAndUpdate(void)
{
    InvalidateRgn(g_gameApp.m_hWnd, 0, 0);

    UpdateWindow(g_gameApp.m_hWnd);
}

// Non-zero when the class already existed. The class name's typo is shipped.
// FUNCTION: HYPERX 0x00493750
int Window::EnsureClassRegistered(HINSTANCE hInstance)
{
    // STRING: HYPERX 0x004c8cac
    const BOOL bClassExists = GetClassInfoA(hInstance, "HyberBlade Atom String", &g_wndClass);

    if (g_pWndClass != 0)
    {
        return 1;
    }

    g_pWndClass = &g_wndClass;

    if (bClassExists != 0)
    {
        return 1;
    }

    g_wndClass.style       = CS_BYTEALIGNWINDOW | CS_BYTEALIGNCLIENT | CS_OWNDC;
    g_wndClass.lpfnWndProc = WndProc::MainWndProc;
    g_wndClass.cbWndExtra  = 0;
    g_wndClass.cbClsExtra  = 0;
    g_wndClass.hInstance   = hInstance;
    g_wndClass.hIcon       = 0;
    g_wndClass.hCursor     = 0;
    // The game paints on demand, so the class's erase would flash.
    g_wndClass.hbrBackground = 0;
    g_wndClass.lpszMenuName  = 0;
    g_wndClass.lpszClassName = "HyberBlade Atom String";

    RegisterClassA(&g_wndClass);

    return 0;
}

// The menus and splashes paint at 640x480. They draw into this canvas, which is scaled to the
// window keeping its shape, and the mouse is mapped back into it.
constexpr auto MENU_WIDTH  = 640;
constexpr auto MENU_HEIGHT = 480;

static HDC     s_hMenuDC     = 0;
static HBITMAP s_hMenuBitmap = 0;

HDC Window::MenuDC()
{
    if (s_hMenuDC == 0)
    {
        const HDC hdcWindow = GetDC(g_gameApp.m_hWnd);

        s_hMenuDC     = CreateCompatibleDC(hdcWindow);
        s_hMenuBitmap = CreateCompatibleBitmap(hdcWindow, MENU_WIDTH, MENU_HEIGHT);

        ReleaseDC(g_gameApp.m_hWnd, hdcWindow);

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

void Window::PresentMenu(HWND hWnd)
{
    const auto rc = MenuRect(hWnd);

    if (rc.right <= rc.left || rc.bottom <= rc.top)
    {
        return;
    }

    RECT rcClient;

    GetClientRect(hWnd, &rcClient);

    const HDC hdc = GetDC(hWnd);

    SetStretchBltMode(hdc, HALFTONE);
    SetBrushOrgEx(hdc, 0, 0, 0);
    StretchBlt(hdc, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, Window::MenuDC(), 0, 0, MENU_WIDTH, MENU_HEIGHT, SRCCOPY);

    const auto hBlack = (HBRUSH)GetStockObject(BLACK_BRUSH);
    RECT       rcBar;

    SetRect(&rcBar, 0, 0, rc.left, rcClient.bottom);
    FillRect(hdc, &rcBar, hBlack);
    SetRect(&rcBar, rc.right, 0, rcClient.right, rcClient.bottom);
    FillRect(hdc, &rcBar, hBlack);
    SetRect(&rcBar, rc.left, 0, rc.right, rc.top);
    FillRect(hdc, &rcBar, hBlack);
    SetRect(&rcBar, rc.left, rc.bottom, rc.right, rcClient.bottom);
    FillRect(hdc, &rcBar, hBlack);

    ReleaseDC(hWnd, hdc);
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

// A drag runs a modal loop inside DispatchMessageA, and this timer keeps the frames coming.
constexpr auto SIZEMOVE_TIMER_ID = 0x4252u;
constexpr auto SIZEMOVE_TIMER_MS = 33u;

static int s_bInSizeMove    = 0;
static int s_bInTimerUpdate = 0;

void Window::OnEnterSizeMove(HWND hWnd)
{
    s_bInSizeMove = 1;

    SetTimer(hWnd, SIZEMOVE_TIMER_ID, SIZEMOVE_TIMER_MS, 0);
}

void Window::OnExitSizeMove(HWND hWnd)
{
    KillTimer(hWnd, SIZEMOVE_TIMER_ID);

    s_bInSizeMove = 0;
}

int Window::OnTimer(HWND hWnd, UINT_PTR nTimerId)
{
    if (nTimerId != SIZEMOVE_TIMER_ID)
    {
        return 0;
    }

    if (s_bInTimerUpdate == 0 && s_bInSizeMove != 0)
    {
        s_bInTimerUpdate = 1;
        GameApp::MainUpdate(hWnd);
        s_bInTimerUpdate = 0;
    }

    return 1;
}

int Window::InSizeMove()
{
    return s_bInSizeMove;
}

// The 3D renders at the client size, so it is never stretched. The width stays a multiple of four,
// which the back buffer needs, and PresentFrame centres what that leaves.
void Window::OnSize(WPARAM nKind, int cx, int cy)
{
#ifdef HYPERX_BRINGUP
    BringUp::Trace("sizeKind", (int)nKind);
    BringUp::Trace("sizeWidth", cx);
    BringUp::Trace("sizeHeight", cy);
#endif

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
