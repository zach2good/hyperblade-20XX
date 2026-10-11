// Globals the engine files share. Most are defined in src/globals.cpp, in the original's .data order.

#ifndef HYPERBLADE_ENGINEGLOBALS_H
#define HYPERBLADE_ENGINEGLOBALS_H
#include "common/compat.h"

#include "globals.h"
#include "engine/reserverows.h"

// One team's lineup card bitmaps; the tournament setup screen shows one at a time.
struct TeamCardNames
{
    const char* pszStartersCard;
    const char* pszScrubsCard;
};

// The one-character string the config readers split a key from its value on.
// GLOBAL: HYPERX 0x004b3f20
extern const char* g_pszEqualsSign;

// GLOBAL: HYPERX 0x004d2170
extern int g_nCountdownLastSecs;

// Half, used to take the midpoint between the watched and the tracked object.
// GLOBAL: HYPERX 0x004b16a8
extern const float g_flCamWatchMidpointScale;

// Each screen remembers where it was entered from, and the builder copies that on replacement.
// GLOBAL: HYPERX 0x004d2758
extern int g_nGameConfigReturnState;

// GLOBAL: HYPERX 0x004d275c
extern int g_nSaveLoadReturnState;

// GLOBAL: HYPERX 0x004d2760
extern int g_nMatchInfoReturnState;

// GLOBAL: HYPERX 0x004d2768
extern class MatchInfoUIScreen* g_pScreenMatchInfo;

// GLOBAL: HYPERX 0x004d276c
extern class SaveLoadUIScreen* g_pScreenSaveLoad;

// GLOBAL: HYPERX 0x004d2770
extern class GameConfigUIScreen* g_pScreenGameConfig;

// The zero the application object gives its three floats.
// GLOBAL: HYPERX 0x004b2968
extern const float g_flAppInitZero;

// GLOBAL: HYPERX 0x004b3e5c
extern const float g_flCollisionPartDefault;

// GLOBAL: HYPERX 0x004c7060
extern const char* g_pszOptionsSection;

// GLOBAL: HYPERX 0x004c7064
extern const char* g_pszAttackModeKey;

// GLOBAL: HYPERX 0x004c7068
extern const char* g_pszTeammateModeKey;

// The hidden-teams cheat's ini section and key; "Potatoe" is the original's spelling.
// GLOBAL: HYPERX 0x004c2820
extern const char* g_pszMiscSection;

// GLOBAL: HYPERX 0x004c2824
extern const char* g_pszPotatoeKey;

// The four player slots in the order the engine walks them: 0, 1, 3 and 4.
// GLOBAL: HYPERX 0x004c7110
extern int g_anTeamSlotOrder[4];

// GLOBAL: HYPERX 0x004d2198
extern unsigned char g_bSuppressRokId;

// Indexed aSeat[1] + aSeat[0] * 4.
// GLOBAL: HYPERX 0x004b3f44
extern ReserveRows g_aFormationTable[16];

// GLOBAL: HYPERX 0x004d096c
extern int g_nSortedListTotal;

// GLOBAL: HYPERX 0x004c4530
extern char g_bMultiplierColoursUnsaved;

// GLOBAL: HYPERX 0x004c4534
extern int g_nMultiplierColourPositive;

// GLOBAL: HYPERX 0x004c4538
extern int g_nMultiplierColourNegative;

// GLOBAL: HYPERX 0x004cfa58
extern short g_wMultiplierRestColour1;

// GLOBAL: HYPERX 0x004cfa5a
extern short g_wMultiplierRestColour2;

// GLOBAL: HYPERX 0x004b39e0
extern const float g_flBaseStatsSentinel;

// GLOBAL: HYPERX 0x004b39e4
extern const float g_flSpeedRateBase;

// GLOBAL: HYPERX 0x004b2c98
extern const float g_flZeroProjection;

// GLOBAL: HYPERX 0x004c8508
extern TeamCardNames g_aTeamCardNames[14];

// A third, near enough. RampObject::InitCollisionPlanes scales by it.
// GLOBAL: HYPERX 0x004b3ee4
extern const float g_flOneThird;

// GLOBAL: HYPERX 0x004d21a4
extern short g_nMaxPendingSeen;

#endif
