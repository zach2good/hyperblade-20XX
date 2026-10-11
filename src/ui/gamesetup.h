#ifndef HYPERBLADE_GAMESETUP_H
#define HYPERBLADE_GAMESETUP_H

#include "common/compat.h"

namespace GameSetup
{

BOOL CALLBACK SessionListDialogProc(HWND hDlg, UINT nMsg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK SessionConnectDialogProc(HWND hDlg, UINT nMsg, WPARAM wParam, LPARAM lParam);
int           ShowPreMatchDialogs(void);

} // namespace GameSetup

#endif
