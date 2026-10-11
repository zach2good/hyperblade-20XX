#include "common/compat.h"
#include "globals.h"
#include "engine/objectclasses.h"
#include "ratingsdialogstate.h"
#include "dialogids.h"
#include "engine/objectstats.h"
#include <new.h>
#include <string.h>
#include "common/stringpool.h"
#include "app/gameapp.h"
#include "engine/playerobject.h"
#include "ui/dialog.h"
#include "ui/ratingsdialog.h"
#include "app/win32.h"
#include "engine/gameengine.h"
#include "engine/statids.h"
#include "app/stringids.h"

// The first four slots are players; everything after them takes the second name array.
#define FIRST_NON_PLAYER 4

// The key configuration sheet's handler in keyconfigdialog.cpp is the same, down to the string id.

// FUNCTION: HYPERX 0x00419aa0
int __cdecl RatingsDialog::OnInit(HWND hDlg, HWND, LPARAM)
{
    if (RatingsDialog::InitControls(hDlg) != 0)
    {
        Win32::CenterWindow(hDlg, GetParent(hDlg));

        return 1;
    }

    char* const pszMessage = g_gameApp.m_stringPool.LoadText(g_gameApp.m_hInstance, STRING_OUT_OF_MEMORY);

    g_gameApp.Error(pszMessage);

    delete[] pszMessage;

    EndDialog(hDlg, -1);

    return 1;
}

// Resource 0x140. The second copy walks 63 entries of a 16-entry array: the original's overrun.

// GLOBAL: HYPERX 0x004b22d8
extern const float g_flRatingStart;

#define PLAYER_COUNT 6

// FUNCTION: HYPERX 0x00419b10
unsigned int __cdecl RatingsDialog::OnCommand(HWND hDlg, int nCommand, HWND hControl, int nNotify)
{
    RatingsDialogState* pState;
    PlayerObject*       pObject;
    float               flValue;
    int                 nPlayer;
    int                 nCategory;

    switch (nCommand)
    {
        case IDCANCEL:
            if (nNotify == BN_CLICKED)
            {
                RatingsDialog::Close(hDlg, 0);
            }

            return 1;

        case IDOK:
            if (nNotify == BN_CLICKED)
            {
                nPlayer = (int)SendMessageA(GetDlgItem(hDlg, IDC_PLAYER_LIST), CB_GETCURSEL, 0, 0);

                pObject = g_pGameEngine->GetPlayer((short)nPlayer);

                flValue = Dialog::GetItemFloat(hDlg, IDC_RATING_VALUE, 0);

                nCategory = (int)SendMessageA(GetDlgItem(hDlg, IDC_CATEGORY_LIST), CB_GETCURSEL, 0, 0);

                pObject->m_stats.m_aStats[RATING_SLOT(nCategory)] = flValue;

                RatingsDialog::Close(hDlg, 1);

                return 1;
            }

            return 1;

        case IDC_PLAYER_LIST:
            if (nNotify == CBN_SELCHANGE)
            {
                pState = (RatingsDialogState*)GetWindowLongA(hDlg, GWL_USERDATA);

                if (pState != 0)
                {
                    nPlayer = (int)SendMessageA(hControl, CB_GETCURSEL, 0, 0);

                    const HWND hCategories = GetDlgItem(hDlg, IDC_CATEGORY_LIST);

                    nCategory = (int)SendMessageA(hCategories, CB_GETCURSEL, 0, 0);

                    SendMessageA(hCategories, CB_RESETCONTENT, 0, 0);

                    for (int nAt = 0; nAt < RATING_COUNT; nAt++)
                    {
                        if (nPlayer < FIRST_NON_PLAYER)
                        {
                            SendMessageA(hCategories, CB_ADDSTRING, 0, (LPARAM)pState->ppNames[nAt]);
                        }
                        else
                        {
                            SendMessageA(hCategories, CB_ADDSTRING, 0, (LPARAM)pState->ppGoalieNames[nAt]);
                        }
                    }

                    SendMessageA(hCategories, CB_SETCURSEL, nCategory, 0);

                    return 1;
                }

                return 1;
            }

            break;

        case IDC_CATEGORY_LIST:
            if (nNotify == CBN_SELCHANGE)
            {
                pState = (RatingsDialogState*)GetWindowLongA(hDlg, GWL_USERDATA);

                if (pState != 0)
                {
                    flValue = Dialog::GetItemFloat(hDlg, IDC_RATING_VALUE, 0);

                    pState->flValue = flValue;

                    pObject = g_pGameEngine->GetPlayer(pState->nPlayer);

                    pObject->m_stats.m_aStats[RATING_SLOT(pState->nCategory)] = flValue;

                    pState->nPlayer =
                        (short)SendMessageA(GetDlgItem(hDlg, IDC_PLAYER_LIST), CB_GETCURSEL, 0, 0);

                    pState->nCategory = (int)SendMessageA(hControl, CB_GETCURSEL, 0, 0);

                    pObject = g_pGameEngine->GetPlayer(pState->nPlayer);

                    Dialog::SetItemText(hDlg, IDC_RATING_VALUE, pObject->m_stats.GetScalar(pState->nCategory));
                }

                return 1;
            }

            break;

        default:
            break;
    }

    return 0;
}

// FUNCTION: HYPERX 0x00419d50
int __cdecl RatingsDialog::InitControls(HWND hDlg)
{
    char** ppGoalieNames;
    int    nPlayer;

    int nResult = 0;

    RatingsDialogState* pState = new RatingsDialogState;

    if (pState != 0)
    {
        pState->ppGoalieNames = 0;
        pState->nPlayer       = 0;
        pState->nCategory     = 0;
        pState->flValue       = g_flRatingStart;

        char** const ppNames = new char*[RATING_COUNT];

        pState->ppNames = ppNames;

        if (ppNames != 0)
        {
            ppGoalieNames = new char*[RATING_COUNT];

            pState->ppGoalieNames = ppGoalieNames;

            if (ppGoalieNames != 0)
            {
                char aszRatings[RATING_COUNT][30] = {
                    // STRING: HYPERX 0x004bbf90
                    "WeightClass",
                    // STRING: HYPERX 0x004bbfb0
                    "Stamina",
                    // STRING: HYPERX 0x004bbfa4
                    "Checking",
                    // STRING: HYPERX 0x004bbf9c
                    "Balance",
                    // STRING: HYPERX 0x004bbf88
                    "Speed",
                    // STRING: HYPERX 0x004bbf7c
                    "ShotPower",
                    // STRING: HYPERX 0x004bbf6c
                    "ShotAccuracy",
                    // STRING: HYPERX 0x004bbf5c
                    "PassAccuracy",
                    // STRING: HYPERX 0x004bbf48
                    "OffensiveAbility",
                    // STRING: HYPERX 0x004bbf38
                    "LongShotEnable",
                    // STRING: HYPERX 0x004bbf2c
                    "Immunity",
                    // STRING: HYPERX 0x004bbf1c
                    "LongPassEnable",
                    // STRING: HYPERX 0x004bbf08
                    "MultiPointSpread",
                    // STRING: HYPERX 0x004bbefc
                    "RokHandling",
                    // STRING: HYPERX 0x004bbef0
                    "ArmoryRange",
                    // STRING: HYPERX 0x004bbee8
                    "Damage",
                    // STRING: HYPERX 0x004bbee0
                    "Gender",
                    // STRING: HYPERX 0x004bbed0
                    "Speedburst0a",
                    // STRING: HYPERX 0x004bbec0
                    "Speedburst0b",
                    // STRING: HYPERX 0x004bbeb0
                    "Speedburst1a",
                    // STRING: HYPERX 0x004bbea0
                    "Speedburst1b",
                    // STRING: HYPERX 0x004bbe90
                    "Speedburst2a",
                    // STRING: HYPERX 0x004bbe80
                    "Speedburst2b",
                    // STRING: HYPERX 0x004bbe70
                    "Speedburst3a",
                    // STRING: HYPERX 0x004bbe60
                    "Speedburst3b",
                    // STRING: HYPERX 0x004bbe50
                    "Speedburst4a",
                    // STRING: HYPERX 0x004bbe40
                    "Speedburst4b",
                    // STRING: HYPERX 0x004bbe38
                    "Bungee0",
                    // STRING: HYPERX 0x004bbe30
                    "Bungee1",
                    // STRING: HYPERX 0x004bbe28
                    "Bungee2",
                    // STRING: HYPERX 0x004bbe20
                    "Pass0",
                    // STRING: HYPERX 0x004bbe18
                    "Pass1",
                    // STRING: HYPERX 0x004bbe10
                    "Pass2",
                    // STRING: HYPERX 0x004bbe00
                    "WinCloseFreeRok",
                    // STRING: HYPERX 0x004bbdec
                    "LoseCloseFreeRok",
                    // STRING: HYPERX 0x004bbddc
                    "WinFarFreeRok",
                    // STRING: HYPERX 0x004bbdcc
                    "LoseFarFreeRok",
                    // STRING: HYPERX 0x004bbdbc
                    "WinOffWingJob",
                    // STRING: HYPERX 0x004bbdac
                    "LoseOffWingJob",
                    // STRING: HYPERX 0x004bbda0
                    "Multiplier0",
                    // STRING: HYPERX 0x004bbd94
                    "Multiplier1",
                    // STRING: HYPERX 0x004bbd88
                    "Turnstile0",
                    // STRING: HYPERX 0x004bbd7c
                    "Turnstile1",
                    // STRING: HYPERX 0x004bbd70
                    "Killball0",
                    // STRING: HYPERX 0x004bbd64
                    "Killball1",
                    // STRING: HYPERX 0x004bbd58
                    "Randomize0",
                    // STRING: HYPERX 0x004bbd4c
                    "Randomize1",
                    // STRING: HYPERX 0x004bbd40
                    "Fighting0",
                    // STRING: HYPERX 0x004bbd34
                    "Fighting1",
                    // STRING: HYPERX 0x004bbd28
                    "Fighting2",
                    // STRING: HYPERX 0x004bbd1c
                    "Fighting3",
                    // STRING: HYPERX 0x004bbd10
                    "Fighting4",
                    // STRING: HYPERX 0x004bbd00
                    "FlyingRepetoire",
                    // STRING: HYPERX 0x004bbcec
                    "HalfpipeRepetoire",
                    // STRING: HYPERX 0x004bbcd8
                    "CarryRokThrowDist",
                    // STRING: HYPERX 0x004bbcc0
                    "WinCarryRokRegroupDist",
                    // STRING: HYPERX 0x004bbca8
                    "LoseCarryRokRegroupDist",
                    // STRING: HYPERX 0x004bbc90
                    "WinAggressionOnOthers",
                    // STRING: HYPERX 0x004bbc6c
                    "LoseAggressionOnOthers",
                    // STRING: HYPERX 0x004bbc5c
                    "SubOutDamage",
                    // STRING: HYPERX 0x004bbc50
                    "SubInDamage",
                    // STRING: HYPERX 0x004bbc40
                    "MultiplierRange",
                    // STRING: HYPERX 0x004bbc34
                    "MinSubIn"
                };

                char aszGoalieRatings[GOALIE_RATING_COUNT][20] = {
                    "WeightClass",
                    // STRING: HYPERX 0x004bbc84
                    "Strength",
                    // STRING: HYPERX 0x004bbc24
                    "ReactionTime",
                    "Balance",
                    "Speed",
                    "PassAccuracy",
                    // STRING: HYPERX 0x004bbc14
                    "Intelligence",
                    // STRING: HYPERX 0x004bbc04
                    "UnderPressure",
                    // STRING: HYPERX 0x004bbbf8
                    "SaveRight",
                    // STRING: HYPERX 0x004bbbec
                    "SaveLeft",
                    // STRING: HYPERX 0x004bbbe0
                    "SaveHigh",
                    // STRING: HYPERX 0x004bbbd8
                    "SaveLow",
                    // STRING: HYPERX 0x004bbbc8
                    "ShieldBlocking",
                    // STRING: HYPERX 0x004bbbb8
                    "CatchingAbility",
                    // STRING: HYPERX 0x004bbbac
                    "Aggression",
                    // STRING: HYPERX 0x004bbba4
                    "Overall"
                };

                char aszPlayers[PLAYER_COUNT][20] = {
                    // STRING: HYPERX 0x004bbb98
                    "RAM (Dr 0)",
                    // STRING: HYPERX 0x004bbb8c
                    "ROM (Dr 1)",
                    // STRING: HYPERX 0x004bbb78
                    "Snake Eyes (Dr 2)",
                    // STRING: HYPERX 0x004bbb64
                    "Crash Bug (Dr 3)",
                    // STRING: HYPERX 0x004bbb50
                    "Tempura (N Goalie)",
                    // STRING: HYPERX 0x004bbb3c
                    "Ace High (S Goalie)"
                };

                const HWND hCategories = GetDlgItem(hDlg, IDC_CATEGORY_LIST);

                for (int nAt = 0; nAt < RATING_COUNT; nAt++)
                {
                    SendMessageA(hCategories, CB_ADDSTRING, 0, (LPARAM)aszRatings[nAt]);

                    char** ppEntry = &pState->ppNames[nAt];

                    *ppEntry = strcpy(
                        new char[strlen(aszRatings[nAt]) + 1], aszRatings[nAt]);

                    char* const pszGoalie = aszGoalieRatings[nAt];

                    char** ppGoalieName = &pState->ppGoalieNames[nAt];

                    *ppGoalieName = strcpy(
                        new char[strlen(pszGoalie) + 1], pszGoalie);
                }

                SendMessageA(hCategories, CB_SETCURSEL, 0, 0);

                SetWindowLongA(hDlg, GWL_USERDATA, (LONG)pState);

                const HWND hPlayers = GetDlgItem(hDlg, IDC_PLAYER_LIST);

                for (nPlayer = 0; nPlayer < PLAYER_COUNT; nPlayer++)
                {
                    SendMessageA(hPlayers, CB_ADDSTRING, 0, (LPARAM)aszPlayers[nPlayer]);
                }

                SendMessageA(hPlayers, CB_SETCURSEL, 0, 0);

                PlayerObject* const pObject = g_pGameEngine->GetPlayer(0);

                Dialog::SetItemText(
                    hDlg, IDC_RATING_VALUE, pObject->m_stats.GetScalar(RATING_WEIGHT_CLASS));

                nResult = 1;
            }
        }
    }

    return nResult;
}

// Two parallel arrays of sixty three strings, walked by byte offset.

// FUNCTION: HYPERX 0x0041b7e0
void __cdecl RatingsDialog::Close(HWND hWnd, INT_PTR nResult)
{
    RatingsDialogState* pState = (RatingsDialogState*)GetWindowLongA(hWnd, GWL_USERDATA);

    if (pState != 0)
    {
        for (int nAt = 0; nAt < RATING_COUNT; nAt++)
        {
            char** ppEntry = &pState->ppNames[nAt];

            if (*ppEntry != 0)
            {
                delete[] *ppEntry;
                *ppEntry = 0;
            }

            ppEntry = &pState->ppGoalieNames[nAt];

            if (*ppEntry != 0)
            {
                delete[] *ppEntry;
                *ppEntry = 0;
            }
        }

        delete[] pState->ppNames;
        delete[] pState->ppGoalieNames;
        delete pState;
    }

    SetWindowLongA(hWnd, GWL_USERDATA, 0);

    EndDialog(hWnd, nResult);
}
