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

#ifdef HYPERX_BRINGUP
    // A full-screen popup never becomes visible on a modern desktop.
    SetWindowLongA(hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);
    SetWindowLongA(hWnd, GWL_ID, 0);
    SetWindowPos(hWnd, 0, 0, 0, 660, 520, SWP_NOZORDER | SWP_FRAMECHANGED);
#else
    SetWindowLongA(hWnd, GWL_STYLE, WS_POPUP);
    SetWindowLongA(hWnd, GWL_ID, 0);

    Window::OnWindowMove(hWnd, 0);
#endif

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
#ifdef HYPERX_BRINGUP
    // Bring-up paints on demand, so the class's erase would flash.
    g_wndClass.hbrBackground = 0;
#else
    g_wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
#endif
    g_wndClass.lpszMenuName  = 0;
    g_wndClass.lpszClassName = "HyberBlade Atom String";

    RegisterClassA(&g_wndClass);

    return 0;
}
