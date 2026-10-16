#ifndef HYPERBLADE_WINDOW_H
#define HYPERBLADE_WINDOW_H

#include "common/compat.h"

namespace Window
{

void         InvalidateAndUpdate(void);
void         ApplyFullscreenStyle(HWND hWnd, int nFlag);
int          EnsureClassRegistered(HINSTANCE hInstance);
unsigned int OnWindowMove(HWND hWnd, int nFlag);
HDC          MenuDC();
void         PresentMenu(HWND hWnd);
LPARAM       ToMenuPoint(HWND hWnd, LPARAM lParam);
void         OnSizeMenus(HWND hWnd, WPARAM nKind);
void         OnEnterSizeMove(HWND hWnd);
void         OnExitSizeMove(HWND hWnd);
int          OnTimer(HWND hWnd, UINT_PTR nTimerId);
int          InSizeMove();
void         OnSize(WPARAM nKind, int cx, int cy);

} // namespace Window

#endif
