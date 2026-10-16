#include "common/compat.h"
#include "globals.h"
#include "keyconfigstate.h"
#include "dialogids.h"
#include "common/pathstring.h"
#include "common/stringpool.h"
#include "app/gameapp.h"
#include "app/config.h"
#include <new.h>
#include <string.h>
#include <windows.h>
#include "math/point.h"
#include "ui/dialog.h"
#include "ui/keyconfigdialog.h"
#include "engine/object.h"
#include "app/win32.h"
#include "app/stringids.h"
#include "common/fileobj.h"
#include "common/ownedfile.h"

// The Ratings dialog handler in ratingsdialog.cpp is the same, down to the string id.

// FUNCTION: HYPERX 0x0046c300
int __cdecl KeyConfigDialog::OnInitDialog(HWND hDlg, HWND, LPARAM)
{
    if (KeyConfigDialog::Init(hDlg) != 0)
    {
        Win32::CenterWindow(hDlg, GetParent(hDlg));

        return 1;
    }

    char* const pszMessage = g_gameApp.m_stringPool.LoadText(STRING_OUT_OF_MEMORY);

    g_gameApp.Error(pszMessage);

    delete[] pszMessage;

    EndDialog(hDlg, -1);

    return 1;
}

// Resource 0x139. The table copy is 0xf6 bytes for 0xf0 bytes of rows.

// Copied over the module's table when no file has filled it.
// GLOBAL: HYPERX 0x004c4b8c
extern short g_aDefaultKeyBindings[120];

#define FLAG_SHIFT   8
#define FLAG_CONTROL 4

#define SOURCE_NONE      0
#define SOURCE_PRIMARY   1
#define SOURCE_ALTERNATE 2

#define ACTION_COUNT 40
#define KEY_COUNT    59

// FUNCTION: HYPERX 0x0046c370
int __cdecl KeyConfigDialog::Init(HWND hDlg)
{
    KeyConfigState* pState;
    HWND            hList;
    int             nAt;

    pState = new KeyConfigState;

    if (pState != 0)
    {
        char* apszKeys[KEY_COUNT] = {
            // STRING: HYPERX 0x004c4b58
            "Up Arrow",
            // STRING: HYPERX 0x004c4b4c
            "Down Arrow",
            // STRING: HYPERX 0x004c4b40
            "Left Arrow",
            // STRING: HYPERX 0x004c4b34
            "Right Arrow",
            // STRING: HYPERX 0x004c4b28
            "Enter Key",
            // STRING: HYPERX 0x004c4b20
            "Esc Key",
            // STRING: HYPERX 0x004c4b18
            "Tab Key",
            // STRING: HYPERX 0x004c4b0c
            "Space Bar",
            // STRING: HYPERX 0x004c4af8
            "Left Mouse Button",
            // STRING: HYPERX 0x004c4ae4
            "Right Mouse Button",
            // STRING: HYPERX 0x004c4ae0
            "F1",
            // STRING: HYPERX 0x004c4adc
            "F2",
            // STRING: HYPERX 0x004c4ad8
            "F3",
            // STRING: HYPERX 0x004c4ad4
            "F4",
            // STRING: HYPERX 0x004c4ad0
            "F5",
            // STRING: HYPERX 0x004c4acc
            "F6",
            // STRING: HYPERX 0x004c4ac8
            "F7",
            // STRING: HYPERX 0x004c4ac4
            "F8",
            // STRING: HYPERX 0x004c4ac0
            "F9",
            // STRING: HYPERX 0x004c4abc
            "F10",
            // STRING: HYPERX 0x004c4ab8
            "F11",
            // STRING: HYPERX 0x004c4ab4
            "F12",
            // STRING: HYPERX 0x004c4ab0
            "a",
            // STRING: HYPERX 0x004c4aac
            "b",
            // STRING: HYPERX 0x004c4aa8
            "c",
            // STRING: HYPERX 0x004c4aa4
            "d",
            // STRING: HYPERX 0x004c4aa0
            "e",
            // STRING: HYPERX 0x004c4a9c
            "f",
            // STRING: HYPERX 0x004c4a98
            "g",
            // STRING: HYPERX 0x004c4a94
            "h",
            // STRING: HYPERX 0x004c4a90
            "i",
            // STRING: HYPERX 0x004c4a8c
            "j",
            // STRING: HYPERX 0x004c4a88
            "m",
            // STRING: HYPERX 0x004c4a84
            "n",
            // STRING: HYPERX 0x004c4a80
            "o",
            // STRING: HYPERX 0x004c4a7c
            "p",
            // STRING: HYPERX 0x004c4a78
            "q",
            "r",
            // STRING: HYPERX 0x004c4a74
            "s",
            // STRING: HYPERX 0x004c4a70
            "t",
            // STRING: HYPERX 0x004c4a6c
            "x",
            // STRING: HYPERX 0x004c4a68
            "y",
            // STRING: HYPERX 0x004c4a64
            "z",
            // STRING: HYPERX 0x004c4a58
            "NUMPAD 0",
            // STRING: HYPERX 0x004c4a4c
            "NUMPAD 1",
            // STRING: HYPERX 0x004c4a40
            "NUMPAD 2",
            // STRING: HYPERX 0x004c4a34
            "NUMPAD 3",
            // STRING: HYPERX 0x004c4a28
            "NUMPAD 4",
            // STRING: HYPERX 0x004c4a1c
            "NUMPAD 5",
            // STRING: HYPERX 0x004c4a10
            "NUMPAD 6",
            // STRING: HYPERX 0x004c4a04
            "NUMPAD 7",
            // STRING: HYPERX 0x004c49f8
            "NUMPAD 8",
            // STRING: HYPERX 0x004c49ec
            "NUMPAD 9",
            // STRING: HYPERX 0x004c49dc
            "Decimal Point",
            // STRING: HYPERX 0x004c49d4
            "Plus",
            // STRING: HYPERX 0x004c49cc
            "Minus",
            // STRING: HYPERX 0x004c49c8
            "*",
            // STRING: HYPERX 0x004c49c4
            "/",
            // STRING: HYPERX 0x004c49b8
            "UnMapped"
        };

        // The original reserves a slot past the last name and clears it.
        char* apszActions[ACTION_COUNT + 1] = {
            // STRING: HYPERX 0x004c49a4
            "Accelerate/Forward",
            // STRING: HYPERX 0x004c498c
            "Decelerate/Backwards",
            // STRING: HYPERX 0x004c4980
            "Turn Left",
            // STRING: HYPERX 0x004c4974
            "Turn Right",
            // STRING: HYPERX 0x004c4964
            "Spin 180 Left",
            // STRING: HYPERX 0x004c4954
            "Spin 180 Right",
            // STRING: HYPERX 0x004c4940
            "Spin 180 to Forward",
            // STRING: HYPERX 0x004c4934
            "Shoot Rok",
            // STRING: HYPERX 0x004c492c
            "Test",
            // STRING: HYPERX 0x004c4920
            "Look Left",
            // STRING: HYPERX 0x004c4914
            "Look Right",
            // STRING: HYPERX 0x004c4904
            "Look Forward",
            // STRING: HYPERX 0x004c48f8
            "GamePad A",
            // STRING: HYPERX 0x004c48ec
            "GamePad B",
            // STRING: HYPERX 0x004c48e0
            "GamePad C",
            // STRING: HYPERX 0x004c48d4
            "GamePad D",
            // STRING: HYPERX 0x004c48c8
            "Pause Game",
            // STRING: HYPERX 0x004c48b8
            "Move Eye Left",
            // STRING: HYPERX 0x004c48a8
            "Move Eye Right",
            // STRING: HYPERX 0x004c4898
            "Move Eye Back",
            // STRING: HYPERX 0x004c4884
            "Move Eye Forward",
            // STRING: HYPERX 0x004c4874
            "Move Eye Down",
            // STRING: HYPERX 0x004c4868
            "Move Eye Up",
            // STRING: HYPERX 0x004c485c
            "Swap Views",
            // STRING: HYPERX 0x004c484c
            "Force Attach",
            // STRING: HYPERX 0x004c4840
            "Quit Game",
            // STRING: HYPERX 0x004c4834
            "Pass Rok",
            // STRING: HYPERX 0x004c4820
            "Swap Watched Obj",
            // STRING: HYPERX 0x004c480c
            "Swap Tracked Obj",
            // STRING: HYPERX 0x004c4800
            "Speed Burst",
            // STRING: HYPERX 0x004c47f4
            "Kick/Punch",
            // STRING: HYPERX 0x004c47ec
            "Zoom In",
            // STRING: HYPERX 0x004c47e0
            "Zoom Out",
            // STRING: HYPERX 0x004c47cc
            "Control Teammate",
            // STRING: HYPERX 0x004c47bc
            "Alternate Shoot",
            // STRING: HYPERX 0x004c47ac
            "Alternate Pass",
            // STRING: HYPERX 0x004c479c
            "Engage Opponent",
            // STRING: HYPERX 0x004c478c
            "Become Goalie",
            // STRING: HYPERX 0x004c4778
            "Shorten Cam Bungee",
            // STRING: HYPERX 0x004c4764
            "Lengthen Cam Bungee"
        };

        // One row more than the table holds.
        pState->pSaved = new KeyRow[ACTION_COUNT + 1];

        pState->nSource = SOURCE_NONE;

        pState->pszPrimary = strcpy(
            // STRING: HYPERX 0x004c4750
            new char[strlen("Data\\DftKeybd.cfg") + 1],
            "Data\\DftKeybd.cfg");

        pState->pszAlternate = strcpy(
            // STRING: HYPERX 0x004c473c
            new char[strlen("Data\\DftStick.cfg") + 1],
            "Data\\DftStick.cfg");

        SetWindowLongPtrA(hDlg, GWLP_USERDATA, (LONG_PTR)pState);

        if (g_aKeyRows[0].cFlags == 0)
        {
            memcpy(g_aKeyRows, g_aDefaultKeyBindings, sizeof(g_aKeyRows));
        }

        memcpy(pState->pSaved, g_aKeyRows, sizeof(g_aKeyRows));

        hList = GetDlgItem(hDlg, IDC_ACTION_COMBO);

        for (nAt = 0; nAt < ACTION_COUNT; nAt++)
        {
            SendMessageA(hList, CB_ADDSTRING, 0, (LPARAM)apszActions[nAt]);
        }

        SendMessageA(hList, CB_SETCURSEL, 0, 0);

        hList = GetDlgItem(hDlg, IDC_KEY_COMBO);

        for (nAt = 0; nAt < KEY_COUNT; nAt++)
        {
            SendMessageA(hList, CB_ADDSTRING, 0, (LPARAM)apszKeys[nAt]);
        }

        SendMessageA(hList, CB_SETCURSEL, g_aKeyRows[0].nIndex, 0);

        if ((g_aKeyRows[0].cFlags & FLAG_SHIFT) != 0)
        {
            SendMessageA(GetDlgItem(hDlg, IDC_SHIFT), BM_SETCHECK, 1, 0);
        }
        else if ((g_aKeyRows[0].cFlags & FLAG_CONTROL) != 0)
        {
            SendMessageA(GetDlgItem(hDlg, IDC_CONTROL), BM_SETCHECK, 1, 0);
        }

        if (g_strPrefs.m_pchData == pState->pszPrimary)
        {
            pState->nSource = SOURCE_PRIMARY;
        }

        if (g_strPrefs.m_pchData == pState->pszAlternate)
        {
            pState->nSource = SOURCE_ALTERNATE;
        }

        if (pState->nSource == SOURCE_PRIMARY)
        {
            SendMessageA(GetDlgItem(hDlg, IDC_PRIMARY), BM_SETCHECK, 1, 0);

            return 1;
        }

        if (pState->nSource == SOURCE_ALTERNATE)
        {
            SendMessageA(GetDlgItem(hDlg, IDC_ALTERNATE), BM_SETCHECK, 1, 0);

            return 1;
        }

        SetWindowTextA(GetDlgItem(hDlg, IDC_FILE_LABEL), g_strPrefs.m_pchData);

        return 1;
    }

    return 0;
}

// A second pointer to the empty default name.
// GLOBAL: HYPERX 0x004b3854
extern char* g_pszKeyConfigDefaultName;

#define FLAG_SET 1

// The check boxes toggle: a clear box XORs the flag, so a clear box on a clear flag sets it.
// FUNCTION: HYPERX 0x0046ca00
unsigned int __cdecl KeyConfigDialog::OnCommand(HWND hDlg, int nCommand, HWND hControl, int nNotify)
{
    int         nRow;
    const char* pszName;
    int         nLength;

    switch (nCommand)
    {
        case IDCANCEL:
            if (nNotify == BN_CLICKED)
            {
                memcpy(g_aKeyRows,
                       ((KeyConfigState*)GetWindowLongPtrA(hDlg, GWLP_USERDATA))->pSaved,
                       sizeof(g_aKeyRows));

                KeyConfigDialog::CloseDialog(hDlg, 0);
            }

            return 1;

        case IDOK:
            if (nNotify == BN_CLICKED)
            {
                if (SendMessageA(GetDlgItem(hDlg, IDC_SAVE_ON_EXIT), BM_GETCHECK, 0, 0) != 0)
                {
                    Config::SavePreferences(hDlg, g_strPrefs.m_pchData);
                }

                KeyConfigDialog::CloseDialog(hDlg, 1);
            }

            return 1;

        case IDC_ACTION_COMBO:
            if (nNotify == CBN_SELCHANGE)
            {
                nRow = (int)SendMessageA(hControl, CB_GETCURSEL, 0, 0);

                SendMessageA(GetDlgItem(hDlg, IDC_KEY_COMBO), CB_SETCURSEL, g_aKeyRows[nRow].nIndex, 0);
                SendMessageA(GetDlgItem(hDlg, IDC_SHIFT), BM_SETCHECK, (g_aKeyRows[nRow].cFlags & FLAG_SHIFT) != 0, 0);
                SendMessageA(GetDlgItem(hDlg, IDC_CONTROL), BM_SETCHECK, (g_aKeyRows[nRow].cFlags & FLAG_CONTROL) != 0, 0);
            }

            return 1;

        case IDC_DEFAULTS:
            if (nNotify == BN_CLICKED)
            {
                KeyConfigDialog::SaveKeysAs(hDlg, g_aKeyRows);

                SetWindowTextA(GetDlgItem(hDlg, IDC_FILE_LABEL), g_strPrefs.m_pchData);
            }

            return 1;

        case IDC_KEY_COMBO:
            if (nNotify == CBN_SELCHANGE)
            {
                int aKeys[KEY_COUNT] = {
                    VK_UP, VK_DOWN, VK_LEFT, VK_RIGHT, VK_RETURN, VK_ESCAPE, VK_TAB, VK_SPACE, VK_LBUTTON, VK_RBUTTON, VK_F1, VK_F2, VK_F3, VK_F4, VK_F5, VK_F6, VK_F7, VK_F8, VK_F9, VK_F10, VK_F11, VK_F12, 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'X', 'Y', 'Z', VK_NUMPAD0, VK_NUMPAD1, VK_NUMPAD2, VK_NUMPAD3, VK_NUMPAD4, VK_NUMPAD5, VK_NUMPAD6, VK_NUMPAD7, VK_NUMPAD8, VK_NUMPAD9, VK_DECIMAL, VK_ADD, VK_SUBTRACT, VK_MULTIPLY, VK_DIVIDE, 0
                };

                nRow           = (int)SendMessageA(GetDlgItem(hDlg, IDC_ACTION_COMBO), CB_GETCURSEL, 0, 0);
                const int nKey = (int)SendMessageA(hControl, CB_GETCURSEL, 0, 0);

                g_aKeyRows[nRow].cFlags = FLAG_SET;

                if (SendMessageA(GetDlgItem(hDlg, IDC_SHIFT), BM_GETCHECK, 0, 0) != 0)
                {
                    g_aKeyRows[nRow].cFlags |= FLAG_SHIFT;
                }

                if (SendMessageA(GetDlgItem(hDlg, IDC_CONTROL), BM_GETCHECK, 0, 0) != 0)
                {
                    g_aKeyRows[nRow].cFlags |= FLAG_CONTROL;
                }

                g_aKeyRows[nRow].nKey   = (unsigned short)aKeys[nKey];
                g_aKeyRows[nRow].nIndex = (unsigned short)nKey;

                ((KeyConfigState*)GetWindowLongPtrA(hDlg, GWLP_USERDATA))->nSource = SOURCE_NONE;

                SendMessageA(GetDlgItem(hDlg, IDC_PRIMARY), BM_SETCHECK, 0, 0);
                SendMessageA(GetDlgItem(hDlg, IDC_ALTERNATE), BM_SETCHECK, 0, 0);
            }

            return 1;

        case IDC_CLEAR:
            if (nNotify == BN_CLICKED)
            {
                KeyConfigDialog::ClearKeys(hDlg, g_aKeyRows);

                ((KeyConfigState*)GetWindowLongPtrA(hDlg, GWLP_USERDATA))->nSource = SOURCE_NONE;

                SendMessageA(GetDlgItem(hDlg, IDC_PRIMARY), BM_SETCHECK, 0, 0);
                SendMessageA(GetDlgItem(hDlg, IDC_ALTERNATE), BM_SETCHECK, 0, 0);
                SetWindowTextA(GetDlgItem(hDlg, IDC_FILE_LABEL), g_strPrefs.m_pchData);
                SendMessageA(GetDlgItem(hDlg, IDC_ACTION_COMBO), CB_SETCURSEL, 0, 0);
                SendMessageA(GetDlgItem(hDlg, IDC_KEY_COMBO), CB_SETCURSEL, g_aKeyRows[0].nIndex, 0);
            }

            return 1;

        case IDC_SHIFT:
            if (nNotify == BN_CLICKED)
            {
                if (SendMessageA(hControl, BM_GETCHECK, 0, 0) != 0)
                {
                    nRow = (int)SendMessageA(GetDlgItem(hDlg, IDC_ACTION_COMBO), CB_GETCURSEL, 0, 0);
                    g_aKeyRows[nRow].cFlags |= FLAG_SHIFT;
                }
                else
                {
                    nRow = (int)SendMessageA(GetDlgItem(hDlg, IDC_ACTION_COMBO), CB_GETCURSEL, 0, 0);
                    g_aKeyRows[nRow].cFlags ^= FLAG_SHIFT;
                }

                ((KeyConfigState*)GetWindowLongPtrA(hDlg, GWLP_USERDATA))->nSource = SOURCE_NONE;

                SendMessageA(GetDlgItem(hDlg, IDC_PRIMARY), BM_SETCHECK, 0, 0);
                SendMessageA(GetDlgItem(hDlg, IDC_ALTERNATE), BM_SETCHECK, 0, 0);
            }

            return 1;

        case IDC_CONTROL:
            if (nNotify == BN_CLICKED)
            {
                if (SendMessageA(hControl, BM_GETCHECK, 0, 0) != 0)
                {
                    nRow = (int)SendMessageA(GetDlgItem(hDlg, IDC_ACTION_COMBO), CB_GETCURSEL, 0, 0);
                    g_aKeyRows[nRow].cFlags |= FLAG_CONTROL;
                }
                else
                {
                    nRow = (int)SendMessageA(GetDlgItem(hDlg, IDC_ACTION_COMBO), CB_GETCURSEL, 0, 0);
                    g_aKeyRows[nRow].cFlags ^= FLAG_CONTROL;
                }

                ((KeyConfigState*)GetWindowLongPtrA(hDlg, GWLP_USERDATA))->nSource = SOURCE_NONE;

                SendMessageA(GetDlgItem(hDlg, IDC_PRIMARY), BM_SETCHECK, 0, 0);
                SendMessageA(GetDlgItem(hDlg, IDC_ALTERNATE), BM_SETCHECK, 0, 0);
            }

            return 1;

        case IDC_PRIMARY:
            if (nNotify == BN_CLICKED && SendMessageA(hControl, BM_GETCHECK, 0, 0) != 0)
            {
                Config::ParseFile(hDlg,
                                  ((KeyConfigState*)GetWindowLongPtrA(hDlg, GWLP_USERDATA))->pszPrimary,
                                  ((KeyConfigState*)GetWindowLongPtrA(hDlg, GWLP_USERDATA))->pszPrimary,
                                  g_aKeyRows);

                SendMessageA(GetDlgItem(hDlg, IDC_ACTION_COMBO), CB_SETCURSEL, 0, 0);
                SendMessageA(GetDlgItem(hDlg, IDC_KEY_COMBO), CB_SETCURSEL, g_aKeyRows[0].nIndex, 0);

                ((KeyConfigState*)GetWindowLongPtrA(hDlg, GWLP_USERDATA))->nSource = SOURCE_PRIMARY;

                pszName = ((KeyConfigState*)GetWindowLongPtrA(hDlg, GWLP_USERDATA))->pszPrimary;
                if (pszName == 0)
                {
                    nLength = 0;
                }
                else
                {
                    nLength = lstrlenA(pszName);
                }

                g_strPrefs.Assign(nLength, pszName);

                SetWindowTextA(GetDlgItem(hDlg, IDC_FILE_LABEL), g_pszKeyConfigDefaultName);
            }

            return 1;

        case IDC_ALTERNATE:
            if (nNotify == BN_CLICKED && SendMessageA(hControl, BM_GETCHECK, 0, 0) != 0)
            {
                Config::ParseFile(hDlg,
                                  ((KeyConfigState*)GetWindowLongPtrA(hDlg, GWLP_USERDATA))->pszAlternate,
                                  ((KeyConfigState*)GetWindowLongPtrA(hDlg, GWLP_USERDATA))->pszPrimary,
                                  g_aKeyRows);

                SendMessageA(GetDlgItem(hDlg, IDC_ACTION_COMBO), CB_SETCURSEL, 0, 0);
                SendMessageA(GetDlgItem(hDlg, IDC_KEY_COMBO), CB_SETCURSEL, g_aKeyRows[0].nIndex, 0);

                ((KeyConfigState*)GetWindowLongPtrA(hDlg, GWLP_USERDATA))->nSource = SOURCE_ALTERNATE;

                pszName = ((KeyConfigState*)GetWindowLongPtrA(hDlg, GWLP_USERDATA))->pszPrimary;
                if (pszName == 0)
                {
                    nLength = 0;
                }
                else
                {
                    nLength = lstrlenA(pszName);
                }

                g_strPrefs.Assign(nLength, pszName);

                SetWindowTextA(GetDlgItem(hDlg, IDC_FILE_LABEL), g_pszKeyConfigDefaultName);
            }

            return 1;

        default:
            return 0;
    }
}

// FUNCTION: HYPERX 0x0046d320
void __cdecl KeyConfigDialog::CloseDialog(HWND hWnd, INT_PTR nResult)
{
    KeyConfigState* const pState = (KeyConfigState*)GetWindowLongPtrA(hWnd, GWLP_USERDATA);

    if (pState != 0)
    {
        delete[] pState->pSaved;

        if (pState->pszPrimary != 0)
        {
            delete[] pState->pszPrimary;
            pState->pszPrimary = 0;
        }

        if (pState->pszAlternate != 0)
        {
            delete[] pState->pszAlternate;
            pState->pszAlternate = 0;
        }

        delete pState;
    }

    SetWindowLongPtrA(hWnd, GWLP_USERDATA, 0);

    EndDialog(hWnd, nResult);
}

// FUNCTION: HYPERX 0x0042bcd0
int __cdecl KeyConfigDialog::SaveKeysAs(HWND, KeyRow*)
{
    return 0;
}

// The buffer handed to the writer is allocated and never filled; the original does the same.
// FUNCTION: HYPERX 0x0046d3a0
int Config::SavePreferences(HWND hWnd, const char* pszText)
{
    OwnedFile file;

    // STRING: HYPERX 0x004bba9c
    PathString strPath(String::Concat(g_gameApp.m_strExeDir.m_str, "Prefs.txt").m_pchData,
                       MAX_PATH);

    if (file.m_file.Open(strPath.m_str.m_pchData, FILE_MODE_WRITE) != 0)
    {
        char* const pszLine = new char[strlen(pszText)];

        const int nWritten = file.m_file.PrintFormatted(pszLine);

        delete[] pszLine;

        file.m_file.CloseAndTest();

        if (nWritten == 0)
        {
            // STRING: HYPERX 0x004c4b64
            g_gameApp.Fatal("Zero bytes written to Preferences!");
        }

        return 1;
    }

    g_gameApp.ReportLastError(hWnd);

    return 0;
}
