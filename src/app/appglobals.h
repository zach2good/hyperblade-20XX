// Globals only the app files read. Their definitions, in .data order, are in src/globals.cpp.

#ifndef HYPERBLADE_APPGLOBALS_H
#define HYPERBLADE_APPGLOBALS_H
#include "common/compat.h"

#include "globals.h"

// Set by -s, which starts the game without sound.
// GLOBAL: HYPERX 0x004cb4f4
extern int g_bNoSoundArg;

// GLOBAL: HYPERX 0x004ceec4
extern int g_nFpsTextLength;

// GLOBAL: HYPERX 0x004b3858
extern const char* g_pszScanFormat;

// The one-character scan format the config reader uses to look at a line's first token.
// GLOBAL: HYPERX 0x004b3eec
extern const char* g_pszCharFormat;

// GLOBAL: HYPERX 0x004cef4a
extern char g_bShuttingDown;

// GLOBAL: HYPERX 0x004d0508
extern int g_nInstallLevel;

#endif
