#include "common/compat.h"
#include "graphics/renderer.h"
#include "anim/animdb.h"
#include <mmsystem.h>
#include <stdlib.h>
#include <string>
#include "globals.h"
#include "engine/gameengine.h"
#include "common/pathstring.h"
#include "planvm/planvm.h"
#include "gameapp.h"
#include "graphics/cgpalette.h"
#include "graphics/gdi.h"
#include "net/netsetupdialog.h"
#include "net/network.h"
#include "graphics/swrend.h"
#include "app/win32.h"
#include "app/window.h"

// Drives count from nought at A.
// FUNCTION: HYPERX 0x004233b0
bool Win32::IsCDRomDrive(int nDrive)
{
    // STRING: HYPERX 0x004bc418
    char szRoot[] = "A:\\";

    szRoot[0] = (char)(szRoot[0] + nDrive);

    return GetDriveTypeA(szRoot) == DRIVE_CDROM;
}

std::string Win32::VolumeLabel(int nDrive)
{
    // STRING: HYPERX 0x004bc418
    char szRoot[] = "A:\\";

    szRoot[0] = (char)(szRoot[0] + nDrive);

    char  szVolume[MAX_PATH];
    char  szFileSystem[MAX_PATH];
    DWORD dwOut;

    if (GetVolumeInformationA(szRoot, szVolume, MAX_PATH, &dwOut, &dwOut, &dwOut, szFileSystem, MAX_PATH) == 0)
    {
        return std::string();
    }

    return std::string(szVolume);
}

// FUNCTION: HYPERX 0x0047ad70
int Win32::GetWindowTextAlloc(HWND hWnd, char** ppszOut)
{
    int nLength = GetWindowTextLengthA(hWnd);

    if (nLength > 0)
    {
        nLength++;

        char* const pszText = new char[nLength];

        *ppszOut = pszText;

        if (pszText != 0)
        {
            GetWindowTextA(hWnd, pszText, nLength);
        }
    }

    return nLength;
}

// FUNCTION: HYPERX 0x0048f470
bool Win32::AskYesNo(const char* pszText, const char* pszCaption)
{
    return Window::ShowMessage(pszText, pszCaption, Window::MessageButtons::YES_NO, Window::MessageIcon::INFO) == Window::MessageAnswer::YES;
}

// FUNCTION: HYPERX 0x0048fb50
void Win32::CenterWindow(HWND hWnd, HWND hParent)
{
    RECT rcWindow;
    RECT rcParent;

    GetWindowRect(hWnd, &rcWindow);

    const int cxWindow = rcWindow.right - rcWindow.left;
    const int cyWindow = rcWindow.bottom - rcWindow.top;

    GetWindowRect(hParent, &rcParent);

    const int cxParent = rcParent.right - rcParent.left;
    const int cyParent = rcParent.bottom - rcParent.top;

    const int cxScreen = GetSystemMetrics(SM_CXSCREEN);
    const int cyScreen = GetSystemMetrics(SM_CYSCREEN);

    int x = rcParent.left + (cxParent - cxWindow) / 2;

    if (x < 0)
    {
        x = 0;
    }
    else if (x + cxWindow > cxScreen)
    {
        x = cxScreen - cxWindow;
    }

    int y = rcParent.top + (cyParent - cyWindow) / 2;

    if (y < 0)
    {
        y = 0;
    }
    else if (y + cyWindow > cyScreen)
    {
        y = cyScreen - cyWindow;
    }

    SetWindowPos(hWnd, 0, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}
