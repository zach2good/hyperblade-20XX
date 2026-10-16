#ifndef HYPERBLADE_NETSETUPDIALOG_H
#define HYPERBLADE_NETSETUPDIALOG_H

#include "common/compat.h"
#include "net/directplay.h"

struct PlayerSlotName;

namespace NetSetupDialog
{

unsigned int FinalizeAndStartGame(void);
int          OnTimer(HWND hDlg);
BOOL         AddProvider(LPGUID pGuid, LPSTR pszName, DWORD nMajor, DWORD nMinor, LPVOID pContext);
unsigned int EnumerateProviders(HWND hDlg);
BOOL         AddSessionEntry(LPDPSESSIONDESC pDesc, LPVOID pContext, LPDWORD pnUnusedA, DWORD nUnusedB);
unsigned int SearchSessions(HWND hDlg, unsigned int nMsg, unsigned int nParam, long lParam);
unsigned int SelectSession(HWND hDlg, unsigned int nMsg, unsigned int nParam, long lParam);
BOOL         OnEnumeratedPlayer(DWORD nPlayerId, LPSTR pszUnusedA, LPSTR pszUnusedB, DWORD nUnusedC, LPVOID pUnusedD);
unsigned int JoinSession(HWND hDlg, unsigned int nMsg, unsigned int nParam, long lParam);
unsigned int VerifyAssets(HWND hDlg, unsigned int nMsg, unsigned int nParam, long lParam);
void         PopulateSelectionLists(HWND hDlg);
unsigned int OnSelectionChanged(HWND hDlg, unsigned int nUnused, unsigned int nControl, long lParam);
unsigned int HostSession(HWND hDlg, unsigned int nMsg, unsigned int nParam, long lParam);
int          ValidatePlayerName(HWND hDlg, unsigned int nMsg, unsigned int nParam, long lParam);
unsigned int SelectPlayerPosition(HWND hDlg, unsigned int nUnused, unsigned int nControl, long lParam);
unsigned int ConfirmPlayerPosition(HWND hDlg, unsigned int nMsg, unsigned int nParam, long lParam);
unsigned int SendChatMessage(HWND hDlg, unsigned int nMsg, unsigned int nParam, long lParam);
unsigned int StartGame(HWND hDlg, unsigned int nMsg, unsigned int nParam, long lParam);
unsigned int OnPlayerJoined(unsigned int nFlags, PlayerSlotName* pEntry);
unsigned int RefreshSessionList(HWND hDlg);
BOOL         AddEnumeratedSession(LPDPSESSIONDESC pDesc, LPVOID pUnusedA, LPDWORD pnUnusedB, DWORD nFlags);
unsigned int OnChangeSlot(HWND hDlg);
int          CheckPlayerReadyButton(int nPlayer);
int          PopulateSessionList(HWND hDlg);

} // namespace NetSetupDialog

// GLOBAL: HYPERX 0x004c55dc
extern int g_aPlayerReadyButtonId[16];
// GLOBAL: HYPERX 0x004d0788
extern HWND g_hWndNetSetupDlg;

#endif
