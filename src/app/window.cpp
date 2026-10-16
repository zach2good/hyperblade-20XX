#include "common/compat.h"
#include "globals.h"
#include "graphics/ddrawapi.h"
#include "gameapp.h"
#include "app/window.h"
#include "wndproc.h"

#define SWP_MOVE_FLAGS (SWP_SHOWWINDOW | SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOMOVE)

// The second argument is unread.
// FUNCTION: HYPERX 0x0044bd60
void Window::ApplyFullscreenStyle(HWND hWnd, int)
{
    if (g_gameApp.m_hMenu != 0)
    {
        SetMenu(hWnd, 0);
    }

    // A full-screen popup never becomes visible on a modern desktop, so the game runs framed.
    RECT rcFrame = { 0, 0, 640, 480 };

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

// Menus paint and hit-test at the origin unscaled, so only the surround is cleared.
void Window::ClearSurround(HWND hWnd, int cx, int cy)
{
    RECT rcClient;

    GetClientRect(hWnd, &rcClient);

    if (rcClient.right <= cx && rcClient.bottom <= cy)
    {
        return;
    }

    const HDC hdc = GetDC(hWnd);

    if (hdc == 0)
    {
        return;
    }

    RECT rcBar;

    if (rcClient.right > cx)
    {
        SetRect(&rcBar, cx, 0, rcClient.right, rcClient.bottom);
        FillRect(hdc, &rcBar, (HBRUSH)GetStockObject(BLACK_BRUSH));
    }

    if (rcClient.bottom > cy)
    {
        SetRect(&rcBar, 0, cy, rcClient.right, rcClient.bottom);
        FillRect(hdc, &rcBar, (HBRUSH)GetStockObject(BLACK_BRUSH));
    }

    ReleaseDC(hWnd, hdc);
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
