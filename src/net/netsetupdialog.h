#ifndef HYPERBLADE_NETSETUPDIALOG_H
#define HYPERBLADE_NETSETUPDIALOG_H

#include "common/compat.h"
#include "net/directplay.h"

struct PlayerSlotName;

namespace NetSetupDialog
{

unsigned int         FinalizeAndStartGame(void);
int                  OnTimer(HWND hDlg);
BOOL __stdcall       AddProvider(LPGUID pGuid, LPSTR pszName, DWORD nMajor, DWORD nMinor, LPVOID pContext);
unsigned int __cdecl EnumerateProviders(HWND hDlg);
BOOL __stdcall       AddSessionEntry(LPDPSESSIONDESC pDesc, LPVOID pContext, LPDWORD pnUnusedA, DWORD nUnusedB);
unsigned int __cdecl SearchSessions(HWND hDlg, unsigned int nMsg, unsigned int nParam, long lParam);
unsigned int __cdecl SelectSession(HWND hDlg, unsigned int nMsg, unsigned int nParam, long lParam);
BOOL __stdcall       OnEnumeratedPlayer(DWORD nPlayerId, LPSTR pszUnusedA, LPSTR pszUnusedB, DWORD nUnusedC, LPVOID pUnusedD);
unsigned int __cdecl JoinSession(HWND hDlg, unsigned int nMsg, unsigned int nParam, long lParam);
unsigned int __cdecl VerifyAssets(HWND hDlg, unsigned int nMsg, unsigned int nParam, long lParam);
void __cdecl         PopulateSelectionLists(HWND hDlg);
unsigned int __cdecl OnSelectionChanged(HWND hDlg, unsigned int nUnused, unsigned int nControl, long lParam);
unsigned int __cdecl HostSession(HWND hDlg, unsigned int nMsg, unsigned int nParam, long lParam);
int __cdecl          ValidatePlayerName(HWND hDlg, unsigned int nMsg, unsigned int nParam, long lParam);
unsigned int __cdecl SelectPlayerPosition(HWND hDlg, unsigned int nUnused, unsigned int nControl, long lParam);
unsigned int __cdecl ConfirmPlayerPosition(HWND hDlg, unsigned int nMsg, unsigned int nParam, long lParam);
unsigned int __cdecl SendChatMessage(HWND hDlg, unsigned int nMsg, unsigned int nParam, long lParam);
unsigned int __cdecl StartGame(HWND hDlg, unsigned int nMsg, unsigned int nParam, long lParam);
unsigned int __cdecl OnPlayerJoined(unsigned int nFlags, PlayerSlotName* pEntry);
unsigned int __cdecl RefreshSessionList(HWND hDlg);
BOOL __stdcall       AddEnumeratedSession(LPDPSESSIONDESC pDesc, LPVOID pUnusedA, LPDWORD pnUnusedB, DWORD nFlags);
unsigned int __cdecl OnChangeSlot(HWND hDlg);
int                  CheckPlayerReadyButton(int nPlayer);
int __cdecl          PopulateSessionList(HWND hDlg);

} // namespace NetSetupDialog

// GLOBAL: HYPERX 0x004c55dc
extern int g_aPlayerReadyButtonId[16];
// GLOBAL: HYPERX 0x004d0788
extern HWND g_hWndNetSetupDlg;

#endif
