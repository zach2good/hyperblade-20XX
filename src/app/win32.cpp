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

// FUNCTION: HYPERX 0x0048f470
bool Win32::AskYesNo(const char* pszText, const char* pszCaption)
{
    return Window::ShowMessage(pszText, pszCaption, Window::MessageButtons::YES_NO, Window::MessageIcon::INFO) == Window::MessageAnswer::YES;
}
