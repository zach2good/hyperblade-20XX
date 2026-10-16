// The game's text. It is read once from the player's HYPERX.EXE into a strings file beside it,
// which every later run loads, and which a translation can replace.

#ifndef HYPERBLADE_STRINGTABLE_H
#define HYPERBLADE_STRINGTABLE_H

namespace StringTable
{

// The name of the strings file, in the game's own folder.
constexpr auto FILE_NAME = "hyperblade_20XX_strings.txt";

// Loads the strings file in pszDirectory, writing it from the HYPERX.EXE there first when there
// is none. False when there is neither.
bool Load(const char* pszDirectory);

// An unknown id reads as an empty string, as a failed LoadString did.
const char* Find(unsigned int nID);

} // namespace StringTable

#endif
