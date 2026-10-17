#include "app/cddrive.h"

bool CdDrive::Supported()
{
    return false;
}

bool CdDrive::IsCDRom(int)
{
    return false;
}

std::string CdDrive::VolumeLabel(int)
{
    return std::string();
}
