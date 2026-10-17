// The CD-ROM drives, for the original's check for its disc. Windows can look for one; elsewhere
// there are none.

#ifndef HYPERBLADE_CDDRIVE_H
#define HYPERBLADE_CDDRIVE_H

#include <string>

namespace CdDrive
{

// False where the platform has no way to look for a disc.
bool Supported();

// Drives count from nought at A. A drive with no disc has an empty label.
bool        IsCDRom(int nDrive);
std::string VolumeLabel(int nDrive);

} // namespace CdDrive

#endif
