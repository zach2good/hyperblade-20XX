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
inline int __cdecl SaveKeysAs(HWND hDlg, struct KeyRow* pRows);
inline int __cdecl ClearKeys(HWND hDlg, struct KeyRow* pRows);

#ifdef HYPERX_V100
int __cdecl  SaveKeys(HWND hDlg, struct KeyRow* pRows);
void __cdecl GetHeldMoveKeys(unsigned int nHeldBindings, unsigned int* pnKeys);
void __cdecl ApplyModifier80(int nContext, char bSet, int nSlot);
void __cdecl ApplyModifier100(int nContext, char bSet, int nSlot);
#endif

} // namespace KeyConfigDialog

// Inline, so bare `return 0` and `return 1` bodies fold onto these; callers turn inlining off.
// FUNCTION: HYPERX100 0x00411110
// FUNCTION: HYPERX 0x0042bcd0
inline int __cdecl KeyConfigDialog::SaveKeysAs(HWND, struct KeyRow*)
{
    return 0;
}

// FUNCTION: HYPERX100 0x00411120
// FUNCTION: HYPERX 0x0046d700
inline int __cdecl KeyConfigDialog::ClearKeys(HWND, struct KeyRow*)
{
    return 1;
}

#endif
