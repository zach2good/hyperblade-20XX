// Globals only the ui files read. Their definitions, in .data order, are in src/globals.cpp.

#ifndef HYPERBLADE_UIGLOBALS_H
#define HYPERBLADE_UIGLOBALS_H
#include "common/compat.h"

#include "globals.h"

// GLOBAL: HYPERX 0x004c6650
extern const char* g_apAttackModeBitmapNames[2];

// GLOBAL: HYPERX 0x004c6658
extern const char* g_apTeammateModeBitmapNames[2];

// GLOBAL: HYPERX 0x004d08c8
extern char g_szSfxBarName[20];

// GLOBAL: HYPERX 0x004d08dc
extern char g_szMusicBarName[20];

// Read only by the options screen's cancel handler.
// GLOBAL: HYPERX 0x004d08f8
extern int g_nOptionsSavedCdVolume;

// GLOBAL: HYPERX 0x004d08fc
extern int g_nOptionsSavedSfxVolume;

// GLOBAL: HYPERX 0x004d0910
extern int g_nOptionDifficulty;

// GLOBAL: HYPERX 0x004d0914
extern int g_nOptionBrutality;

// GLOBAL: HYPERX 0x004d0918
extern int g_nOptionMusicVolume;

// GLOBAL: HYPERX 0x004d091c
extern int g_nOptionSfxVolume;

// GLOBAL: HYPERX 0x004d0920
extern int g_nOptionPeriodLength;

// GLOBAL: HYPERX 0x004d0924
extern int g_nPeriodLengthMs;

// GLOBAL: HYPERX 0x004d0928
extern int g_nOptionTeammateMode;

// GLOBAL: HYPERX 0x004d092c
extern int g_nOptionAttackMode;

// The index one past the last credit page.
// GLOBAL: HYPERX 0x004c1ed8
extern int g_nCreditPageCount;

// GLOBAL: HYPERX 0x004c6854
extern char g_szAwayScore[32];

// GLOBAL: HYPERX 0x004c6874
extern char g_szHomeScore[32];

// GLOBAL: HYPERX 0x004c6894
extern int g_bHomeScoreDirty;

// GLOBAL: HYPERX 0x004c6898
extern int g_bAwayScoreDirty;

// GLOBAL: HYPERX 0x004d178c
extern int g_nHomeScoreLength;

// GLOBAL: HYPERX 0x004d1790
extern int g_nAwayScoreLength;

// A ring of ten lines, with the index of the next to write.
// GLOBAL: HYPERX 0x004d1178
extern int g_anDebugLineLength[10];

// GLOBAL: HYPERX 0x004d11a0
extern char g_aDebugLineText[10][150];

// GLOBAL: HYPERX 0x004d1780
extern short g_nDebugLineNext;

// GLOBAL: HYPERX 0x004bbfbc
extern const char* g_pszAwardsBackground;

// Per award: the trophy bitmap, and the prefix of every roster portrait's name.
// GLOBAL: HYPERX 0x004bc080
extern const char* g_apszAwardTrophy[6];

// GLOBAL: HYPERX 0x004bc098
extern const char* g_apszAwardStripPrefix[6];

// GLOBAL: HYPERX 0x004bddb0
extern int g_nMatchInfoBitmapsUnloaded;

// GLOBAL: HYPERX 0x004c2828
extern const char* g_pszIntroBackground;

// Three training backgrounds by difficulty, then the five credit pages.
// GLOBAL: HYPERX 0x004c1eb8
extern const char* g_apszTrainingBackgrounds[8];

constexpr auto kTrainingCreditFirst = 3;

// Nothing reads it.
// GLOBAL: HYPERX 0x004c67a4
extern short g_wUnusedBesidePalette;

// Three means a palette is created; anything else is not palettised.
// GLOBAL: HYPERX 0x004c67a6
extern short g_wPaletteMode;

// GLOBAL: HYPERX 0x004d177c
extern int g_nSavedCursorState;

#endif
