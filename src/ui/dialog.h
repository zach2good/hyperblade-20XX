#ifndef HYPERBLADE_DIALOG_H
#define HYPERBLADE_DIALOG_H

#include "common/compat.h"

namespace Dialog
{

BOOL __cdecl  SetItemText(HWND hDlg, int nIDDlgItem, float flValue);
float __cdecl GetItemFloat(HWND hDlg, int nIDDlgItem, int* pbValid);

} // namespace Dialog

#endif
