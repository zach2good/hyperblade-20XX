#include "app/cddrive.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

bool CdDrive::Supported()
{
    return true;
}

// FUNCTION: HYPERX 0x004233b0
bool CdDrive::IsCDRom(int nDrive)
{
    // STRING: HYPERX 0x004bc418
    char szRoot[] = "A:\\";

    szRoot[0] = (char)(szRoot[0] + nDrive);

    return GetDriveTypeA(szRoot) == DRIVE_CDROM;
}

std::string CdDrive::VolumeLabel(int nDrive)
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
