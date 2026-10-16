#ifndef HYPERBLADE_WINDOW_H
#define HYPERBLADE_WINDOW_H

#include "common/compat.h"

namespace Window
{

void         InvalidateAndUpdate(void);
void         ApplyFullscreenStyle(HWND hWnd, int nFlag);
int          EnsureClassRegistered(HINSTANCE hInstance);
unsigned int OnWindowMove(HWND hWnd, int nFlag);
void         ClearSurround(HWND hWnd, int cx, int cy);
void         OnEnterSizeMove(HWND hWnd);
void         OnExitSizeMove(HWND hWnd);
int          OnTimer(HWND hWnd, UINT_PTR nTimerId);
int          InSizeMove();

} // namespace Window

#endif
