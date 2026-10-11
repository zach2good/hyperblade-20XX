#ifndef HYPERBLADE_KEYCONFIGDIALOG_H
#define HYPERBLADE_KEYCONFIGDIALOG_H

#include "common/compat.h"

namespace KeyConfigDialog
{

int __cdecl          Init(HWND hDlg);
int __cdecl          OnInitDialog(HWND hDlg, HWND hwndFocus, LPARAM lParam);
void __cdecl         CloseDialog(HWND hWnd, INT_PTR nResult);
unsigned int __cdecl OnCommand(HWND hDlg, int nCommand, HWND hControl, int nNotify);

// Unwritten in the original, and the linker shares each body with unrelated table slots.
int __cdecl SaveKeysAs(HWND hDlg, struct KeyRow* pRows);
int __cdecl ClearKeys(HWND hDlg, struct KeyRow* pRows);

} // namespace KeyConfigDialog

#endif
