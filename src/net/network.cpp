#include "common/compat.h"
#include "net/syncblock.h"
#include "globals.h"
#include "netglobals.h"
#include "net/gamesession.h"
#include "session.h"
#include "netsessionentry.h"
#include "netsetupstate.h"
#include "app/gameapp.h"
#include "cplayer.h"
#include "engine/objectclasses.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <mbstring.h>
#include "net/netsetup.h"
#include "net/netsetupdialog.h"
#include "net/network.h"
#include "app/win32.h"

#define PLAYER_SLOTS 16

#define PERIOD_TIMER_MS 500

// The slot table and the refusal names, defined in src/net/networkslot.cpp.
extern int         g_anNetworkSlotObject[16];
extern const char* g_apNetworkSlotNames[16];

// FUNCTION: HYPERX 0x00440cc0
int __fastcall Network::AddPlayer(GameSession* pSession, unsigned int nDpid, PlayerSlotName* pName)
{
    const unsigned int nFlags = (unsigned int)pName->cSlotFlags;
    const unsigned int nSlot  = nFlags & PLAYER_NAME_SLOT_MASK;

    CPlayer* const pPlayer = pSession->apPlayers[nSlot];

    if (pPlayer == 0)
    {
        // STRING: HYPERX 0x004c17ec
        g_gameApp.Error("Player %s does not exist in AddPlayer()",
                        g_apNetworkSlotNames[nSlot]);

        return 0;
    }

    if ((nFlags & PLAYER_NAME_HOST) != 0 && pSession->bHosting != 0)
    {
        // STRING: HYPERX 0x004c17bc
        g_gameApp.Error("Invalid remote host/join flag in AddPlayer()");

        return 0;
    }

    PlayerState* const pAssign = pPlayer->pState;

    pAssign->nPlayerId   = nDpid;
    pAssign->nConnected  = 1;
    pAssign->nHuman      = (nFlags & PLAYER_NAME_HUMAN) != 0;
    pAssign->nObjectSlot = g_anNetworkSlotObject[nSlot];

    strcpy(pAssign->slotName.GetWireName(), pName->GetWireName());

    pAssign->nSlotFlags    = nSlot;
    pAssign->nReady        = 0;
    pAssign->bLocal        = (nFlags & PLAYER_NAME_LOCAL) != 0;
    pAssign->nActive       = 0;
    pAssign->bHost         = (nFlags & PLAYER_NAME_HOST) != 0;
    pAssign->nDisconnected = 0;

    pSession->apPlayers[nSlot]->ResetState();

    return 1;
}

// The list is a set, so an id already there is accepted; a full list answers no, even for one it holds.

// FUNCTION: HYPERX 0x00440dc0
int __fastcall Network::AddDPID(GameSession* pSession, DWORD nId)
{
    const int nCount = pSession->nRemoteCount;

    if (nCount < SESSION_REMOTE_IDS)
    {
        for (int nAt = 0; nAt < nCount; nAt++)
        {
            if (pSession->aRemoteIds[nAt] == nId)
            {
                return 1;
            }
        }

        pSession->aRemoteIds[nCount] = nId;

        pSession->nRemoteCount++;

        Network::FlushPendingPlayerName();

        g_nSessionState = 1;

        return 1;
    }

    // STRING: HYPERX 0x004c1814
    g_gameApp.Error("DPID list overflow in AddDPID()\n");

    return 0;
}

// The list is compacted in place.
// FUNCTION: HYPERX 0x00440e20
int __fastcall Network::RemoveDPID(GameSession* pSession, DWORD nId)
{
    int bRemoved = 0;

    for (int i = 0; i < pSession->nRemoteCount; i++)
    {
        if (pSession->aRemoteIds[i] == nId)
        {
            for (int j = i + 1; j < pSession->nRemoteCount; j++)
            {
                pSession->aRemoteIds[j - 1] = pSession->aRemoteIds[j];
            }

            pSession->nRemoteCount--;

            NetSetup::OnPlayerDisconnect(nId);

            bRemoved = 1;
            break;
        }
    }

    g_nSessionState = (pSession->nRemoteCount > 0);

    return bRemoved;
}

// Named after the original's own diagnostic, "Update GodsInventory[] array in NETWORK.CPP".

// Each entry's id must equal its index and its kind be zero or one; anything else is fatal.

// FUNCTION: HYPERX 0x004415a0
int __stdcall Network::ValidateStaticObjects(void)
{
    int nResult = 1;

    for (int nIndex = 0; nIndex <= GODS_INVENTORY_LAST; nIndex++)
    {
        const GodsInventoryEntry* const pEntry = &g_godsInventory[nIndex];

        if (pEntry->nObjectId != nIndex || (pEntry->nKind != 0 && pEntry->nKind != 1))
        {
            nResult = 0;

            // STRING: HYPERX 0x004c18f0
            g_gameApp.Fatal("Network table of static objects does not match object id list\nUpdate GodsInventory[] array in NETWORK.CPP\n");

            break;
        }
    }

    return nResult;
}

// An empty slot takes its refusal name.
// FUNCTION: HYPERX 0x00477c40
unsigned int Network::SetupPlayerSlot(unsigned int nDpid, int nSlot)
{
    PlayerSlotName name;

    unsigned int nResult = 1;

    if (g_pGameSession->apPlayers[nDpid & PLAYER_NAME_SLOT_MASK]->pState->nSlotFlags == 0)
    {
        strcpy(name.szName, g_apNetworkSlotNames[nDpid & PLAYER_NAME_SLOT_MASK]);

        name.cSlotFlags = (char)nDpid;

        if (g_pGameSession->bHosting != 0)
        {
            if (nSlot == SLOT_NONE || g_pGameSession->apPlayers[nSlot]->pState->nHuman == 0 || g_pGameSession->apPlayers[nSlot]->pState->nConnected == 0)
            {
                nDpid |= PLAYER_NAME_HOST;
                name.cSlotFlags = (char)nDpid;

                nResult = g_pGameSession->AssignPlayerSlot(nDpid, &name);
            }
            else
            {
                nResult = Network::AddPlayer(g_pGameSession, nDpid, &name);
            }
        }
        else if (nSlot != SLOT_NONE && g_pGameSession->apPlayers[nSlot] == g_pGameSession->pLeadPlayer)
        {
            nResult = g_pGameSession->AssignPlayerSlot(nDpid, &name);
        }
        else
        {
            if (nSlot == SLOT_NONE)
            {
                nDpid |= PLAYER_NAME_HOST;
            }
            else
            {
                PlayerState* const pAssign = g_pGameSession->apPlayers[nSlot]->pState;

                if (pAssign->nSlotFlags == 0 || pAssign->nHuman == 0 || pAssign->nConnected == 0 || pAssign->bHost != 0)
                {
                    nDpid |= PLAYER_NAME_HOST;
                }
            }

            name.cSlotFlags = (char)nDpid;

            nResult = Network::AddPlayer(g_pGameSession, nDpid, &name);
        }
    }

    g_pGameSession->apPlayers[nDpid & PLAYER_NAME_SLOT_MASK]
        ->AssignObject(g_anNetworkSlotObject[nDpid & PLAYER_NAME_SLOT_MASK]);

    return nResult;
}

// FUNCTION: HYPERX 0x004783c0
int Network::ShowWaitingDialog(void)
{
    // No session can start without the set-up dialogs, so there is never anyone to wait for.
    return 1;
}

// The lobby is dropped without release when the session owned it.

// FUNCTION: HYPERX 0x00478590
unsigned int Network::AbortAndCleanup(HWND hWnd)
{
    g_nNetSetupUpdating = 1;

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

    GameSession::FreeAssetBuffer(&g_syncSend, &g_syncRecv);

    while (g_pSessionList != 0 && g_nSessionCount > 0)
    {
        NetSessionEntry* const pNext = g_pSessionList->m_pNext;

        free(g_pSessionList);

        g_pSessionList = pNext;
        g_nSessionCount--;
    }

    g_pSessionList    = 0;
    g_nSessionCount   = 0;
    g_bSessionActive  = 0;
    g_nSessionState   = 0;
    g_nNetSetupState  = NETSETUP_STATE_START;
    g_hWndNetSetupDlg = 0;

    if (hWnd != 0 && g_nNetSetupTimerId != 0)
    {
        KillTimer(hWnd, g_nNetSetupTimerId);
    }

    g_nNetSetupTimerId     = 0;
    g_nNetSetupPendingSlot = 0;

    HCURSOR hCursor = LoadCursorA(0, IDC_ARROW);

    SetCursor(hCursor);

    g_nNetSetupUpdating = 0;
    g_nPrevEngineState  = ESTATE_MATCH_LIVE;

    return 0;
}

#define ASSET_SETTINGS_MIN 6
#define ASSET_SETTINGS_MAX 0x36
#define ASSET_CATALOG_MAX  0x28

// Four blocks of type 2 to 5: a type byte, a count, then six-byte records.
// FUNCTION: HYPERX 0x00478690
void __cdecl Network::SendAssetData(int nRequesterId)
{
    if (g_pGameSession != 0 && g_pGameSession->bHosting != 0)
    {
        if (g_syncSend.apBlocks[SYNC_BLOCK_SETTINGS] != 0 && g_syncSend.apBlocks[SYNC_BLOCK_TEAMS] != 0 && g_syncSend.apBlocks[SYNC_BLOCK_ARENAS] != 0 && g_syncSend.apBlocks[SYNC_BLOCK_PLANS] != 0)
        {
            if (g_syncSend.apBlocks[SYNC_BLOCK_SETTINGS]->nKind == ASSET_TYPE_SETTINGS && g_syncSend.apBlocks[SYNC_BLOCK_TEAMS]->nKind == ASSET_TYPE_TEAMS && g_syncSend.apBlocks[SYNC_BLOCK_ARENAS]->nKind == ASSET_TYPE_ARENAS && g_syncSend.apBlocks[SYNC_BLOCK_PLANS]->nKind == ASSET_TYPE_PLANS && g_syncSend.apBlocks[SYNC_BLOCK_SETTINGS]->nCount >= ASSET_SETTINGS_MIN && g_syncSend.apBlocks[SYNC_BLOCK_SETTINGS]->nCount <= ASSET_SETTINGS_MAX && g_syncSend.apBlocks[SYNC_BLOCK_TEAMS]->nCount <= ASSET_CATALOG_MAX && g_syncSend.apBlocks[SYNC_BLOCK_ARENAS]->nCount <= ASSET_CATALOG_MAX && g_syncSend.apBlocks[SYNC_BLOCK_PLANS]->nCount <= ASSET_CATALOG_MAX)
            {
                g_pGameSession->apPlayers[SESSION_LOCAL_PLAYER]->SendRawMsg(
                    nRequesterId, g_syncSend.apBlocks[SYNC_BLOCK_SETTINGS], g_syncSend.apBlocks[SYNC_BLOCK_SETTINGS]->nCount * sizeof(SyncFileStamp) + sizeof(SyncBlockHeader));

                g_pGameSession->apPlayers[SESSION_LOCAL_PLAYER]->SendRawMsg(
                    nRequesterId, g_syncSend.apBlocks[SYNC_BLOCK_TEAMS], g_syncSend.apBlocks[SYNC_BLOCK_TEAMS]->nCount * sizeof(SyncFileStamp) + sizeof(SyncBlockHeader));

                g_pGameSession->apPlayers[SESSION_LOCAL_PLAYER]->SendRawMsg(
                    nRequesterId, g_syncSend.apBlocks[SYNC_BLOCK_ARENAS], g_syncSend.apBlocks[SYNC_BLOCK_ARENAS]->nCount * sizeof(SyncFileStamp) + sizeof(SyncBlockHeader));

                g_pGameSession->apPlayers[SESSION_LOCAL_PLAYER]->SendRawMsg(
                    nRequesterId, g_syncSend.apBlocks[SYNC_BLOCK_PLANS], g_syncSend.apBlocks[SYNC_BLOCK_PLANS]->nCount * sizeof(SyncFileStamp) + sizeof(SyncBlockHeader));

                return;
            }

            g_bNetSetupStatusDirty = 1;

            // STRING: HYPERX 0x004c6074
            sprintf(g_szNetSetupStatus, "Trying to process REQUEST with corrupted asset list");

            if (g_hWndNetSetupDlg != 0)
            {
                PostMessageA(g_hWndNetSetupDlg, WM_COMMAND, IDCANCEL, 0);
            }

            return;
        }

        g_bNetSetupStatusDirty = 1;

        // STRING: HYPERX 0x004c6040
        sprintf(g_szNetSetupStatus, "Trying to process NEEDINFO with NULL assets buffer");

        if (g_hWndNetSetupDlg != 0)
        {
            PostMessageA(g_hWndNetSetupDlg, WM_COMMAND, IDCANCEL, 0);
        }
    }
}

// A length mismatch complains and stores the block anyway; the sender id is never read.
// FUNCTION: HYPERX 0x00478840
void __cdecl Network::ReceiveAssetPacket(const SyncBlockHeader* pPacket, unsigned int nLength, int)
{
    SyncBlockIndex nBlock;
    unsigned int   nExpected;

    if (g_hWndNetSetupDlg != 0)
    {
        if (g_nNetSetupState == NETSETUP_STATE_JOINING)
        {
            g_nNetSetupState = NETSETUP_STATE_VERIFYING;
        }
        else if (g_nNetSetupState != NETSETUP_STATE_VERIFYING)
        {
            g_bNetSetupStatusDirty = 1;

            // STRING: HYPERX 0x004c6164
            sprintf(g_szNetSetupStatus, "Trying to process ASSETS in unknown state %d", g_nNetSetupState);

            PostMessageA(g_hWndNetSetupDlg, WM_COMMAND, IDCANCEL, 0);

            return;
        }

        if (g_pGameSession == 0 || g_pGameSession->bHosting != 0)
        {
            g_bNetSetupStatusDirty = 1;

            // STRING: HYPERX 0x004c60d8
            sprintf(g_szNetSetupStatus, "Trying to process ASSETS while HOST");

            PostMessageA(g_hWndNetSetupDlg, WM_COMMAND, IDCANCEL, 0);

            return;
        }

        if (g_syncRecv.Settings() == 0 || g_syncRecv.Teams() == 0 || g_syncRecv.Arenas() == 0 || g_syncRecv.Plans() == 0)
        {
            g_bNetSetupStatusDirty = 1;

            // STRING: HYPERX 0x004c60fc
            sprintf(g_szNetSetupStatus, "Trying to process ASSETS with NULL assets buffer");

            PostMessageA(g_hWndNetSetupDlg, WM_COMMAND, IDCANCEL, 0);

            return;
        }

        nExpected = (unsigned int)pPacket->nCount * sizeof(SyncFileStamp) + sizeof(SyncBlockHeader);

        if (nExpected != nLength)
        {
            g_bNetSetupStatusDirty = 1;

            // STRING: HYPERX 0x004c6194
            sprintf(g_szNetSetupStatus, "%s(ASSETS packet length %d instead of %d)",
                    // STRING: HYPERX 0x004c0e48
                    "Network connection failed.\nPlease try again.\n",
                    nLength,
                    nExpected);

            PostMessageA(g_hWndNetSetupDlg, WM_COMMAND, IDCANCEL, 0);
        }

        switch (pPacket->nKind)
        {
            case ASSET_TYPE_SETTINGS:
                nBlock = SYNC_BLOCK_SETTINGS;
                g_pGameSession->nAssetsArrived |= 1 << SYNC_BLOCK_SETTINGS;
                break;

            case ASSET_TYPE_TEAMS:
                nBlock = SYNC_BLOCK_TEAMS;
                g_pGameSession->nAssetsArrived |= 1 << SYNC_BLOCK_TEAMS;
                break;

            case ASSET_TYPE_ARENAS:
                nBlock = SYNC_BLOCK_ARENAS;
                g_pGameSession->nAssetsArrived |= 1 << SYNC_BLOCK_ARENAS;
                break;

            case ASSET_TYPE_PLANS:
                nBlock = SYNC_BLOCK_PLANS;
                g_pGameSession->nAssetsArrived |= 1 << SYNC_BLOCK_PLANS;
                break;

            default:
                g_bNetSetupStatusDirty = 1;

                // STRING: HYPERX 0x004c6130
                sprintf(g_szNetSetupStatus, "Trying to process ASSETS from unknown packet type");

                PostMessageA(g_hWndNetSetupDlg, WM_COMMAND, IDCANCEL, 0);

                return;
        }

        memcpy(g_syncRecv.apBlocks[nBlock], pPacket, nLength);

        if ((g_pGameSession->nAssetsArrived & SYNC_BLOCKS_ALL) == SYNC_BLOCKS_ALL)
        {
            PostMessageA(g_hWndNetSetupDlg, WM_COMMAND, ID_HOST, 0);
        }

        return;
    }

    // STRING: HYPERX 0x004c60a8
    g_gameApp.Error("Trying to process ASSETS with NULL dialog box");
}

// The host's answer to a slot request; from the last dialog state on, every request is refused.
// FUNCTION: HYPERX 0x00478aa0
void __cdecl Network::OnPlayerConnected(DWORD idFrom, int nSlot)
{
    PlayerState* pState;

    if (g_pGameSession == 0)
    {
        return;
    }

    if (g_pGameSession->bHosting == 0)
    {
        return;
    }

    if (g_nNetSetupState >= NETSETUP_STATE_DIALOG_LAST)
    {
        g_pGameSession->apPlayers[SESSION_LOCAL_PLAYER]->SendSlotRejected(idFrom,
                                                                          (char)nSlot);

        return;
    }

    CPlayer* const pPlayer = g_pGameSession->apPlayers[nSlot];

    if (pPlayer == 0)
    {
        return;
    }

    pState = pPlayer->pState;
    if (pState->nSlotFlags != 0)
    {
        g_pGameSession->apPlayers[SESSION_LOCAL_PLAYER]->SendSlotRejected(idFrom,
                                                                          (char)nSlot);

        return;
    }

    if (g_pGameSession->apPlayers[nSlot] != 0)
    {
        g_pGameSession->apPlayers[nSlot]->pState->nSlotFlags = nSlot;

        g_pGameSession->apPlayers[SESSION_LOCAL_PLAYER]->SendSlotAccepted(idFrom,
                                                                          (char)nSlot);
    }
}

// Refuses outside NETSETUP_STATE_OFFERED; the offered slot must be empty with no join in flight.
// FUNCTION: HYPERX 0x00478b30
void Network::OnAcceptPlayerSlot(int nPlayer)
{
    if (g_hWndNetSetupDlg != 0)
    {
        if (g_nNetSetupState != NETSETUP_STATE_OFFERED)
        {
            g_bNetSetupStatusDirty = 1;

            // STRING: HYPERX 0x004c6220
            sprintf(g_szNetSetupStatus, "Trying to process ACCEPT in unknown state %d", g_nNetSetupState);

            PostMessageA(g_hWndNetSetupDlg, WM_COMMAND, IDCANCEL, 0);

            return;
        }

        if (g_pGameSession != 0 && g_pGameSession->pLeadPlayer == 0 && g_pGameSession->apPlayers[nPlayer] != 0 && g_pGameSession->apPlayers[nPlayer]->pState->nSlotFlags == 0 && g_pGameSession->bHosting == 0)
        {
            CPlayer* const pCarrier = g_pGameSession->apPlayers[SESSION_LOCAL_PLAYER];

            if (g_pGameSession->AssignPlayerSlot(
                    g_pGameSession->apPlayers[SESSION_LOCAL_PLAYER]->pState->nSlotFlags,
                    &pCarrier->pState->slotName) == 0)
            {
                if (g_hWndNetSetupDlg == 0)
                {
                    return;
                }

                PostMessageA(g_hWndNetSetupDlg, WM_COMMAND, IDCANCEL, 0);

                return;
            }

            g_pGameSession->apPlayers[nPlayer]->pState->nActive = 1;
            g_pGameSession->apPlayers[nPlayer]->pState->nReady  = 1;
            g_nDPlayLastError                                   = 0;
            g_nNetSetupState                                    = NETSETUP_STATE_READY;
            g_nNetSetupUpdating                                 = 1;

            NetSetupDialog::CheckPlayerReadyButton(g_pGameSession->pLeadPlayer->pState->nSlotFlags);

            // STRING: HYPERX 0x004c5e34
            NetSetup::DisplayChatMessage(-1, "SYSTEM: Waiting for remote players", (int)_mbslen((const unsigned char*)"SYSTEM: Waiting for remote players"));

            g_nNetSetupUpdating = 0;

            Network::FlushPendingPlayerName();

            return;
        }

        g_bNetSetupStatusDirty = 1;

        // STRING: HYPERX 0x004c61f0
        sprintf(g_szNetSetupStatus, "Trying to process ACCEPT for wrong player %d", nPlayer);

        PostMessageA(g_hWndNetSetupDlg, WM_COMMAND, IDCANCEL, 0);

        return;
    }

    // STRING: HYPERX 0x004c61c0
    g_gameApp.Error("Trying to process ACCEPT with NULL dialog box");
}

// FUNCTION: HYPERX 0x00478cd0
void Network::OnRejectPlayerSlot(int nPlayer)
{
    if (g_hWndNetSetupDlg != 0)
    {
        if (g_nNetSetupState != NETSETUP_STATE_OFFERED)
        {
            g_bNetSetupStatusDirty = 1;

            // STRING: HYPERX 0x004c6310
            sprintf(g_szNetSetupStatus, "Trying to process REJECT in unknown state %d", g_nNetSetupState);

            PostMessageA(g_hWndNetSetupDlg, WM_COMMAND, IDCANCEL, 0);

            return;
        }

        if (g_pGameSession != 0 && g_pGameSession->bHosting == 0)
        {
            g_bNetSetupStatusDirty = 1;

            // STRING: HYPERX 0x004c62b0
            sprintf(g_szNetSetupStatus, "Sorry, the player position you have selected\nhas already been filled.  Please try another one.\n");

            g_nNetSetupState = NETSETUP_STATE_CHOOSING;

            PostMessageA(g_hWndNetSetupDlg, WM_COMMAND, ID_HOST, 0);

            return;
        }

        g_bNetSetupStatusDirty = 1;

        // STRING: HYPERX 0x004c6280
        sprintf(g_szNetSetupStatus, "Trying to process REJECT for wrong player %d", nPlayer);

        PostMessageA(g_hWndNetSetupDlg, WM_COMMAND, IDCANCEL, 0);

        return;
    }

    // STRING: HYPERX 0x004c6250
    g_gameApp.Error("Trying to process REJECT with NULL dialog box");
}

// FUNCTION: HYPERX 0x00479360
void Network::FlushPendingPlayerName(void)
{
    if (g_pGameSession != 0)
    {
        CPlayer* const pPlayer = g_pGameSession->pLeadPlayer;

        if (pPlayer != 0)
        {
            PlayerState* const pPending = pPlayer->pState;

            if (pPending->nActive != 0)
            {
                pPlayer->SendStringMsg(DPID_ALLPLAYERS, pPending->nSlotFlags, pPending->slotName.GetWireName());

                if (g_pGameSession->pLeadPlayer->pState->nReady != 0)
                {
                    g_pGameSession->pLeadPlayer->SendPlayerReady(DPID_ALLPLAYERS);
                }
            }
        }
    }
}
