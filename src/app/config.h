#ifndef HYPERBLADE_CONFIG_H
#define HYPERBLADE_CONFIG_H
#include "common/compat.h"
#include "globals.h"

struct KeyRow;

namespace Config
{

void LoadPreferences(const char* pszFileName);
int  ParseFile(const char* pszData, const char* pszFallbackPath, KeyRow* pRows);
int  IsCommentLine(const char* pszLine);

} // namespace Config

// The unit's first initialiser thunk, and g_strPrefs's atexit registration and destructor thunk.
// SYNTHETIC: HYPERX 0x0046d530
// $E2
// SYNTHETIC: HYPERX 0x0046d570
// $E8
// SYNTHETIC: HYPERX 0x0046d580
// $E7

// g_flNetTracking's initialiser thunk, which shares its name with RokObject's scratch transform.
// SYNTHETIC: HYPERX 0x0046d670
// $E11
// SYNTHETIC: HYPERX 0x0046d650
// $E14

// SYNTHETIC: HYPERX 0x0046d6e0
// $E39

// SYNTHETIC: HYPERX 0x0046d6f0
// $E38

#endif
