#include "common/compat.h"
#include "globals.h"
#include "netglobals.h"
#include "netsetupstate.h"
#include "session.h"
#include <mbstring.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include "gamesession.h"
#include "cplayer.h"
#include "app/gameapp.h"
#include "net/netsetup.h"
#include "net/netsetupdialog.h"
#include "net/network.h"

// A name for the slot this end was waiting on cancels the wait and goes back to choosing.
// FUNCTION: HYPERX 0x00478da0
void NetSetup::ProcessAddPlayerMsg(int nDpid, int nSlot, PlayerSlotName* pName)
{
    if (g_hWndNetSetupDlg != 0)
    {
        if (g_pGameSession == 0 || g_pGameSession->apPlayers[nSlot] == 0)
        {
            g_bNetSetupStatusDirty = 1;

            // STRING: HYPERX 0x004c6340
            sprintf(g_szNetSetupStatus, "Trying to process ADD_NAME for NULL player %d", nSlot);
        }
        else
        {
            if (Network::AddPlayer(g_pGameSession, nDpid, pName) != 0)
            {
                PostMessageA(g_hWndNetSetupDlg, WM_COMMAND, NETSETUP_NAME_ARRIVED, (LPARAM)&g_pGameSession->apPlayers[nSlot]->pState->slotName);

                Network::AddDPID(g_pGameSession, nDpid);

                if (g_nNetSetupPendingSlot != nSlot)
                {
                    return;
                }

                if (g_nNetSetupState < NETSETUP_STATE_SELECTED)
                {
                    return;
                }

                if (g_nNetSetupState >= (int)NETSETUP_STATE_AFTER_CHOOSING)
                {
                    return;
                }

                g_nNetSetupPendingSlot = 0;
                g_nNetSetupState       = NETSETUP_STATE_CHOOSING;

                PostMessageA(g_hWndNetSetupDlg, WM_COMMAND, ID_HOST, 0);

                // STRING: HYPERX 0x004c6370
                NetSetup::DisplayChatMessage(-1, "SYSTEM: Sorry, another player has taken this position, try another.", (int)_mbslen((const unsigned char*)"SYSTEM: Sorry, another player has taken this position, try another."));

                return;
            }
        }

        PostMessageA(g_hWndNetSetupDlg, WM_COMMAND, IDCANCEL, 0);
    }
}

// The start signal is sent four times, because nothing acknowledges it.
// FUNCTION: HYPERX 0x00478ec0
void __cdecl NetSetup::OnPlayerReady(unsigned int nSlot)
{
    CPlayer* const pPlayer = g_pGameSession->apPlayers[nSlot];

    if (pPlayer != 0)
    {
        PlayerState* const pAssign = pPlayer->pState;

        if (pAssign->nReady == 0)
        {
            pAssign->nReady = 1;

            NetSetupDialog::CheckPlayerReadyButton(nSlot);

            if (g_pGameSession->bHosting != 0)
            {
                if (g_pGameSession->AreAllSlotsReady() != 0)
                {
                    if (g_pGameSession->pLeadPlayer == 0)
                    {
                        // STRING: HYPERX 0x004c63b4
                        g_gameApp.Error("RCV: READY for HOST with NULL Lead player");
                    }
                    else if (g_pGameSession->pLeadPlayer->pState->nReady != 0)
                    {
                        for (int nIndex = 0; nIndex < SESSION_PLAYER_COUNT; nIndex++)
                        {
                            g_pGameSession->apPlayers[nIndex]->pState->nReady = 0;
                        }

                        g_pGameSession->pLeadPlayer->SendHostStart(DPID_ALLPLAYERS);
                        g_pGameSession->pLeadPlayer->SendHostStart(DPID_ALLPLAYERS);
                        g_pGameSession->pLeadPlayer->SendHostStart(DPID_ALLPLAYERS);
                        g_pGameSession->pLeadPlayer->SendHostStart(DPID_ALLPLAYERS);

                        NetSetupDialog::FinalizeAndStartGame();
                    }
                }
            }
        }
    }
}

// FUNCTION: HYPERX 0x00478fb0
void __stdcall NetSetup::OnHostStartGame(void)
{
    if (g_pGameSession->pLeadPlayer != 0 && g_pGameSession->pLeadPlayer->pState->nReady != 0)
    {
        for (int nIndex = 0; nIndex < SESSION_PLAYER_COUNT; nIndex++)
        {
            g_pGameSession->apPlayers[nIndex]->pState->nReady = 0;
        }

        NetSetupDialog::FinalizeAndStartGame();

        return;
    }

    if (g_nNetSetupState >= (int)NETSETUP_STATE_JOINED)
    {
        g_bNetSetupStatusDirty = 1;
    }

    g_nNetSetupState = NETSETUP_STATE_START;

    // STRING: HYPERX 0x004c63e0
    sprintf(g_szNetSetupStatus, "%s(The HOST has started without you)\n",
            // STRING: HYPERX 0x004c0e48
            "Network connection failed.\nPlease try again.\n");

    PostMessageA(g_hWndNetSetupDlg, WM_COMMAND, IDCANCEL, 0);
}

// The list is trimmed to two entries first, so the newest is always the third.

// FUNCTION: HYPERX 0x00479050
void NetSetup::DisplayChatMessage(int, const char* pszText, int)
{
    const int nWasUpdating = g_nNetSetupUpdating;

    if (g_hWndNetSetupDlg == 0)
    {
        // STRING: HYPERX 0x004c6408
        g_gameApp.Error("Trying to process CHAT message with NULL dialog box");

        return;
    }

    g_nNetSetupUpdating = 0;

    if (g_nNetSetupState > NETSETUP_STATE_JOINED)
    {
        while (SendMessageA(GetDlgItem(g_hWndNetSetupDlg, ID_CHAT_LIST), LB_GETCOUNT, 0, 0) > 2)
        {
            SendMessageA(GetDlgItem(g_hWndNetSetupDlg, ID_CHAT_LIST), LB_DELETESTRING, 0, 0);
        }

        SendMessageA(GetDlgItem(g_hWndNetSetupDlg, ID_CHAT_LIST), LB_ADDSTRING, 0, (LPARAM)pszText);
    }
    else
    {
        SendMessageA(GetDlgItem(g_hWndNetSetupDlg, ID_CHAT_ENTRY), WM_SETTEXT, 0, (LPARAM)pszText);
    }

    g_nNetSetupUpdating = nWasUpdating;
}

// Resets the slot of a player who has left and marks a disconnect pending.

// FUNCTION: HYPERX 0x00479140
void __cdecl NetSetup::OnPlayerDisconnect(DWORD nId)
{
    if (g_nNetSetupState == NETSETUP_STATE_START)
    {
        return;
    }

    GameSession* const pSession = g_pGameSession;
    CPlayer**          ppEntry  = pSession->apPlayers;

    for (int nSlot = 0; nSlot < SESSION_PLAYER_SLOTS; nSlot++, ppEntry++)
    {
        if (pSession != 0 && *ppEntry != 0 && (*ppEntry)->pState->nPlayerId == nId)
        {
            g_pGameSession->apPlayers[nSlot]->pState->bLocal        = 0;
            g_pGameSession->apPlayers[nSlot]->pState->nPlayerId     = DPID_UNKNOWN;
            g_pGameSession->apPlayers[nSlot]->pState->nHuman        = 0;
            g_pGameSession->apPlayers[nSlot]->pState->nDisconnected = 1;
            g_pGameSession->apPlayers[nSlot]->pState->nReady        = 0;

            g_pGameSession->nPendingDisconnect = 1;

            if (g_pGameSession->apPlayers[nSlot]->pState->bHost != 0)
            {
                if (g_nNetSetupState <= (int)NETSETUP_STATE_DIALOG_LAST && g_hWndNetSetupDlg != 0)
                {
                    // STRING: HYPERX 0x004c6460
                    sprintf(g_szNetSetupStatus, "%s(The HOST machine disconected)\n",
                            // STRING: HYPERX 0x004c0e48
                            "Network connection failed.\nPlease try again.\n");

                    g_bNetSetupStatusDirty = 1;

                    PostMessageA(g_hWndNetSetupDlg, WM_COMMAND, IDCANCEL, 0);

                    return;
                }

                if (g_gameApp.m_nEngineState != ESTATE_MATCH_LIVE)
                {
                    g_pGameSession->HandlePlayerDisconnect(nSlot);
                }

                if (g_nNetSetupState == NETSETUP_STATE_PLAYING)
                {
                    g_pGameSession->apPlayers[nSlot]->pState->nReady = 0;

                    NetSetup::OnPlayerReady(nSlot);
                }

                if (g_nNetSetupState == NETSETUP_STATE_STARTED)
                {
                    g_pGameSession->apPlayers[nSlot]->pState->nReady = 1;
                }

                return;
            }

            if (g_nNetSetupState <= (int)NETSETUP_STATE_DIALOG_LAST && g_hWndNetSetupDlg != 0)
            {
                g_bNetSetupStatusDirty = 1;

                // STRING: HYPERX 0x004c643c
                sprintf(g_szNetSetupStatus, "%s(CLIENT player %s disconected)\n",
                        // STRING: HYPERX 0x004c0e48
                        "Network connection failed.\nPlease try again.\n",
                        g_pGameSession->apPlayers[nSlot]->pState->slotName.szName);

                PostMessageA(g_hWndNetSetupDlg, WM_COMMAND, IDCANCEL, 0);

                return;
            }

            if (g_gameApp.m_nEngineState != ESTATE_MATCH_LIVE)
            {
                g_pGameSession->HandlePlayerDisconnect(nSlot);
            }

            if (g_nNetSetupState == NETSETUP_STATE_PLAYING)
            {
                g_pGameSession->apPlayers[nSlot]->pState->nReady = 1;

                NetSetup::OnPlayerReady(nSlot);
            }

            if (g_nNetSetupState == NETSETUP_STATE_STARTED)
            {
                g_pGameSession->apPlayers[nSlot]->pState->nReady = 1;
            }

            return;
        }
    }
}
