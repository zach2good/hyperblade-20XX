#include "common/compat.h"
#include "common/pathstring.h"
#include <stdlib.h>
#include "ui/dialog.h"
#include "app/win32.h"

// 0x004b313c points to the pooled "%f" in .data.

// GLOBAL: HYPERX 0x004b313c
extern const char* const g_pszDialogFloatFormat;

// FUNCTION: HYPERX 0x00440760
BOOL Dialog::SetItemText(HWND hDlg, int nIDDlgItem, float flValue)
{
    String str;

    String::Format(&str, g_pszDialogFloatFormat, flValue);

    return SetWindowTextA(GetDlgItem(hDlg, nIDDlgItem), str.m_pchData);
}

// An empty control reads as the default at 0x004b3140, nought.

// GLOBAL: HYPERX 0x004b3140
extern const float g_flDialogFloatDefault;

// FUNCTION: HYPERX 0x004407f0
float Dialog::GetItemFloat(HWND hDlg, int nIDDlgItem, int* pbValid)
{
    char* pszText;

    float flValue = g_flDialogFloatDefault;
    int   bValid  = 0;

    if (Win32::GetWindowTextAlloc(GetDlgItem(hDlg, nIDDlgItem), &pszText) != 0)
    {
        flValue = (float)atof(pszText);
        bValid  = 1;

        delete[] pszText;
    }

    if (pbValid != 0)
    {
        *pbValid = bValid;
    }

    return flValue;
}
