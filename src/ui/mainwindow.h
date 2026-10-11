#ifndef HYPERBLADE_MAINWINDOW_H
#define HYPERBLADE_MAINWINDOW_H

#include "common/compat.h"

namespace MainWindow
{

int             OnCreate(HWND hWnd, LPARAM lParam);
LRESULT __cdecl OnCommand(HWND hWnd, unsigned int nCommand, LPARAM lParam, int nNotify);
int             OpenPlanEditor(HINSTANCE hInstance, HWND hParent);
BOOL CALLBACK   RatingsDialogProc(HWND hDlg, UINT nMsg, UINT wParam, LONG lParam);
BOOL CALLBACK   DebugParamsDialogProc(HWND hDlg, UINT nMsg, UINT wParam, LONG lParam);
BOOL CALLBACK   KeyboardDialogProc(HWND hDlg, UINT nMsg, UINT wParam, LONG lParam);
BOOL CALLBACK   PhysicsConfigProc(HWND hDlg, UINT nMsg, UINT wParam, LONG lParam);

} // namespace MainWindow

#endif
