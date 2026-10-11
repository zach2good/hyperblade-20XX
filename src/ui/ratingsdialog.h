#ifndef HYPERBLADE_RATINGSDIALOG_H
#define HYPERBLADE_RATINGSDIALOG_H

#include "common/compat.h"

namespace RatingsDialog
{

int __cdecl          InitControls(HWND hDlg);
int __cdecl          OnInit(HWND hDlg, HWND hwndFocus, LPARAM lParam);
void __cdecl         Close(HWND hWnd, INT_PTR nResult);
unsigned int __cdecl OnCommand(HWND hDlg, int nCommand, HWND hControl, int nNotify);

} // namespace RatingsDialog

#endif
