#ifndef HYPERBLADE_WINDOW_H
#define HYPERBLADE_WINDOW_H

#include "common/compat.h"

namespace Window
{

void         InvalidateAndUpdate(void);
void         ApplyFullscreenStyle(HWND hWnd, int nFlag);
int          EnsureClassRegistered(HINSTANCE hInstance);
unsigned int OnWindowMove(HWND hWnd, int nFlag);

#ifdef HYPERX_V100
// Without Direct3D it also paints the game window black.
BOOL Destroy(HWND hWnd);
#endif

} // namespace Window

#endif
