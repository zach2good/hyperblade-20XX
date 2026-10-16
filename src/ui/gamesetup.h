#ifndef HYPERBLADE_GAMESETUP_H
#define HYPERBLADE_GAMESETUP_H

#include "common/compat.h"

namespace GameSetup
{

INT_PTR CALLBACK SessionListDialogProc(HWND hDlg, UINT nMsg, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK SessionConnectDialogProc(HWND hDlg, UINT nMsg, WPARAM wParam, LPARAM lParam);
int              ShowPreMatchDialogs(void);

} // namespace GameSetup

#endif
