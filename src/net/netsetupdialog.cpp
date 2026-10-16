#include "common/compat.h"
#include "globals.h"
#include "netglobals.h"
#include "netsetupstate.h"
#include "session.h"
#include "netsessionentry.h"
#include "ui/screens/dromeexteriorscreen.h"
#include "engine/gameengine.h"
#include "engine/objectclasses.h"
#include "engine/teamblock.h"
#include "graphics/meshbone.h"
#include "engine/teamsblock.h"
#include "net/gamesession.h"
#include <string.h>
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include "app/gameapp.h"
#include "cplayer.h"
#include "engine/level.h"
#include "net/netsetup.h"
#include "net/netsetupdialog.h"
#include "net/network.h"
#include "net/syncblock.h"
#include <cstdlib>

// One allocation holds both directions: a settings block and three catalogs each way.
union SyncBlockSlot
{
    SyncSettingsBlock settings;
    SyncCatalogBlock  catalog;
};

static_assert(sizeof(SyncBlockSlot) == SYNC_BLOCK_BYTES);

#define SYNC_SLOT_COUNT 8

// The number of providers the callback kept. Nothing to choose from is a failure.
// GLOBAL: HYPERX 0x004d078c
extern int g_nNetProviderCount;

// Modems are hidden unless enabled.
// FUNCTION: HYPERX 0x00475c70
BOOL __stdcall NetSetupDialog::AddProvider(LPGUID pGuid, LPSTR pszName, DWORD, DWORD, LPVOID pContext)
{
    const HWND hDlg = (HWND)pContext;

    // STRING: HYPERX 0x004c5664
    if (g_bModemPlay == 0 && strstr(pszName, "Modem") != 0)
    {
        return 1;
    }

    const LRESULT nItem = SendMessageA(GetDlgItem(hDlg, ID_PROVIDER_LIST), LB_ADDSTRING, 0, (LPARAM)pszName);

    SendMessageA(GetDlgItem(hDlg, ID_PROVIDER_LIST), LB_SETITEMDATA, nItem, (LPARAM)pGuid);

    g_nNetProviderCount++;

    return 1;
}

// FUNCTION: HYPERX 0x00475cf0
unsigned int __cdecl NetSetupDialog::EnumerateProviders(HWND hDlg)
{
    SetCursor(LoadCursor(0, IDC_WAIT));

    // STRING: HYPERX 0x004c5708
    NetSetup::DisplayChatMessage(-1, "SYSTEM: ...Creating DirectPlay and listing service providers", (int)strlen("SYSTEM: ...Creating DirectPlay and listing service providers"));

    if (g_nNetSetupState != NETSETUP_STATE_START)
    {
        const int nStateBeforeAbort = g_nNetSetupState;

        Network::AbortAndCleanup(hDlg);

        // STRING: HYPERX 0x004c56d8
        g_gameApp.Error("Trying to list providers in unknown state %d", nStateBeforeAbort);

        return 0;
    }

    // One block for every slot; GameSession::FreeAssetBuffer frees it through the first.
    SyncBlockSlot* const pSlots = (SyncBlockSlot*)std::malloc(SYNC_SLOT_COUNT * sizeof(SyncBlockSlot));

    g_syncSend.apBlocks[SYNC_BLOCK_SETTINGS] = &pSlots[0].settings;

    if (pSlots == 0)
    {
        Network::AbortAndCleanup(hDlg);

        // STRING: HYPERX 0x004c56a0
        g_gameApp.Error("Memory allocation for ASSET verification list failed\n");

        return 0;
    }

    g_syncSend.apBlocks[SYNC_BLOCK_TEAMS]    = &pSlots[1].catalog;
    g_syncSend.apBlocks[SYNC_BLOCK_ARENAS]   = &pSlots[2].catalog;
    g_syncSend.apBlocks[SYNC_BLOCK_PLANS]    = &pSlots[3].catalog;
    g_syncRecv.apBlocks[SYNC_BLOCK_SETTINGS] = &pSlots[4].settings;
    g_syncRecv.apBlocks[SYNC_BLOCK_TEAMS]    = &pSlots[5].catalog;
    g_syncRecv.apBlocks[SYNC_BLOCK_ARENAS]   = &pSlots[6].catalog;
    g_syncRecv.apBlocks[SYNC_BLOCK_PLANS]    = &pSlots[7].catalog;

    EnableWindow(GetDlgItem(hDlg, ID_PROVIDER_LIST), 1);
    EnableWindow(GetDlgItem(hDlg, ID_HOST), 0);
    EnableWindow(GetDlgItem(hDlg, ID_JOIN), 0);
    EnableWindow(GetDlgItem(hDlg, ID_SESSION_LIST), 0);

    SendMessageA(GetDlgItem(hDlg, ID_SESSION_LIST), LB_RESETCONTENT, 0, 0);
    SendDlgItemMessageA(hDlg, ID_SESSION_LIST, LB_SETCURSEL, (WPARAM)-1, 0);

    if (g_pGameSession != 0)
    {
        g_pGameSession = g_pGameSession->Destroy();

        g_pDPlayLobby = 0;
    }
    else if (g_pDPlayLobby != 0)
    {
        g_pDPlayLobby->Release();

        g_pDPlayLobby = 0;
    }

    g_nNetProviderCount = 0;

    if (DirectPlayEnumerateA(NetSetupDialog::AddProvider, hDlg) == 0 && g_nNetProviderCount > 0)
    {
        g_nNetSetupUpdating = 1;

        SendDlgItemMessageA(hDlg, ID_PROVIDER_LIST, LB_SETCURSEL, (WPARAM)-1, 0);

        SetFocus(GetDlgItem(hDlg, ID_PROVIDER_LIST));

        g_nNetSetupUpdating = 0;
        g_nNetSetupState    = NETSETUP_STATE_PROVIDER;

        // STRING: HYPERX 0x004c566c
        NetSetup::DisplayChatMessage(-1, "SYSTEM: Waiting for you to select a network to use", (int)strlen("SYSTEM: Waiting for you to select a network to use"));

        SetCursor(LoadCursor(0, IDC_ARROW));

        return 1;
    }

    Network::AbortAndCleanup(hDlg);

    // STRING: HYPERX 0x004c0e48
    g_gameApp.Error("Network connection failed.\nPlease try again.\n");

    return 0;
}

// The item data is read back from the list head, which is the entry just added.

// FUNCTION: HYPERX 0x00475f30
BOOL __stdcall NetSetupDialog::AddSessionEntry(LPDPSESSIONDESC pDesc, LPVOID pContext, LPDWORD, DWORD)
{
    const HWND hDlg = (HWND)pContext;

    LRESULT nItem;

    if (pDesc == 0)
    {
        return 0;
    }

    NetSessionEntry* const pEntry = (NetSessionEntry*)malloc(sizeof(NetSessionEntry));

    if (pEntry == 0)
    {
        // STRING: HYPERX 0x004c5748
        g_gameApp.Error("Malloc in NETSETUP failed");

        return 0;
    }

    memcpy(pEntry, pDesc, sizeof(DPSESSIONDESC));

    g_nSessionCount++;

    pEntry->m_bLive = 1;
    pEntry->m_pNext = g_pSessionList;

    g_pSessionList = pEntry;

    nItem = SendMessageA(GetDlgItem(hDlg, ID_SESSION_LIST), LB_ADDSTRING, 0, (LPARAM)pDesc->szSessionName);

    SendMessageA(GetDlgItem(hDlg, ID_SESSION_LIST), LB_SETITEMDATA, nItem, (LPARAM)g_pSessionList);

    return 1;
}

#define ENUM_TIMEOUT_MS 2000

// Closes anything open, then creates a fresh lobby from the chosen provider's GUID.
// FUNCTION: HYPERX 0x00475fe0
unsigned int __cdecl NetSetupDialog::SearchSessions(HWND hDlg, unsigned int, unsigned int, long)
{
    DPSESSIONDESC description;
    int           hr;

    SetCursor(LoadCursor(0, IDC_WAIT));

    // STRING: HYPERX 0x004c57d0
    NetSetup::DisplayChatMessage(-1, "SYSTEM: ...Searching the network for existing games.", (int)strlen("SYSTEM: ...Searching the network for existing games."));

    if (g_nNetSetupState != NETSETUP_STATE_PROVIDER)
    {
        const int nStateBeforeAbort = g_nNetSetupState;

        Network::AbortAndCleanup(hDlg);

        // STRING: HYPERX 0x004c57a4
        g_gameApp.Error("Trying to list sessions in unknown state %s", nStateBeforeAbort);

        return 0;
    }

    g_nNetSetupUpdating = 1;

    EnableWindow(GetDlgItem(hDlg, ID_PROVIDER_LIST), 0);
    EnableWindow(GetDlgItem(hDlg, ID_HOST), 0);
    EnableWindow(GetDlgItem(hDlg, ID_JOIN), 0);
    EnableWindow(GetDlgItem(hDlg, ID_SESSION_LIST), 0);

    if (g_pGameSession != 0)
    {
        g_pGameSession = g_pGameSession->Destroy();

        g_pDPlayLobby = 0;
    }
    else if (g_pDPlayLobby != 0)
    {
        g_pDPlayLobby->Release();

        g_pDPlayLobby = 0;
    }

    const LRESULT nItem = SendMessageA(GetDlgItem(hDlg, ID_PROVIDER_LIST), LB_GETCURSEL, 0, 0);
    GUID* const   pGuid = (GUID*)SendMessageA(GetDlgItem(hDlg, ID_PROVIDER_LIST), LB_GETITEMDATA, nItem, 0);

    if (DirectPlayCreate(pGuid, &g_pDPlayLobby, 0) != DP_OK)
    {
        Network::AbortAndCleanup(hDlg);

        // STRING: HYPERX 0x004c0e48
        g_gameApp.Error("Network connection failed.\nPlease try again.\n");

        return 0;
    }

    memset(&description, 0, sizeof(description));

    description.dwSize      = sizeof(description);
    description.guidSession = g_guidNetApplication;

    g_nNetSetupUpdating = 1;

    hr = g_pDPlayLobby->EnumSessions(&description, ENUM_TIMEOUT_MS, NetSetupDialog::AddSessionEntry, hDlg, DPENUMSESSIONS_AVAILABLE);

    g_nNetSetupUpdating = 0;

    if (hr != 0 && hr != DPERR_NOSESSIONS)
    {
        Network::AbortAndCleanup(hDlg);

        // STRING: HYPERX 0x004c0e48
        g_gameApp.Error("Network connection failed.\nPlease try again.\n");

        return 0;
    }

    EnableWindow(GetDlgItem(hDlg, ID_SEARCH), 1);

    g_nNetSetupState = NETSETUP_STATE_BROWSING;

    // STRING: HYPERX 0x004c5764
    NetSetup::DisplayChatMessage(-1, "SYSTEM: Waiting for you to JOIN a game, or to HOST a new one", (int)strlen("SYSTEM: Waiting for you to JOIN a game, or to HOST a new one"));

    SetCursor(LoadCursor(0, IDC_ARROW));

    EnableWindow(GetDlgItem(hDlg, ID_HOST), 1);
    EnableWindow(GetDlgItem(hDlg, ID_SESSION_LIST), 1);

    g_nNetSetupUpdating = 0;

    return 1;
}

// The session the join reads, taken from the selected item's data.
// GLOBAL: HYPERX 0x004d0778
extern DPSESSIONDESC* g_pSelectedSession;

// An empty selection restores the dialog and succeeds.
// FUNCTION: HYPERX 0x00476210
unsigned int __cdecl NetSetupDialog::SelectSession(HWND hDlg, unsigned int, unsigned int, long)
{
    if (g_nNetSetupState != NETSETUP_STATE_BROWSING)
    {
        Network::AbortAndCleanup(hDlg);

        // STRING: HYPERX 0x004c5808
        g_gameApp.Error("Trying to JOIN session in unknown state %d", g_nNetSetupState);

        return 0;
    }

    g_nNetSetupUpdating = 1;

    const int nItem = (int)SendMessageA(GetDlgItem(hDlg, ID_SESSION_LIST), LB_GETCURSEL, 0, 0);

    if (nItem == LB_ERR)
    {
        g_nNetSetupUpdating = 0;

        return 1;
    }

    g_nNetSetupState   = NETSETUP_STATE_SELECTED;
    g_nNetSetupTimerId = 0;

    EnableWindow(GetDlgItem(hDlg, ID_SESSION_LIST), 0);
    EnableWindow(GetDlgItem(hDlg, ID_SEARCH), 0);

    g_pSelectedSession = (DPSESSIONDESC*)SendMessageA(GetDlgItem(hDlg, ID_SESSION_LIST), LB_GETITEMDATA, nItem, 0);

    EnableWindow(GetDlgItem(hDlg, ID_JOIN), 1);

    g_nNetSetupUpdating = 0;

    return 1;
}

// Reads only its first argument and answers nought or one.

// FUNCTION: HYPERX 0x00476300
BOOL __stdcall NetSetupDialog::OnEnumeratedPlayer(DWORD nPlayerId, LPSTR, LPSTR, DWORD, LPVOID)
{
    return Network::AddDPID(g_pGameSession, nPlayerId) != 0;
}

// The cursor stays an hourglass until VerifyAssets resets it.
// FUNCTION: HYPERX 0x00476320
unsigned int __cdecl NetSetupDialog::JoinSession(HWND hDlg, unsigned int, unsigned int, long)
{
    const int nState = g_nNetSetupState;

    if (nState != NETSETUP_STATE_SELECTED)
    {
        Network::AbortAndCleanup(hDlg);

        // STRING: HYPERX 0x004c5808
        g_gameApp.Error("Trying to JOIN session in unknown state %d", nState);

        return 0;
    }

    if (g_pSelectedSession == 0)
    {
        Network::AbortAndCleanup(hDlg);

        // STRING: HYPERX 0x004c5980
        g_gameApp.Error("%s(Selected game has already started without you)",
                        // STRING: HYPERX 0x004c0e48
                        "Network connection failed.\nPlease try again.\n");

        return 0;
    }

    g_nNetSetupUpdating = 1;

    EnableWindow(GetDlgItem(hDlg, ID_TEAM_AWAY), 0);
    EnableWindow(GetDlgItem(hDlg, ID_TEAM_HOME), 0);
    EnableWindow(GetDlgItem(hDlg, ID_ARENA), 0);

    NetSetupDialog::PopulateSelectionLists(hDlg);

    EnableWindow(GetDlgItem(hDlg, ID_POSITION_FIRST), 1);
    EnableWindow(GetDlgItem(hDlg, ID_POSITION_THIRD), 1);
    EnableWindow(GetDlgItem(hDlg, ID_POSITION_SECOND), 1);
    EnableWindow(GetDlgItem(hDlg, ID_POSITION_FOURTH), 1);
    EnableWindow(GetDlgItem(hDlg, ID_START), 0);

    if (g_pDPlayLobby == 0)
    {
        Network::AbortAndCleanup(hDlg);

        // STRING: HYPERX 0x004c594c
        g_gameApp.Error("Trying to JOIN session, without DirectPlay object");

        return 0;
    }

    if (g_pGameSession != 0)
    {
        Network::AbortAndCleanup(hDlg);

        // STRING: HYPERX 0x004c5918
        g_gameApp.Error("Trying to JOIN session, with session already open");

        return 0;
    }

    g_pGameSession = new GameSession(hDlg, g_pDPlayLobby);

    if (g_pGameSession == 0 || g_pGameSession->apPlayers[0] == 0)
    {
        Network::AbortAndCleanup(hDlg);

        // STRING: HYPERX 0x004c5834
        g_gameApp.Error("Memory allocation (new) for dpNet failed");

        return 0;
    }

    SetCursor(LoadCursor(0, IDC_WAIT));

    g_nNetSetupUpdating = 1;

    // STRING: HYPERX 0x004c58e0
    NetSetup::DisplayChatMessage(-1, "SYSTEM: ...Searching for players already in this game", (int)strlen("SYSTEM: ...Searching for players already in this game"));

    if (g_pGameSession->OpenSession(DPOPEN_JOIN, g_pSelectedSession) == 0)
    {
        Network::AbortAndCleanup(hDlg);

        return 0;
    }

    if (g_pDPlayLobby->EnumPlayers(g_pSelectedSession->dwSession,
                                   NetSetupDialog::OnEnumeratedPlayer,
                                   hDlg,
                                   0) != 0)
    {
        Network::AbortAndCleanup(hDlg);

        // STRING: HYPERX 0x004c0e48
        g_gameApp.Error("Network connection failed.\nPlease try again.\n");

        return 0;
    }

    if (g_pGameSession->CreatePacketCarrier() == 0)
    {
        return Network::AbortAndCleanup(hDlg);
    }

    EnableWindow(GetDlgItem(hDlg, ID_CHAT_ENTRY), 1);

    SendMessageA(GetDlgItem(hDlg, ID_CHAT_ENTRY), WM_SETTEXT, 0, (LPARAM) "");

    g_nNetSetupState = NETSETUP_STATE_JOINING;

    // STRING: HYPERX 0x004c589c
    NetSetup::DisplayChatMessage(-1, "SYSTEM: ...Wait, getting arena and team information from the HOST", (int)strlen("SYSTEM: ...Wait, getting arena and team information from the HOST"));

    // STRING: HYPERX 0x004c5860
    NetSetup::DisplayChatMessage(-1, "SYSTEM: ...Wait, loading models for this arena and teams", (int)strlen("SYSTEM: ...Wait, loading models for this arena and teams"));

    g_pGameSession->nAssetsArrived = 0;

    g_pGameSession->apPlayers[SESSION_LOCAL_PLAYER]->SendAssetRequest(DPID_ALLPLAYERS);

    g_nNetSetupUpdating = 0;

    return 1;
}

// Past the three reported refusals, a failed check tears the connection down silently.
// FUNCTION: HYPERX 0x00476660
unsigned int __cdecl NetSetupDialog::VerifyAssets(HWND hDlg, unsigned int, unsigned int, long)
{
    if (g_nNetSetupState != NETSETUP_STATE_VERIFYING)
    {
        Network::AbortAndCleanup(hDlg);

        // STRING: HYPERX 0x004c5a50
        g_gameApp.Error("Trying to VERIFY assets in unknown state");

        return 0;
    }

    if (g_pGameSession != 0 && g_pGameSession->apPlayers[0] != 0)
    {
        if ((g_pGameSession->nAssetsArrived & SYNC_BLOCKS_ALL) != SYNC_BLOCKS_ALL)
        {
            Network::AbortAndCleanup(hDlg);

            // STRING: HYPERX 0x004c5a20
            g_gameApp.Error("Trying to VERIFY before asset info was received");

            return 0;
        }

        g_nNetSetupUpdating = 1;

        SetCursor(LoadCursor(0, IDC_WAIT));

        if (GameSession::BuildSyncPacket(&g_syncSend, &g_syncRecv) != 0)
        {
            if (GameSession::ValidateAndApplyAssets(&g_syncSend, &g_syncRecv) != 0)
            {
                g_pGameEngine->SetTeams((short)g_nDromeTeamHome,
                                        (short)g_nDromeTeamAway);
                g_pGameEngine->SetArena((short)g_nDromeArena);

                NetSetupDialog::PopulateSelectionLists(hDlg);

                if (Level::IndexStationaryObjects() != 0)
                {
                    if (Level::ValidateArenaObjects(&g_syncSend, &g_syncRecv) != 0)
                    {
                        g_nNetSetupState = NETSETUP_STATE_NAMED;

                        // STRING: HYPERX 0x004c59dc
                        NetSetup::DisplayChatMessage(-1, "SYSTEM: Waiting for you to name a player (in any available slot)", (int)strlen("SYSTEM: Waiting for you to name a player (in any available slot)"));

                        SetCursor(LoadCursor(0, IDC_ARROW));

                        if (NetSetupDialog::OnChangeSlot(hDlg) == 0)
                        {
                            return Network::AbortAndCleanup(hDlg);
                        }

                        g_nNetSetupUpdating = 0;

                        return 1;
                    }
                }

                return Network::AbortAndCleanup(hDlg);
            }
        }

        return Network::AbortAndCleanup(hDlg);
    }

    Network::AbortAndCleanup(hDlg);

    // STRING: HYPERX 0x004c59b4
    g_gameApp.Error("Trying to VERIFY with no dpNet object");

    return 0;
}

#define ID_LABEL_HOME 0x4c5
#define ID_LABEL_AWAY 0x4d0

// The catalogue SetTeams and SetArena index: entries, a short count at 4, names at 0x20.
namespace NetSetupDialog
{

static __inline char* TeamCatalogueName(short nIndex);

}

__inline char* NetSetupDialog::TeamCatalogueName(short nIndex)
{
    TeamsBlock* const pTable = g_pGameEngine->m_pTeamTable;

    if (nIndex < pTable->GetTeamCount())
    {
        return pTable->m_ppTeams[nIndex]->szName;
    }

    return 0;
}

// The away box skips the home selection and the home box the away one.
// FUNCTION: HYPERX 0x00476820
void __cdecl NetSetupDialog::PopulateSelectionLists(HWND hDlg)
{
    LRESULT nItem;

    SendMessageA(GetDlgItem(hDlg, ID_TEAM_HOME), CB_RESETCONTENT, 0, 0);
    SendDlgItemMessageA(hDlg, ID_TEAM_HOME, CB_SETCURSEL, (WPARAM)-1, 0);

    SendMessageA(GetDlgItem(hDlg, ID_TEAM_AWAY), CB_RESETCONTENT, 0, 0);
    SendDlgItemMessageA(hDlg, ID_TEAM_AWAY, CB_SETCURSEL, (WPARAM)-1, 0);

    SendMessageA(GetDlgItem(hDlg, ID_ARENA), CB_RESETCONTENT, 0, 0);
    SendDlgItemMessageA(hDlg, ID_ARENA, CB_SETCURSEL, (WPARAM)-1, 0);

    for (int nIndex = 0; nIndex < g_pGameEngine->m_pTeamTable->GetTeamCount(); nIndex++)
    {
        if (nIndex != g_nDromeTeamAway)
        {
            nItem = SendMessageA(GetDlgItem(hDlg, ID_TEAM_HOME), CB_ADDSTRING, 0, (LPARAM)TeamCatalogueName((short)nIndex));

            SendMessageA(GetDlgItem(hDlg, ID_TEAM_HOME), CB_SETITEMDATA, nItem, nIndex);

            if (nIndex == g_nDromeTeamHome)
            {
                SendDlgItemMessageA(hDlg, ID_TEAM_HOME, CB_SETCURSEL, nItem, 0);
            }
        }

        if (nIndex != g_nDromeTeamHome)
        {
            nItem = SendMessageA(GetDlgItem(hDlg, ID_TEAM_AWAY), CB_ADDSTRING, 0, (LPARAM)TeamCatalogueName((short)nIndex));

            SendMessageA(GetDlgItem(hDlg, ID_TEAM_AWAY), CB_SETITEMDATA, nItem, nIndex);

            if (nIndex == g_nDromeTeamAway)
            {
                SendDlgItemMessageA(hDlg, ID_TEAM_AWAY, CB_SETCURSEL, nItem, 0);
            }
        }

        nItem = SendMessageA(GetDlgItem(hDlg, ID_ARENA), CB_ADDSTRING, 0, (LPARAM)TeamCatalogueName((short)nIndex));

        SendMessageA(GetDlgItem(hDlg, ID_ARENA), CB_SETITEMDATA, nItem, nIndex);

        if (nIndex == g_nDromeArena)
        {
            SendDlgItemMessageA(hDlg, ID_ARENA, CB_SETCURSEL, nItem, 0);
        }
    }

    SendMessageA(GetDlgItem(hDlg, ID_LABEL_HOME), WM_SETTEXT, 0, (LPARAM)TeamCatalogueName((short)g_nDromeTeamHome));

    SendMessageA(GetDlgItem(hDlg, ID_LABEL_AWAY), WM_SETTEXT, 0, (LPARAM)TeamCatalogueName((short)g_nDromeTeamAway));
}

// Two teams the same is refused after the store, so the selection is on record.
// FUNCTION: HYPERX 0x00476a90
unsigned int __cdecl NetSetupDialog::OnSelectionChanged(HWND hDlg, unsigned int, unsigned int nControl, long)
{
    if (g_nNetSetupState != NETSETUP_STATE_POSITION)
    {
        Network::AbortAndCleanup(hDlg);

        // STRING: HYPERX 0x004c5b10
        g_gameApp.Error("Trying to change teams or arena in unknown state");

        return 0;
    }

    g_nNetSetupUpdating = 1;

    int nItem = (int)SendMessageA(GetDlgItem(hDlg, LOWORD(nControl)), CB_GETCURSEL, 0, 0);
    nItem     = (int)SendMessageA(GetDlgItem(hDlg, LOWORD(nControl)), CB_GETITEMDATA, nItem, 0);

    if (nItem >= 0 && nItem < g_pGameEngine->m_pTeamTable->GetTeamCount())
    {
        if ((short)nControl == ID_TEAM_HOME)
        {
            g_nDromeTeamHome = nItem;
        }
        else if ((short)nControl == ID_TEAM_AWAY)
        {
            g_nDromeTeamAway = nItem;
        }
        else if ((short)nControl == ID_ARENA)
        {
            g_nDromeArena = nItem;
        }
        else
        {
            Network::AbortAndCleanup(hDlg);

            // STRING: HYPERX 0x004c5ab4
            g_gameApp.Error("Trying to change teams or arena with invalid IDC");

            return 0;
        }

        if (g_nDromeTeamHome == g_nDromeTeamAway)
        {
            Network::AbortAndCleanup(hDlg);

            // STRING: HYPERX 0x004c5ae8
            g_gameApp.Error("Trying to make HOME and AWAY teams same");

            return 0;
        }

        NetSetupDialog::PopulateSelectionLists(hDlg);

        g_nNetSetupUpdating = 0;

        return 1;
    }

    Network::AbortAndCleanup(hDlg);

    // STRING: HYPERX 0x004c5a7c
    g_gameApp.Error("Trying to change teams or arena with index out of range");

    return 0;
}

// The dialog changes shape before the refusals are checked.

// FUNCTION: HYPERX 0x00476bd0
unsigned int __cdecl NetSetupDialog::HostSession(HWND hDlg, unsigned int, unsigned int, long)
{
    if (g_nNetSetupState != NETSETUP_STATE_POSITION)
    {
        Network::AbortAndCleanup(hDlg);

        // STRING: HYPERX 0x004c5bd8
        g_gameApp.Error("Trying to HOST session in unknown state");

        return 0;
    }

    g_nNetSetupUpdating = 1;

    EnableWindow(GetDlgItem(hDlg, ID_POSITION_FIRST), 1);
    EnableWindow(GetDlgItem(hDlg, ID_POSITION_THIRD), 1);
    EnableWindow(GetDlgItem(hDlg, ID_POSITION_SECOND), 1);
    EnableWindow(GetDlgItem(hDlg, ID_POSITION_FOURTH), 1);
    EnableWindow(GetDlgItem(hDlg, ID_CHAT_ENTRY), 0);

    SendMessageA(GetDlgItem(hDlg, ID_CHAT_ENTRY), WM_SETTEXT, 0, (LPARAM) "");

    NetSetupDialog::PopulateSelectionLists(hDlg);

    if (g_pDPlayLobby == 0)
    {
        Network::AbortAndCleanup(hDlg);

        // STRING: HYPERX 0x004c5ba4
        g_gameApp.Error("Trying to HOST session, without DirectPlay object");

        return 0;
    }

    if (g_pGameSession != 0)
    {
        Network::AbortAndCleanup(hDlg);

        // STRING: HYPERX 0x004c5b70
        g_gameApp.Error("Trying to HOST session, with session already open");

        return 0;
    }

    g_pGameSession = new GameSession(hDlg, g_pDPlayLobby);

    if (g_pGameSession != 0 && g_pGameSession->apPlayers[0] != 0)
    {
        g_nNetSetupState = NETSETUP_STATE_POSITION;

        // STRING: HYPERX 0x004c59dc
        NetSetup::DisplayChatMessage(-1, "SYSTEM: Waiting for you to name a player (in any available slot)", (int)strlen("SYSTEM: Waiting for you to name a player (in any available slot)"));

        SetCursor(LoadCursor(0, IDC_ARROW));

        SetFocus(GetDlgItem(hDlg, ID_POSITION_FIRST));

        g_nNetSetupUpdating = 0;

        return 1;
    }

    Network::AbortAndCleanup(hDlg);

    // STRING: HYPERX 0x004c5b44
    g_gameApp.Error("Memory allocation (new) failed for dpNet");

    return 0;
}

// The updating flag is restored rather than cleared, except that an unknown control returns with it set.

// FUNCTION: HYPERX 0x00476da0
int __cdecl NetSetupDialog::ValidatePlayerName(HWND hDlg, unsigned int, unsigned int, long)
{
    char szName[62];
    int  nLength;

    if (g_nNetSetupState != NETSETUP_STATE_POSITION && g_nNetSetupState != NETSETUP_STATE_NAMED)
    {
        return 0;
    }

    if (g_pDPlayLobby == 0)
    {
        return 0;
    }

    if (g_pGameSession == 0)
    {
        return 0;
    }

    const int nId = g_nNetSetupControlId;

    if (nId == 0)
    {
        return 0;
    }

    const int nWasUpdating = g_nNetSetupUpdating;

    g_nNetSetupUpdating = 1;

    switch (nId)
    {
        case ID_POSITION_FIRST:
            break;

        case ID_POSITION_SECOND:
            break;

        case ID_POSITION_THIRD:
            break;

        case ID_POSITION_FOURTH:
            break;

        default:
            return 0;
    }

    nLength = GetDlgItemTextA(hDlg, nId, szName, sizeof(szName));

    g_nNetSetupUpdating = nWasUpdating;

    return nLength >= 1;
}

#define NAME_READ 0x3e

// Controls 0x4c1 to 0x4c4 select slots 2, 1, 4 and 3: home and away pair down the column.
// FUNCTION: HYPERX 0x00476e30
unsigned int __cdecl NetSetupDialog::SelectPlayerPosition(HWND hDlg, unsigned int, unsigned int nControl, long)
{
    int nLength;

    // Only ConfirmPlayerPosition fills the slot byte.
    PlayerSlotName entry;

    int nState = g_nNetSetupState;

    if (nState != NETSETUP_STATE_POSITION && nState != NETSETUP_STATE_NAMED)
    {
        Network::AbortAndCleanup(hDlg);

        // STRING: HYPERX 0x004c5c58
        g_gameApp.Error("%s(Cannot set player position in state %d)",
                        // STRING: HYPERX 0x004c0e48
                        "Network connection failed.\nPlease try again.\n",
                        nState);

        return 0;
    }

    if (g_pDPlayLobby != 0 && g_pGameSession != 0)
    {
        g_nNetSetupControlId = LOWORD(nControl);
        g_nNetSetupUpdating  = 1;

        switch (g_nNetSetupControlId)
        {
            case ID_POSITION_FIRST:
                g_nNetSetupPendingSlot = 2;
                nLength                = (int)GetDlgItemTextA(hDlg, g_nNetSetupControlId, entry.szName, NAME_READ);
                break;

            case ID_POSITION_THIRD:
                g_nNetSetupPendingSlot = 4;
                nLength                = (int)GetDlgItemTextA(hDlg, g_nNetSetupControlId, entry.szName, NAME_READ);
                break;

            case ID_POSITION_SECOND:
                g_nNetSetupPendingSlot = 1;
                nLength                = (int)GetDlgItemTextA(hDlg, g_nNetSetupControlId, entry.szName, NAME_READ);
                break;

            case ID_POSITION_FOURTH:
                g_nNetSetupPendingSlot = 3;
                nLength                = (int)GetDlgItemTextA(hDlg, g_nNetSetupControlId, entry.szName, NAME_READ);
                break;

            default:
                Network::AbortAndCleanup(hDlg);

                // STRING: HYPERX 0x004c5c30
                g_gameApp.Error("Invalid player position in net setup");

                return 0;
        }

        g_nNetSetupUpdating = 0;

        if (nLength < 1)
        {
            EnableWindow(GetDlgItem(hDlg, ID_READY), 0);

            return 1;
        }

        g_nNetSetupUpdating = 1;

        if (g_nNetSetupPendingSlot != 2)
        {
            EnableWindow(GetDlgItem(hDlg, ID_POSITION_FIRST), 0);
        }

        if (g_nNetSetupPendingSlot != 4)
        {
            EnableWindow(GetDlgItem(hDlg, ID_POSITION_THIRD), 0);
        }

        if (g_nNetSetupPendingSlot != 1)
        {
            EnableWindow(GetDlgItem(hDlg, ID_POSITION_SECOND), 0);
        }

        if (g_nNetSetupPendingSlot != 3)
        {
            EnableWindow(GetDlgItem(hDlg, ID_POSITION_FOURTH), 0);
        }

        EnableWindow(GetDlgItem(hDlg, ID_READY), 1);

        g_nNetSetupUpdating = 0;

        return 1;
    }

    Network::AbortAndCleanup(hDlg);

    // STRING: HYPERX 0x004c5c00
    g_gameApp.Error("Trying to set player, without DirectPlay object");

    return 0;
}

#define NAME_ROOM 0x1f

#define ROLE_HOST (PLAYER_NAME_HOST | PLAYER_NAME_HUMAN | PLAYER_NAME_LOCAL)
#define ROLE_JOIN (PLAYER_NAME_HUMAN | PLAYER_NAME_LOCAL)

// FUNCTION: HYPERX 0x00477000
unsigned int __cdecl NetSetupDialog::ConfirmPlayerPosition(HWND hDlg, unsigned int, unsigned int, long)
{
    int            nLength;
    PlayerSlotName entry;
    DPSESSIONDESC  msg;

    if (g_nNetSetupState != NETSETUP_STATE_POSITION && g_nNetSetupState != NETSETUP_STATE_NAMED)
    {
        const int nStateBeforeAbort = g_nNetSetupState;

        Network::AbortAndCleanup(hDlg);

        // STRING: HYPERX 0x004c5d90
        g_gameApp.Error("%s(Cannot set player position in state %d",
                        // STRING: HYPERX 0x004c0e48
                        "Network connection failed.\nPlease try again.\n",
                        nStateBeforeAbort);

        return 0;
    }

    if (g_pDPlayLobby != 0 && g_pGameSession != 0)
    {
        int nSlot           = g_nNetSetupPendingSlot;
        g_nNetSetupUpdating = 1;

        if (nSlot == 2)
        {
            nLength = (int)GetDlgItemTextA(hDlg, ID_POSITION_FIRST, entry.szName, NAME_READ);
        }
        else if (nSlot == 4)
        {
            nLength = (int)GetDlgItemTextA(hDlg, ID_POSITION_THIRD, entry.szName, NAME_READ);
        }
        else if (nSlot == 1)
        {
            nLength = (int)GetDlgItemTextA(hDlg, ID_POSITION_SECOND, entry.szName, NAME_READ);
        }
        else if (nSlot == 3)
        {
            nLength = (int)GetDlgItemTextA(hDlg, ID_POSITION_FOURTH, entry.szName, NAME_READ);
        }
        else
        {
            Network::AbortAndCleanup(hDlg);

            // STRING: HYPERX 0x004c5c84
            g_gameApp.Error("Invalid player position or name, try again");

            return 0;
        }

        if (nLength < 1)
        {
            g_nNetSetupUpdating = 0;

            return 1;
        }

        g_nNetSetupPendingSlot = 0;

        EnableWindow(GetDlgItem(hDlg, ID_POSITION_FIRST), 0);
        EnableWindow(GetDlgItem(hDlg, ID_POSITION_THIRD), 0);
        EnableWindow(GetDlgItem(hDlg, ID_POSITION_SECOND), 0);
        EnableWindow(GetDlgItem(hDlg, ID_POSITION_FOURTH), 0);
        EnableWindow(GetDlgItem(hDlg, ID_READY), 0);

        if (g_nNetSetupState == NETSETUP_STATE_POSITION)
        {
            EnableWindow(GetDlgItem(hDlg, ID_TEAM_HOME), 0);
            EnableWindow(GetDlgItem(hDlg, ID_TEAM_AWAY), 0);
            EnableWindow(GetDlgItem(hDlg, ID_ARENA), 0);

            SetCursor(LoadCursor(0, IDC_WAIT));

            // STRING: HYPERX 0x004c5860
            NetSetup::DisplayChatMessage(-1, "SYSTEM: ...Wait, loading models for this arena and teams", (int)strlen("SYSTEM: ...Wait, loading models for this arena and teams"));

            g_pGameEngine->SetTeams((short)g_nDromeTeamHome, (short)g_nDromeTeamAway);
            g_pGameEngine->SetArena((short)g_nDromeArena);

            // STRING: HYPERX 0x004c5d68
            NetSetup::DisplayChatMessage(-1, "SYSTEM: ...Wait, building asset list", (int)strlen("SYSTEM: ...Wait, building asset list"));

            if (Level::IndexStationaryObjects() == 0)
            {
                return Network::AbortAndCleanup(hDlg);
            }

            if (GameSession::BuildSyncPacket(&g_syncSend, &g_syncRecv) == 0)
            {
                return Network::AbortAndCleanup(hDlg);
            }

            if (g_pDPlayLobby != 0 && g_pGameSession != 0)
            {
                memset(&msg, 0, sizeof(msg));

                msg.dwSize = sizeof(msg);

                strncpy(msg.szSessionName, entry.szName, NAME_ROOM);

                // STRING: HYPERX 0x004c5d60
                if (strlen(msg.szSessionName) < NAME_ROOM - strlen("'s Game"))
                {
                    strcat(msg.szSessionName, "'s Game");
                }

                if (g_pGameSession->OpenSession(DPOPEN_CREATE, &msg) == 0)
                {
                    return Network::AbortAndCleanup(hDlg);
                }

                nSlot |= ROLE_HOST;
                entry.cSlotFlags = (char)nSlot;

                if (g_pGameSession->AssignPlayerSlot(nSlot, &entry) == 0)
                {
                    return Network::AbortAndCleanup(hDlg);
                }

                g_pGameSession->pLeadPlayer->pState->nActive = 1;

                if (g_pGameSession->CreatePacketCarrier() == 0)
                {
                    return Network::AbortAndCleanup(hDlg);
                }

                EnableWindow(GetDlgItem(hDlg, ID_CHAT_ENTRY), 1);

                SendMessageA(GetDlgItem(hDlg, ID_CHAT_ENTRY), WM_SETTEXT, 0, (LPARAM) "");

                // The name reads oddly for this use: the error table also writes this address.
                g_nDPlayLastError = 1;
                g_nNetSetupState  = NETSETUP_STATE_VERIFY;

                // STRING: HYPERX 0x004c5d2c
                NetSetup::DisplayChatMessage(-1, "SYSTEM: Waiting for remote players to JOIN the game", (int)strlen("SYSTEM: Waiting for remote players to JOIN the game"));

                SetFocus(GetDlgItem(hDlg, ID_CHAT_ENTRY));

                g_nNetSetupUpdating = 0;

                return 1;
            }

            Network::AbortAndCleanup(hDlg);

            // STRING: HYPERX 0x004c5ba4
            g_gameApp.Error("Trying to HOST session, without DirectPlay object");

            return 0;
        }

        if (g_nNetSetupState == NETSETUP_STATE_NAMED)
        {
            if (g_pDPlayLobby != 0 && g_pGameSession != 0 && g_pGameSession->apPlayers[0] != 0)
            {
                nSlot |= ROLE_JOIN;
                entry.cSlotFlags = (char)nSlot;

                strcpy(g_pGameSession->apPlayers[SESSION_LOCAL_PLAYER]->pState->slotName.GetWireName(), entry.GetWireName());

                g_pGameSession->apPlayers[SESSION_LOCAL_PLAYER]->pState->nSlotFlags = nSlot;

                g_nNetSetupState = NETSETUP_STATE_ACCEPTING;

                // STRING: HYPERX 0x004c5cec
                NetSetup::DisplayChatMessage(-1, "SYSTEM: ...Waiting for HOST to ACCEPT or REJECT your position", (int)strlen("SYSTEM: ...Waiting for HOST to ACCEPT or REJECT your position"));

                SetFocus(GetDlgItem(hDlg, ID_CHAT_ENTRY));

                g_nNetSetupUpdating = 0;

                g_pGameSession->nAssetsArrived = 0;

                for (int nIndex = 0; nIndex < g_pGameSession->nRemoteCount; nIndex++)
                {
                    g_pGameSession->apPlayers[SESSION_LOCAL_PLAYER]->SendPlayerConnected(
                        g_pGameSession->aRemoteIds[nIndex], nSlot & PLAYER_NAME_SLOT_MASK);
                }

                return 1;
            }

            Network::AbortAndCleanup(hDlg);

            // STRING: HYPERX 0x004c5cb0
            g_gameApp.Error("Trying to JOIN NULL session (should have been open by now");

            return 0;
        }

        g_nNetSetupUpdating = 0;

        return 0;
    }

    Network::AbortAndCleanup(hDlg);

    // STRING: HYPERX 0x004c5c00
    g_gameApp.Error("Trying to set player, without DirectPlay object");

    return 0;
}

// Set when the chat entry takes focus and cleared by every send.
// GLOBAL: HYPERX 0x004d07a0
extern int g_bChatEntryActive;

#define CHAT_ROOM    0x7f
#define CHAT_HISTORY 3

// The chat box keeps four lines; a nameless pending player sends as "????".
// FUNCTION: HYPERX 0x00477550
unsigned int __cdecl NetSetupDialog::SendChatMessage(HWND hDlg, unsigned int, unsigned int, long)
{
    char         szLine[128];
    PlayerState* pState;

    if (g_pGameSession == 0 || g_pGameSession->apPlayers[SESSION_LOCAL_PLAYER] == 0 || g_pGameSession->apPlayers[SESSION_LOCAL_PLAYER]->pState == 0 || g_pGameSession->apPlayers[SESSION_LOCAL_PLAYER]->pState->nPlayerId == DPID_UNKNOWN)
    {
        // STRING: HYPERX 0x004c5dbc
        g_gameApp.Error("Trying to send CHAT message before DP Player was created\n");

        g_bChatEntryActive = 0;

        return 1;
    }

    if (g_pGameSession->pLeadPlayer != 0 && g_pGameSession->pLeadPlayer->pState != 0 && g_pGameSession->pLeadPlayer->pState->slotName.cSlotFlags != 0)
    {
        pState = g_pGameSession->pLeadPlayer->pState;

        strcpy(szLine, pState->slotName.szName);

        // STRING: HYPERX 0x004c5e00
        strcat(szLine, ": ");
    }
    else
    {
        // The original also passed the player ID, which this format never reads.
        // STRING: HYPERX 0x004c5df8
        sprintf(szLine, "????: ");
    }

    g_bChatEntryActive  = 0;
    g_nNetSetupUpdating = 1;

    if (GetDlgItemTextA(hDlg, ID_CHAT_ENTRY, &szLine[strlen(szLine)], (int)(CHAT_ROOM - strlen(szLine))) == 0)
    {
        g_nNetSetupUpdating = 0;

        return 1;
    }

    const int nLength = (int)strlen(szLine);

    SendMessageA(GetDlgItem(hDlg, ID_CHAT_ENTRY), WM_SETTEXT, 0, (LPARAM) "");

    while (SendMessageA(GetDlgItem(hDlg, ID_CHAT_LIST), LB_GETCOUNT, 0, 0) > CHAT_HISTORY)
    {
        SendMessageA(GetDlgItem(hDlg, ID_CHAT_LIST), LB_DELETESTRING, 0, 0);
    }

    SendMessageA(GetDlgItem(hDlg, ID_CHAT_LIST), LB_ADDSTRING, 0, (LPARAM)szLine);

    for (int nIndex = 0; nIndex < g_pGameSession->nRemoteCount; nIndex++)
    {
        g_pGameSession->apPlayers[SESSION_LOCAL_PLAYER]->SendDataMsg(
            g_pGameSession->aRemoteIds[nIndex], szLine, nLength);
    }

    SetFocus(GetDlgItem(hDlg, ID_CHAT_ENTRY));

    g_nNetSetupUpdating = 0;

    return 1;
}

// The two waiting messages differ by three dots, which shows which side you are on.
// FUNCTION: HYPERX 0x00477790
unsigned int __cdecl NetSetupDialog::StartGame(HWND hDlg, unsigned int, unsigned int, long)
{
    int nPoll;

    if (g_nNetSetupState != NETSETUP_STATE_VERIFY)
    {
        Network::AbortAndCleanup(hDlg);

        // STRING: HYPERX 0x004c5edc
        g_gameApp.Error("Trying start a game in unknown state");

        return 0;
    }

    g_nNetSetupUpdating = 1;

    EnableWindow(GetDlgItem(hDlg, ID_POSITION_FIRST), 0);
    EnableWindow(GetDlgItem(hDlg, ID_POSITION_THIRD), 0);
    EnableWindow(GetDlgItem(hDlg, ID_POSITION_SECOND), 0);
    EnableWindow(GetDlgItem(hDlg, ID_POSITION_FOURTH), 0);
    EnableWindow(GetDlgItem(hDlg, ID_START), 1);
    EnableWindow(GetDlgItem(hDlg, ID_CHAT_ENTRY), 1);
    EnableWindow(GetDlgItem(hDlg, IDCANCEL), 1);

    GameSession* pSession = g_pGameSession;

    if (pSession != 0 && g_pDPlayLobby != 0 && pSession->apPlayers[7] != 0 && pSession->apPlayers[8] != 0 && pSession->apPlayers[9] != 0 && pSession->apPlayers[10] != 0 && pSession->apPlayers[2] != 0 && pSession->apPlayers[4] != 0 && pSession->apPlayers[6] != 0 && pSession->apPlayers[1] != 0 && pSession->apPlayers[3] != 0 && pSession->apPlayers[5] != 0)
    {
        if (pSession->apPlayers[1]->pState->nSlotFlags == 0 && pSession->apPlayers[3]->pState->nSlotFlags == 0 && pSession->apPlayers[2]->pState->nSlotFlags == 0 && pSession->apPlayers[4]->pState->nSlotFlags == 0)
        {
            Network::AbortAndCleanup(hDlg);

            // STRING: HYPERX 0x004c5eac
            g_gameApp.Error("Trying to start a game with no dromer assigned");

            return 0;
        }

        CPlayer* const pHost = pSession->pLeadPlayer;

        if (pHost == 0)
        {
            Network::AbortAndCleanup(hDlg);

            // STRING: HYPERX 0x004c5e80
            g_gameApp.Error("Trying start a game without a Lead player");

            return 0;
        }

        if (pSession->bHosting != 0)
        {
            pHost->pState->nReady = 1;

            NetSetupDialog::CheckPlayerReadyButton(g_pGameSession->pLeadPlayer->pState->nSlotFlags);

            for (nPoll = 0; nPoll < g_pGameSession->nRemoteCount; nPoll++)
            {
                g_pGameSession->pLeadPlayer->SendPlayerReady(g_pGameSession->aRemoteIds[nPoll]);
            }

            if (g_pGameSession->AreAllSlotsReady() != 0)
            {
                g_pGameSession->pLeadPlayer->SendHostStart(DPID_ALLPLAYERS);
                g_pGameSession->pLeadPlayer->SendHostStart(DPID_ALLPLAYERS);
                g_pGameSession->pLeadPlayer->SendHostStart(DPID_ALLPLAYERS);
                g_pGameSession->pLeadPlayer->SendHostStart(DPID_ALLPLAYERS);

                NetSetupDialog::FinalizeAndStartGame();

                g_nNetSetupState    = NETSETUP_STATE_STARTED;
                g_nNetSetupUpdating = 0;

                return 1;
            }

            // STRING: HYPERX 0x004c5e58
            NetSetup::DisplayChatMessage(-1, "SYSTEM: ...Waiting for remote players", (int)strlen("SYSTEM: ...Waiting for remote players"));

            g_nNetSetupState    = NETSETUP_STATE_VERIFY;
            g_nNetSetupUpdating = 0;

            return 1;
        }

        pHost->pState->nReady = 1;

        NetSetupDialog::CheckPlayerReadyButton(g_pGameSession->pLeadPlayer->pState->nSlotFlags);

        for (nPoll = 0; nPoll < g_pGameSession->nRemoteCount; nPoll++)
        {
            g_pGameSession->pLeadPlayer->SendPlayerReady(g_pGameSession->aRemoteIds[nPoll]);
        }

        // STRING: HYPERX 0x004c5e34
        NetSetup::DisplayChatMessage(-1, "SYSTEM: Waiting for remote players", (int)strlen("SYSTEM: Waiting for remote players"));

        g_nNetSetupUpdating = 0;

        return 1;
    }

    Network::AbortAndCleanup(hDlg);

    // STRING: HYPERX 0x004c5e04
    g_gameApp.Error("Trying to start a game with NULL players or DP");

    return 0;
}

// The updating flag is saved and restored rather than cleared.
// FUNCTION: HYPERX 0x00477b00
unsigned int __cdecl NetSetupDialog::OnPlayerJoined(unsigned int nFlags, PlayerSlotName* pEntry)
{
    const int nWasUpdating = g_nNetSetupUpdating;

    if (g_hWndNetSetupDlg == 0)
    {
        return 0;
    }

    g_nNetSetupUpdating = 1;

    if (g_pGameSession != 0 && g_pGameSession->bHosting != 0)
    {
        EnableWindow(GetDlgItem(g_hWndNetSetupDlg, ID_START), 1);
    }

    const unsigned int nSlot = nFlags & PLAYER_NAME_SLOT_MASK;

    if (nSlot == 2)
    {
        SendMessageA(GetDlgItem(g_hWndNetSetupDlg, ID_POSITION_FIRST), WM_SETTEXT, 0, (LPARAM)pEntry->szName);

        EnableWindow(GetDlgItem(g_hWndNetSetupDlg, ID_POSITION_FIRST), 0);
    }
    else if (nSlot == 4)
    {
        SendMessageA(GetDlgItem(g_hWndNetSetupDlg, ID_POSITION_THIRD), WM_SETTEXT, 0, (LPARAM)pEntry->szName);

        EnableWindow(GetDlgItem(g_hWndNetSetupDlg, ID_POSITION_THIRD), 0);
    }
    else if (nSlot == 1)
    {
        SendMessageA(GetDlgItem(g_hWndNetSetupDlg, ID_POSITION_SECOND), WM_SETTEXT, 0, (LPARAM)pEntry->szName);

        EnableWindow(GetDlgItem(g_hWndNetSetupDlg, ID_POSITION_SECOND), 0);
    }
    else if (nSlot == 3)
    {
        SendMessageA(GetDlgItem(g_hWndNetSetupDlg, ID_POSITION_FOURTH), WM_SETTEXT, 0, (LPARAM)pEntry->szName);

        EnableWindow(GetDlgItem(g_hWndNetSetupDlg, ID_POSITION_FOURTH), 0);
    }

    g_nNetSetupUpdating = nWasUpdating;

    return 1;
}

#define SLOT_UNOCCUPIED (-1)

// FUNCTION: HYPERX 0x00477c10
int NetSetupDialog::CheckPlayerReadyButton(int nPlayer)
{
    const int nButton = g_aPlayerReadyButtonId[nPlayer & PLAYER_NAME_SLOT_MASK];

    if (nButton != -1 && g_hWndNetSetupDlg != 0)
    {
        CheckDlgButton(g_hWndNetSetupDlg, nButton, BST_CHECKED);

        return 1;
    }

    return 0;
}

// A state other than NETSETUP_STATE_READY skips straight to starting the game.
// FUNCTION: HYPERX 0x00477d70
unsigned int NetSetupDialog::FinalizeAndStartGame(void)
{
    g_pDPlayLobby->EnableNewPlayers(FALSE);

    if (g_nNetSetupState == NETSETUP_STATE_READY)
    {
        if (Network::SetupPlayerSlot(2, 4) == 0 || Network::SetupPlayerSlot(4, 2) == 0 || Network::SetupPlayerSlot(6, SLOT_UNOCCUPIED) == 0 || Network::SetupPlayerSlot(1, 3) == 0 || Network::SetupPlayerSlot(3, 1) == 0 || Network::SetupPlayerSlot(5, SLOT_UNOCCUPIED) == 0 || Network::SetupPlayerSlot(7, SLOT_UNOCCUPIED) == 0 || Network::SetupPlayerSlot(8, SLOT_UNOCCUPIED) == 0 || Network::SetupPlayerSlot(9, SLOT_UNOCCUPIED) == 0 || Network::SetupPlayerSlot(10, SLOT_UNOCCUPIED) == 0)
        {
            return Network::AbortAndCleanup(g_hWndNetSetupDlg);
        }
    }

    GameSession::FreeAssetBuffer(&g_syncSend, &g_syncRecv);

    if (g_hWndNetSetupDlg != 0)
    {
        if (g_nNetSetupTimerId != 0)
        {
            KillTimer(g_hWndNetSetupDlg, g_nNetSetupTimerId);
        }

        g_nNetSetupTimerId = 0;
    }

    g_bSessionActive = 1;
    g_nNetSetupState = NETSETUP_STATE_STARTED;

    if (g_hWndNetSetupDlg != 0)
    {
        EndDialog(g_hWndNetSetupDlg, 1);
    }

    g_hWndNetSetupDlg = 0;

    return 1;
}

// FUNCTION: HYPERX 0x00477eb0
int NetSetupDialog::OnTimer(HWND)
{
    if (g_nNetSetupState == NETSETUP_STATE_JOINING)
    {
        g_pGameSession->apPlayers[SESSION_LOCAL_PLAYER]->SendAssetRequest(DPID_ALLPLAYERS);
        return 1;
    }

    if (g_nNetSetupState == NETSETUP_STATE_READY)
    {
        Network::FlushPendingPlayerName();
    }

    return 1;
}

// FUNCTION: HYPERX 0x00477ef0
unsigned int __cdecl NetSetupDialog::RefreshSessionList(HWND hDlg)
{
    DPSESSIONDESC description;
    int           hr;

    if (g_nNetSetupState == NETSETUP_STATE_BROWSING && g_pDPlayLobby != 0)
    {
        SetCursor(LoadCursor(0, IDC_WAIT));

        const BOOL bJoinWasDisabled = EnableWindow(GetDlgItem(hDlg, ID_JOIN), 0);
        const BOOL bListWasDisabled = EnableWindow(GetDlgItem(hDlg, ID_SESSION_LIST), 0);

        for (; g_pSessionList != 0 && g_nSessionCount > 0; g_nSessionCount--)
        {
            NetSessionEntry* const pNext = g_pSessionList->m_pNext;

            free(g_pSessionList);

            g_pSessionList = pNext;
        }

        g_pSessionList  = 0;
        g_nSessionCount = 0;

        memset(&description, 0, sizeof(description));

        g_nNetSetupUpdating = 1;

        description.dwSize = sizeof(description);

        description.guidSession = g_guidNetApplication;

        hr = g_pDPlayLobby->EnumSessions(&description, ENUM_TIMEOUT_MS, NetSetupDialog::AddEnumeratedSession, hDlg, DPENUMSESSIONS_AVAILABLE);

        if (hr != 0 && hr != DPERR_NOSESSIONS)
        {
            Network::AbortAndCleanup(hDlg);

            // STRING: HYPERX 0x004c5f04
            g_gameApp.Error("DirectPlay ReEnumSessions() returned %s\n",
                            DirectPlay::GetErrorString(hr));

            g_nNetSetupUpdating = 0;

            return 0;
        }

        if (NetSetupDialog::PopulateSessionList(hDlg) == 0)
        {
            return 0;
        }

        EnableWindow(GetDlgItem(hDlg, ID_JOIN), bJoinWasDisabled == 0);
        EnableWindow(GetDlgItem(hDlg, ID_SESSION_LIST), bListWasDisabled == 0);

        SetCursor(LoadCursor(0, IDC_ARROW));

        g_nNetSetupUpdating = 0;
    }

    return 1;
}

// FUNCTION: HYPERX 0x004780c0
int NetSetupDialog::PopulateSessionList(HWND hDlg)
{
    LRESULT nItem;

    SendMessageA(GetDlgItem(hDlg, ID_SESSION_LIST), LB_RESETCONTENT, 0, 0);

    SendDlgItemMessageA(hDlg, ID_SESSION_LIST, LB_SETCURSEL, (WPARAM)-1, 0);

    NetSessionEntry* pSession = g_pSessionList;

    for (int i = 0; i < g_nSessionCount; i++)
    {
        if (pSession == 0)
        {
            // STRING: HYPERX 0x004c5f30
            g_gameApp.Error("Corrupted session list\n");

            return 0;
        }

        nItem = SendMessageA(GetDlgItem(hDlg, ID_SESSION_LIST), LB_ADDSTRING, 0, (LPARAM)pSession->szSessionName);

        SendMessageA(GetDlgItem(hDlg, ID_SESSION_LIST), LB_SETITEMDATA, nItem, (LPARAM)g_pSessionList);

        pSession = pSession->m_pNext;
    }

    return 1;
}

// A timeout is not a failure.
// FUNCTION: HYPERX 0x00478180
BOOL __stdcall NetSetupDialog::AddEnumeratedSession(LPDPSESSIONDESC pDesc, LPVOID, LPDWORD, DWORD nFlags)
{
    if ((nFlags & DPESC_TIMEDOUT) != 0 && pDesc == 0)
    {
        return 0;
    }

    NetSessionEntry* const pEntry = (NetSessionEntry*)malloc(sizeof(NetSessionEntry));

    if (pEntry == 0)
    {
        // STRING: HYPERX 0x004c5f48
        g_gameApp.Error("Malloc in NETSETUP failed for GAME");

        return 0;
    }

    memcpy(pEntry, pDesc, sizeof(DPSESSIONDESC));

    g_nSessionCount++;

    pEntry->m_bLive = 0;
    pEntry->m_pNext = g_pSessionList;

    g_pSessionList = pEntry;

    return 1;
}

// An unexpected state is reported and then carried on with; all four taken is the only failure.
// FUNCTION: HYPERX 0x004781f0
unsigned int __cdecl NetSetupDialog::OnChangeSlot(HWND hDlg)
{
    const int nWasUpdating = g_nNetSetupUpdating;

    g_nNetSetupUpdating = 1;

    if (g_nNetSetupState == NETSETUP_STATE_ACCEPTING || g_nNetSetupState == NETSETUP_STATE_NAMED || g_nNetSetupState == NETSETUP_STATE_CHOOSING)
    {
        g_nNetSetupState = NETSETUP_STATE_NAMED;
    }
    else
    {
        // STRING: HYPERX 0x004c5fbc
        g_gameApp.Error("Received ChangeSlot request in improper state %d",
                        g_nNetSetupState);
    }

    if (g_pGameSession->apPlayers[2]->pState->nSlotFlags == 0)
    {
        EnableWindow(GetDlgItem(hDlg, ID_POSITION_FIRST), 1);
    }

    if (g_pGameSession->apPlayers[4]->pState->nSlotFlags == 0)
    {
        EnableWindow(GetDlgItem(hDlg, ID_POSITION_THIRD), 1);
    }

    if (g_pGameSession->apPlayers[1]->pState->nSlotFlags == 0)
    {
        EnableWindow(GetDlgItem(hDlg, ID_POSITION_SECOND), 1);
    }

    if (g_pGameSession->apPlayers[3]->pState->nSlotFlags == 0)
    {
        EnableWindow(GetDlgItem(hDlg, ID_POSITION_FOURTH), 1);
    }

    if (g_pGameSession->apPlayers[1]->pState->nSlotFlags == 0)
    {
        g_nNetSetupPendingSlot = 1;

        SetFocus(GetDlgItem(hDlg, ID_POSITION_SECOND));
    }
    else if (g_pGameSession->apPlayers[2]->pState->nSlotFlags == 0)
    {
        g_nNetSetupPendingSlot = 2;

        SetFocus(GetDlgItem(hDlg, ID_POSITION_FIRST));
    }
    else if (g_pGameSession->apPlayers[3]->pState->nSlotFlags == 0)
    {
        g_nNetSetupPendingSlot = 3;

        SetFocus(GetDlgItem(hDlg, ID_POSITION_FOURTH));
    }
    else if (g_pGameSession->apPlayers[4]->pState->nSlotFlags == 0)
    {
        g_nNetSetupPendingSlot = 4;

        SetFocus(GetDlgItem(hDlg, ID_POSITION_THIRD));
    }
    else
    {
        g_nNetSetupPendingSlot = 0;

        // STRING: HYPERX 0x004c5f6c
        g_gameApp.Error("Sorry, maximum number of players has been reached.\nTry another game session\n");

        g_nNetSetupUpdating = nWasUpdating;

        return 0;
    }

    g_nNetSetupUpdating = nWasUpdating;

    return 1;
}

int g_aPlayerReadyButtonId[16] = {
    -1, 1230, 1228, 1231, 1229, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0
};
