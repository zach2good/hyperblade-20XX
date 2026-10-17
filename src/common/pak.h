// 20XX_data.pak: our own assets, the fonts and such, in a zip beside the image.

#ifndef HYPERBLADE_PAK_H
#define HYPERBLADE_PAK_H

#include <string_view>
#include <vector>

namespace Pak
{

constexpr auto FILE_NAME = "20XX_data.pak";

// Reads the pak into memory. False when it is missing or not a zip.
bool Open(const char* pszPath);

// A packed file's bytes by its path in the pak, or nothing when it is absent or will not unpack.
std::vector<unsigned char> Read(std::string_view name);

} // namespace Pak

#endif
