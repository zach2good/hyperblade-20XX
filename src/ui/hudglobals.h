#ifndef HYPERBLADE_UI_HUDGLOBALS_H
#define HYPERBLADE_UI_HUDGLOBALS_H

#include "math/vector.h"
#include "graphics/clippoly.h"
#include <stddef.h>

// The clock and quit prompt both HUDs draw.
constexpr auto kClockBuffer        = 0x50;
constexpr auto kMsPerMinute        = 60000;
constexpr auto kQuitPromptInterval = 5000;

// Read only by Quat::ToMatrix, Quat::FromMatrix and SlerpSeg::Init.
// GLOBAL: HYPERX 0x004b2bb0
extern const float g_flQuatOne;

// GLOBAL: HYPERX 0x004b2bb4
extern const float g_flQuatTwo;

// GLOBAL: HYPERX 0x004b15f8
extern const float g_flGravityHalf;

// With the half, (3 - dot) / 2 scales the chaser's speed from one, heading together, to two, head on.
// GLOBAL: HYPERX 0x004b1420
extern const float g_flChaseSpeedBias;

// Frames of velocity the rear view leads its target by.
// GLOBAL: HYPERX 0x004b1434
extern const float g_flRearViewLeadFrames;

// GLOBAL: HYPERX 0x004b1488
extern const float g_flOneThrow;

// Three other addresses hold the same value for other subsystems.
// GLOBAL: HYPERX 0x004b1494
extern const float g_flPiThrow;

// Three and a half, the throw arc gate in PlanAction::LaunchAt.
// GLOBAL: HYPERX 0x004b1510
extern const float g_flThrowStepsCap;

// Half a turn, the spin all three impulse responses give their CRelaxYaw agenda item.
// GLOBAL: HYPERX 0x004b1530
extern const float g_flHitSpin;

// GLOBAL: HYPERX 0x004b1534
extern const float g_flSteerBoxSizeX;

// GLOBAL: HYPERX 0x004b1538
extern const float g_flSteerBoxSizeZ;

// GLOBAL: HYPERX 0x004b1540
extern const float g_flWallBoxSizeX;

// GLOBAL: HYPERX 0x004b1544
extern const float g_flWallBoxSizeZ;

// Added to ObjectDef::m_flAzimuth when it goes negative.
// GLOBAL: HYPERX 0x004b15f4
extern const float g_flDegreesPerTurn;

// Scales PhysicsBody::axis x and y, twice over.
// GLOBAL: HYPERX 0x004b1608
extern const float g_flMovementAxisDecay;

// GLOBAL: HYPERX 0x004b222c
extern const float g_flZeroBone;

// GLOBAL: HYPERX 0x004b2858
extern const float g_flPiCamera;

// GLOBAL: HYPERX 0x004b285c
extern const float g_flDegToRadCamera;

// Minus a hundred, the z the CameraObject constructor first aims at; x and y are nought.
// GLOBAL: HYPERX 0x004b2864
extern const float g_flCameraStartLookZ;

// GLOBAL: HYPERX 0x004b2bd8
extern const float g_flTwoEllipsoid;

// GLOBAL: HYPERX 0x004b2bdc
extern const float g_flThreeEllipsoid;

// GLOBAL: HYPERX 0x004b36fc
extern const float g_flPiPlan;

// The camera keeps its own copy at 0x004b285c.
// GLOBAL: HYPERX 0x004b39ec
extern const float g_flDegToRadPlayer;

// GLOBAL: HYPERX 0x004b3e2c
extern const float g_flZeroPlanState;

// GLOBAL: HYPERX 0x004b40b4
extern const float g_flZeroSpline;

// Two buttons, three bitmap names each: plain, rollover and depressed.
// GLOBAL: HYPERX 0x004c6510
extern const char* g_aConfigButtonArt[2][3];

// A two-word point: the image's own padding aligns these tables to eight.
struct ConfigPoint
{
    int nX;
    int nY;
};

// GLOBAL: HYPERX 0x004c6528
extern ConfigPoint g_aConfigButtonPos[2];

// Unread; the button loop's end bound is this object's address.
struct ConfigOptionLabel
{
    const char* pszName;
    int         nX;
    int         nY;
};

// GLOBAL: HYPERX 0x004c6538
extern ConfigOptionLabel g_aConfigOptionLabels[7];

// GLOBAL: HYPERX 0x004c6590
extern ConfigPoint g_aConfigDisplayPos[7];

// The seven rows again, left arrow then right arrow.
// GLOBAL: HYPERX 0x004c65c8
extern ConfigPoint g_aConfigArrowPos[14];

// GLOBAL: HYPERX 0x004c6638
extern const char* g_pszOptionArrowLeft1;

// GLOBAL: HYPERX 0x004c663c
extern const char* g_pszOptionArrowLeft2;

// GLOBAL: HYPERX 0x004c6640
extern const char* g_pszOptionArrowLeft3;

// GLOBAL: HYPERX 0x004c6644
extern const char* g_pszOptionArrowRight1;

// GLOBAL: HYPERX 0x004c6648
extern const char* g_pszOptionArrowRight2;

// GLOBAL: HYPERX 0x004c664c
extern const char* g_pszOptionArrowRight3;

// Twenty bytes below the live settings, field for field; only the cancel handler reads them.
// GLOBAL: HYPERX 0x004d08f0
extern int g_nOptionsSavedDifficulty;

// GLOBAL: HYPERX 0x004d08f4
extern int g_nOptionsSavedBrutality;

// GLOBAL: HYPERX 0x004d0904
extern int g_nOptionsSavedPeriodLengthMs;

// GLOBAL: HYPERX 0x004d0908
extern int g_nOptionsSavedTeammateMode;

// GLOBAL: HYPERX 0x004d090c
extern int g_nOptionsSavedAttackMode;

// GLOBAL: HYPERX 0x004bcf48
extern const char* g_pszKeyConfigBackground;

// The frames are appended plain, rollover, depressed, which is not the record's order.
struct KeyActionButton
{
    const char* pszNormal;
    const char* pszDepressed;
    const char* pszRollover;
    int         nX;
    int         nY;
};

// GLOBAL: HYPERX 0x004bcf50
extern KeyActionButton g_aKeyActionButtons[12];

// Must follow the button table with no pad: the button loop's end bound lands inside it.
// GLOBAL: HYPERX 0x004bd040
extern ConfigPoint g_aKeyDisplayPos[10];

// The inverse of the table at 0x004b2c38 that the accept case walks.
// GLOBAL: HYPERX 0x004b2c60
extern const int g_anKeyConfigDefaultSlots[10];

// Laid out as the image has them: the icon and button loops bound on the tables that follow.
// GLOBAL: HYPERX 0x004bbfb8
extern const char* g_pszAwardsBackdrop;

struct AwardIcon
{
    const char* pszName;
    int         nX;
    int         nY;
};

// GLOBAL: HYPERX 0x004bbfc0
extern AwardIcon g_aAwardIcons[6];

struct AwardButton
{
    const char* pszNormal;
    const char* pszRollover;
    const char* pszDepressed;
    int         nX;
    int         nY;
};

// GLOBAL: HYPERX 0x004bc008
extern AwardButton g_aAwardButtons[6];

// Adjacent, in the image's order: the bracket loop's end bound is in the next table.
// GLOBAL: HYPERX 0x004c3d70
extern ConfigPoint g_aChampHeaderCells[3];

// GLOBAL: HYPERX 0x004c3d88
extern ConfigPoint g_aChampGridCells[50];

// The constructor's loop builds twelve; the thirteenth is its end bound.
// GLOBAL: HYPERX 0x004c3f18
extern ConfigPoint g_aChampBracketCells[13];

// GLOBAL: HYPERX 0x004c3058
extern ConfigPoint g_aLeagueGridCells[50];

// GLOBAL: HYPERX 0x004c31e8
extern ConfigPoint g_aLeagueHeaderCells[2];

// GLOBAL: HYPERX 0x004d1a80
extern ClipStagingBuffer g_clipPolyStaging;

// GLOBAL: HYPERX 0x004ce9c0
extern unsigned int g_nSWFaceColour;

// GLOBAL: HYPERX 0x004ce9c8
extern FacePolyBuffer g_swFaceBuffer;

#endif
