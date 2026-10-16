#ifndef HYPERBLADE_DIALOG_H
#define HYPERBLADE_DIALOG_H

#include "common/compat.h"

namespace Dialog
{

BOOL  SetItemText(HWND hDlg, int nIDDlgItem, float flValue);
float GetItemFloat(HWND hDlg, int nIDDlgItem, int* pbValid);

} // namespace Dialog

#endif
