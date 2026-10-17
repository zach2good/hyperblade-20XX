#ifndef HYPERBLADE_WIN32_H
#define HYPERBLADE_WIN32_H

#include "common/compat.h"
#include <string>

namespace Win32
{

bool AskYesNo(const char* pszText, const char* pszCaption);
// Drives count from nought at A. A drive with no disc has an empty label.
bool        IsCDRomDrive(int nDrive);
std::string VolumeLabel(int nDrive);

} // namespace Win32

#endif
