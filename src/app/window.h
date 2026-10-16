#ifndef HYPERBLADE_WINDOW_H
#define HYPERBLADE_WINDOW_H

#include "common/compat.h"

namespace Window
{

void         InvalidateAndUpdate(void);
void         ApplyFullscreenStyle(HWND hWnd, int nFlag);
unsigned int OnWindowMove(HWND hWnd, int nFlag);
HDC          MenuDC();
void         PresentMenu(HWND hWnd);
LPARAM       ToMenuPoint(HWND hWnd, LPARAM lParam);
void         OnSizeMenus(HWND hWnd, WPARAM nKind);
void         OnSize(WPARAM nKind, int cx, int cy);

// SDL owns the window and its event loop. Window events reach the game as they happen, and
// the input messages it reads raw go to WndProc::MainWndProc unchanged.
HWND Create(const char* pszTitle);
void Show();
void Destroy();

// Handles what has arrived; false once the game has asked to quit.
bool Pump();
void WaitWhileInactive();

} // namespace Window

#endif
