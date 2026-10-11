// Globals with no subsystem header of their own.

#ifndef HYPERBLADE_COMMONGLOBALS_H
#define HYPERBLADE_COMMONGLOBALS_H
#include "common/compat.h"

#include "globals.h"
#include "graphics/surfacepoint.h"

// GLOBAL: HYPERX 0x004cedd0
extern SurfacePoint g_ptHudAnchorC;

// GLOBAL: HYPERX 0x004cedc8
extern SurfacePoint g_ptHudAnchorD;

// GLOBAL: HYPERX 0x004d2180
extern SurfacePoint g_ptPlanEditorOrigin;

// GLOBAL: HYPERX 0x004b3f00
extern const char* const g_pszTokenDelimiters;

// GLOBAL: HYPERX 0x004bcf3c
extern StringHeader* g_pEmptyStringHeader;

// GLOBAL: HYPERX 0x004bcf40
extern char* g_pEmptyStringData;

// The empty string String::Empty assigns when the buffer is locked (a negative reference count).
// GLOBAL: HYPERX 0x004cb7ac
extern char g_szEmptyString[1];

// GLOBAL: HYPERX 0x004b2964
extern const char* g_pszCaptionSeparator;

// Globals the compiler-generated static initialisers read and write.
// GLOBAL: HYPERX 0x004b3860
extern float g_flOneSteering;

// GLOBAL: HYPERX 0x004cb76c
extern float g_flScreenDotsPerInchWidget;

// GLOBAL: HYPERX 0x004cb804
extern float g_flScreenDotsPerInchProjection;

// GLOBAL: HYPERX 0x004cd4a0
extern float g_flScreenDotsPerInchParseUtil;

// GLOBAL: HYPERX 0x004d2bb8
extern float g_flScreenDotsPerInchModelFile;

#endif
