#ifndef HYPERBLADE_SPLASHDIALOG_H
#define HYPERBLADE_SPLASHDIALOG_H

#include "common/compat.h"

namespace SplashDialog
{

void             ShowAbout(HINSTANCE hInstance, HWND hWnd);
int __cdecl      OnCommand(HWND hDlg, int nId, LONG lParam, int nNotify);
BOOL __cdecl     OnInitDialog(HWND hDlg, UINT wParam, LONG lParam);
BOOL __cdecl     PaintSplashBitmap(HWND hDlg);
INT_PTR CALLBACK Proc(HWND hDlg, UINT nMsg, WPARAM wParam, LPARAM lParam);

} // namespace SplashDialog

#endif
