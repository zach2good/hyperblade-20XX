#include "common/compat.h"
#include "globals.h"
#include "net/netsetupstate.h"
#include "net/netglobals.h"
#include "widget.h"
#include "ui/screen.h"
#include <windows.h>
#include <string.h>
#include "app/gameapp.h"
#include "ui/gamesetup.h"
#include "net/netsetupdialog.h"
#include "net/network.h"
#include "net/playerslotname.h"
#include "app/win32.h"
#include "net/directplay.h"
#include "app/dialogresources.h"

// A bad network state shows the first dialog forever.
// FUNCTION: HYPERX 0x004754a0
int GameSetup::ShowPreMatchDialogs(void)
{
    int nResult;

    for (;;)
    {
        for (;;)
        {
            nResult = DialogBoxParamA(g_gameApp.m_hInstance, MAKEINTRESOURCEA(DIALOG_SETUP), g_gameApp.m_hWnd, GameSetup::SessionListDialogProc, 0);

            if (nResult == 0)
            {
                goto done;
            }

            if (g_nNetSetupState != NETSETUP_STATE_WAITING_A && g_nNetSetupState != NETSETUP_STATE_WAITING_B)
            {
                break;
            }

            nResult = DialogBoxParamA(g_gameApp.GetInstance(), MAKEINTRESOURCEA(DIALOG_CONTINUE), g_gameApp.GetMainWindow(), GameSetup::SessionConnectDialogProc, 0);

            if (nResult != 0)
            {
                goto done;
            }
        }

        // STRING: HYPERX 0x004c561c
        g_gameApp.Error("Trying to continue setup in unknown state %d",
                        g_nNetSetupState);
    }

done:
    return nResult;
}

// The marker is in src/net/netsetupdialog.cpp.
extern DPSESSIONDESC* g_pSelectedSession;

// FUNCTION: HYPERX 0x00475510
INT_PTR CALLBACK GameSetup::SessionListDialogProc(HWND hDlg, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    int nHandled = 0;

    if (g_pActiveScreen != 0)
    {
        g_pActiveScreen->SetRepaintBackground(1);
    }

    switch (nMsg)
    {
        case WM_INITDIALOG:
            g_nNetSetupState       = NETSETUP_STATE_IDLE;
            g_nNetSetupUpdating    = 0;
            g_hWndNetSetupDlg      = hDlg;
            g_nNetSetupPendingSlot = 0;
            g_nNetSetupControlId   = 0;
            g_pSelectedSession     = 0;

            Win32::CenterWindow(hDlg, GetParent(hDlg));

            if (NetSetupDialog::EnumerateProviders(hDlg) == 0)
            {
                g_hWndNetSetupDlg = 0;
                EndDialog(hDlg, 0);
            }

            nHandled = 0;
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDCANCEL:
                    Network::AbortAndCleanup(hDlg);

                    if (g_bNetSetupStatusDirty != 0)
                    {
                        g_gameApp.Error(g_szNetSetupStatus);
                        g_bNetSetupStatusDirty = 0;
                        strcpy(g_szNetSetupStatus, "");
                    }

                    EndDialog(hDlg, 0);

                    return 1;

                case ID_JOIN:
                    if (g_nNetSetupUpdating == 0)
                    {
                        if (g_nNetSetupTimerId != 0)
                        {
                            KillTimer(hDlg, g_nNetSetupTimerId);
                        }

                        g_nNetSetupTimerId = 0;
                        g_nNetSetupState   = NETSETUP_STATE_SELECTED;
                        EndDialog(hDlg, 1);
                    }
                    break;

                case ID_HOST:
                    if (g_nNetSetupUpdating == 0 && g_gameApp.FindCDRomDrive(0) != -1)
                    {
                        if (g_nNetSetupTimerId != 0)
                        {
                            KillTimer(hDlg, g_nNetSetupTimerId);
                        }

                        g_nNetSetupTimerId = 0;
                        g_nNetSetupState   = NETSETUP_STATE_HOST_CHOSEN;
                        EndDialog(hDlg, 1);
                    }
                    break;

                case ID_SEARCH:
                    if (g_nNetSetupUpdating == 0 && NetSetupDialog::RefreshSessionList(hDlg) == 0)
                    {
                        EndDialog(hDlg, 0);
                    }
                    break;

                case ID_SESSION_LIST:
                    if (g_nNetSetupUpdating == 0 && HIWORD(wParam) == LBN_SELCHANGE && NetSetupDialog::SelectSession(hDlg, nMsg, wParam, lParam) == 0)
                    {
                        EndDialog(hDlg, 0);
                    }
                    break;

                case ID_PROVIDER_LIST:
                    if (g_nNetSetupUpdating == 0 && HIWORD(wParam) == LBN_SELCHANGE && NetSetupDialog::SearchSessions(hDlg, nMsg, wParam, lParam) == 0)
                    {
                        EndDialog(hDlg, 0);
                    }
                    break;
            }

            nHandled = 1;
            break;

        case WM_TIMER:
            if (g_nNetSetupUpdating == 0 && NetSetupDialog::OnTimer(hDlg) == 0)
            {
                if (g_nNetSetupTimerId != 0)
                {
                    KillTimer(hDlg, g_nNetSetupTimerId);
                }

                g_nNetSetupTimerId = 0;
                Network::AbortAndCleanup(hDlg);
                EndDialog(hDlg, 0);
            }

            nHandled = 1;
            break;
    }

    return nHandled;
}

// The marker is in src/net/netsetupdialog.cpp.
extern int g_bChatEntryActive;

#define POLL_INTERVAL_MS 0x7d0

// Template 0x25f. The host button means six things by setup state; the chat box outranks state six.
// FUNCTION: HYPERX 0x004757f0
INT_PTR CALLBACK GameSetup::SessionConnectDialogProc(HWND hDlg, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    int nHandled = 0;

    if (g_pActiveScreen != 0)
    {
        g_pActiveScreen->SetRepaintBackground(1);
    }

    switch (nMsg)
    {
        case WM_INITDIALOG:
            g_nNetSetupUpdating    = 0;
            g_hWndNetSetupDlg      = hDlg;
            g_nNetSetupPendingSlot = 0;
            g_nNetSetupControlId   = 0;

            Win32::CenterWindow(hDlg, GetParent(hDlg));

            if (g_nNetSetupState == NETSETUP_STATE_HOST_CHOSEN)
            {
                if (NetSetupDialog::HostSession(hDlg, nMsg, wParam, lParam) == 0)
                {
                    EndDialog(hDlg, 0);
                }
            }
            else if (g_nNetSetupState == NETSETUP_STATE_SELECTED)
            {
                if (NetSetupDialog::JoinSession(hDlg, nMsg, wParam, lParam) == 0)
                {
                    EndDialog(hDlg, 0);
                }
            }
            else
            {
                // STRING: HYPERX 0x004c561c
                g_gameApp.Error("Trying to continue setup in unknown state %d",
                                g_nNetSetupState);
                EndDialog(hDlg, 0);
            }

            g_nNetSetupTimerId = SetTimer(hDlg, 0, POLL_INTERVAL_MS, 0);

            if (g_nNetSetupTimerId == 0)
            {
                Network::AbortAndCleanup(hDlg);

                // STRING: HYPERX 0x004c564c
                g_gameApp.Error("NETSETUP timer failed");

                return 0;
            }

            nHandled = 0;
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                    if (g_nNetSetupTimerId != 0)
                    {
                        KillTimer(hDlg, g_nNetSetupTimerId);
                    }

                    g_nNetSetupTimerId = 0;

                    if (NetSetupDialog::StartGame(hDlg, nMsg, wParam, lParam) == 0)
                    {
                        Network::AbortAndCleanup(hDlg);
                        EndDialog(hDlg, 0);
                    }

                    return 1;

                case IDCANCEL:
                    Network::AbortAndCleanup(hDlg);

                    if (g_bNetSetupStatusDirty != 0)
                    {
                        g_gameApp.Error(g_szNetSetupStatus);
                        g_bNetSetupStatusDirty = 0;
                        strcpy(g_szNetSetupStatus, "");
                    }

                    EndDialog(hDlg, 0);

                    return 1;

                case IDABORT:
                    if (g_nNetSetupUpdating == 0 && g_nNetSetupControlId != 0 && NetSetupDialog::ValidatePlayerName(hDlg, nMsg, g_nNetSetupControlId, lParam) != 0 && NetSetupDialog::ConfirmPlayerPosition(hDlg, nMsg, g_nNetSetupControlId, lParam) == 0)
                    {
                        EndDialog(hDlg, 0);
                    }
                    break;

                case ID_HOST:
                    if (g_nNetSetupState == NETSETUP_STATE_BROWSING)
                    {
                        SetFocus(GetDlgItem(hDlg, ID_HOST));
                    }
                    else if (g_nNetSetupState == NETSETUP_STATE_VERIFYING)
                    {
                        if (NetSetupDialog::VerifyAssets(hDlg, nMsg, wParam, lParam) == 0)
                        {
                            EndDialog(hDlg, 0);
                        }
                    }
                    else if (g_bChatEntryActive != 0)
                    {
                        NetSetupDialog::SendChatMessage(hDlg, nMsg, wParam, lParam);
                    }
                    else if (g_nNetSetupState == NETSETUP_STATE_NAMED || g_nNetSetupState == NETSETUP_STATE_HOST_CHOSEN)
                    {
                        if (NetSetupDialog::ValidatePlayerName(hDlg, nMsg, wParam, lParam) != 0)
                        {
                            SetFocus(GetDlgItem(hDlg, ID_CHAT_ENTRY));
                        }
                    }
                    else if (g_nNetSetupState == NETSETUP_STATE_CHOOSING)
                    {
                        NetSetupDialog::OnChangeSlot(hDlg);
                    }
                    break;

                case ID_POSITION_FIRST:
                case ID_POSITION_SECOND:
                case ID_POSITION_THIRD:
                case ID_POSITION_FOURTH:
                    if (g_nNetSetupUpdating == 0 && HIWORD(wParam) == EN_CHANGE && NetSetupDialog::SelectPlayerPosition(hDlg, nMsg, wParam, lParam) == 0)
                    {
                        EndDialog(hDlg, 0);
                    }
                    break;

                case ID_TEAM_HOME:
                case ID_TEAM_AWAY:
                case ID_ARENA:
                    if (g_nNetSetupUpdating == 0 && HIWORD(wParam) == CBN_SELCHANGE && NetSetupDialog::OnSelectionChanged(hDlg, nMsg, wParam, lParam) == 0)
                    {
                        EndDialog(hDlg, 0);
                    }
                    break;

                case ID_PLAYER_LIST:
                {
                    PlayerSlotName* const pJoined = (PlayerSlotName*)lParam;

                    NetSetupDialog::OnPlayerJoined(pJoined->cSlotFlags, pJoined);
                    break;
                }

                case ID_CHAT_ENTRY:
                    if (g_nNetSetupUpdating == 0 && HIWORD(wParam) == EN_CHANGE)
                    {
                        g_bChatEntryActive = 1;
                    }
                    break;
            }

            nHandled = 1;
            break;

        case WM_TIMER:
            if (NetSetupDialog::OnTimer(hDlg) == 0)
            {
                if (g_nNetSetupTimerId != 0)
                {
                    KillTimer(hDlg, g_nNetSetupTimerId);
                }

                g_nNetSetupTimerId = 0;
                Network::AbortAndCleanup(hDlg);
                EndDialog(hDlg, 0);
            }

            nHandled = 1;
            break;
    }

    return nHandled;
}
