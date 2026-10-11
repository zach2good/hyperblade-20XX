#ifndef WNDPROC_H
#define WNDPROC_H

#include "common/compat.h"
#include "ui/mainwindow.h"
#include "graphics/swrend.h"

namespace WndProc
{

int __cdecl          HandlePaint(HWND hWnd);
int __cdecl          OnActivate(HWND hWnd, unsigned int bActive, LPARAM lParam);
unsigned int __cdecl HandleRenderMenuCommand(HMENU hMenu, unsigned short nCommand);
LRESULT CALLBACK     MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

} // namespace WndProc

#endif
