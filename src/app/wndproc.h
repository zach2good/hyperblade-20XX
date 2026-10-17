#ifndef WNDPROC_H
#define WNDPROC_H

#include "common/compat.h"
#include "input/inputevent.h"
#include "ui/mainwindow.h"
#include "graphics/swrend.h"

namespace WndProc
{

int              HandlePaint(HWND hWnd);
int              OnActivate(HWND hWnd, unsigned int bActive, LPARAM lParam);
unsigned int     HandleRenderMenuCommand(unsigned short nCommand);
LRESULT CALLBACK MainWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

// Input from the window. Keys are Win32 virtual-key codes; nMessage is INPUT_LEFT_DOWN, INPUT_LEFT_UP,
// INPUT_RIGHT_DOWN or INPUT_RIGHT_UP; buttons are MOUSE_ flags; points are client pixels.
void OnKeyDown(unsigned int nKey);
void OnKeyUp(unsigned int nKey);
void OnMouseButton(unsigned int nMessage, unsigned int nButtons, int nX, int nY);
void OnMouseMove(unsigned int nButtons, int nX, int nY);

} // namespace WndProc

#endif
