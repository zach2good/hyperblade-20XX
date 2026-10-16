#ifndef HYPERBLADE_MAINWINDOW_H
#define HYPERBLADE_MAINWINDOW_H

#include "common/compat.h"

namespace MainWindow
{

int              OnCreate(HWND hWnd, LPARAM lParam);
LRESULT __cdecl  OnCommand(HWND hWnd, unsigned int nCommand, LPARAM lParam, int nNotify);
int              OpenPlanEditor(HINSTANCE hInstance, HWND hParent);
INT_PTR CALLBACK RatingsDialogProc(HWND hDlg, UINT nMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK DebugParamsDialogProc(HWND hDlg, UINT nMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK KeyboardDialogProc(HWND hDlg, UINT nMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK PhysicsConfigProc(HWND hDlg, UINT nMsg, WPARAM wParam, LPARAM lParam);

} // namespace MainWindow

#endif
