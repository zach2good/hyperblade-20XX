// Definitions for the globals declared in globals.h, in the original's .data order.

#include "common/compat.h"
#include "common/clock.h"
#include "graphics/bitmapformat.h"
#include "planvm/planaction.h"
#include "graphics/modelfile.h"
#include "engine/gameengine.h"
#include "engine/cameraobject.h"
#include "globals.h"
#include "app/frametimer.h"
#include "sound/audiomgr.h"
#include "input/cheatstate.h"
#include "app/gameapp.h"
#include "anim/animglobals.h"
#include "app/appglobals.h"
#include "common/commonglobals.h"
#include "engine/engineglobals.h"
#include "graphics/graphicsglobals.h"
#include "input/inputglobals.h"
#include "math/mathglobals.h"
#include "physics/physicsglobals.h"
#include "planvm/planvmglobals.h"
#include "ui/uiglobals.h"
#include "ui/torchbitmaps.h"
#include "ui/pausemenu.h"
#include "ui/screens/tournamentsetupuiscreen.h"
#include "ui/saveload.h"
#include "ui/screens/saveloaduiscreen.h"
#include "planvm/planvm.h"
#include "graphics/renderer.h"
#include "ui/infobarbase.h"
#include "graphics/colorpalette.h"
#include "common/pathstring.h"
#include "net/session.h"
#include "planvm/agendaitem.h"
#include "engine/cspeedburst.h"
#include "ui/hudglobals.h"
#include "planvm/damagegate.h"
#include "physics/upaxis.h"
#include "math/vector.h"
#include "engine/stationaryplanrecord.h"
#include "engine/objectclasses.h"
#include "physics/physicsbody.h"
#include "planvm/planopcodetable.h"

char                g_szFpsText[100];
unsigned char       g_bIsWindowCreated;
int                 g_nPrevEngineState;
RenderFlags         g_renderFlags = { 1, 1 };
Clock::Milliseconds g_frameGameLogicStart;
InfoBarBase*        g_pRenderer;
struct GameEngine*  g_pGameEngine;
int                 g_bNoSoundArg;
int                 g_nSplashFrame;
ColorPalette*       g_pColorPalette;
const char* const   g_pszModelExtension       = "wbm";
const char* const   g_pszWaveExtension        = "wav";
const float         g_flCamWatchMidpointScale = 0.5f;
FileVersion         g_modelVersion            = { 1, 7, 0, 0, 0 };
GameApp             g_gameApp(2);
AudioMgr*           g_pAudioMgr;
// The reserves for each pair of seats, indexed aSeat[1] + aSeat[0] * 4. Minus one marks a pair with no formation.
ReserveRows g_aFormationTable[16] = {
    { { -1, -1 } },
    { { 2, 3 } },
    { { 1, 3 } },
    { { 1, 2 } },
    { { 2, 3 } },
    { { -1, -1 } },
    { { 0, 3 } },
    { { 0, 2 } },
    { { 1, 3 } },
    { { 0, 3 } },
    { { -1, -1 } },
    { { 0, 1 } },
    { { 1, 2 } },
    { { 0, 2 } },
    { { 0, 1 } },
    { { -1, -1 } }
};
Clock::Milliseconds  g_announcerLine;
StationaryPlanRecord g_aStationaryPlans[kStationarySlots] = {};
int                  g_aStationarySlots[kStationarySlots] = { 0 };
int                  g_nStationaryCount;

// STRING: HYPERX 0x004c724c
const char*               g_pszEqualsSign = "=";
unsigned char             g_bNetworkGame;
unsigned char             g_bShowLightingInfo;
int                       g_bSkipAssetValidation;
int                       g_bModemPlay;
class CD3DTextureManager* g_pTexManager;
int                       g_nCountdownLastSecs;
int                       g_nSavedCursorVisible;
const char* const         g_pszPbnExtension    = "pbn";
const char* const         g_pszTokenDelimiters = "	";
int                       g_nPlanSequenceStep;
const char* const         g_pszPlanVmEmptyName = "";
char                      g_bShuttingDown;
class KeyboardDeviceObj*  g_pInputDevice;
unsigned char             g_bRightEdgeUsed;
unsigned char             g_bLeftEdgeUsed;
short                     g_wSpanYMax;
short                     g_wSpanYMin;
int                       g_nRadarLineColour;
int                       g_nRadarBlipColourRok;
int                       g_nRadarBlipColourDromer3;
int                       g_nRadarBlipColourDromer1;
int                       g_nRadarBlipColourDromer2;
int                       g_nRadarBlipColourDromer0;
unsigned char             g_bEdgeWalked;
int                       g_nFillColour;
class Screen*             g_pActiveScreen;
class MessageQueue*       g_pMessageQueue;
class CheatState          g_cheatState;
int                       g_bCursorHidden = 1;
int                       g_bShadedDromeModels;
GameOptions*              g_pGameOptions;
int                       g_bBackBufferReady;
unsigned char             g_cMdmTexLoaded;
int                       g_nSurfacePitchCopy;
int                       g_nViewportOriginY;
int                       g_nViewportOriginX;
unsigned char*            g_pBackBufferBitsCopy;
// Full screen by default; -w turns it off.
int g_bFullScreen = 1;
int g_nBackBufferHeight;
int g_nBackBufferWidth;

CameraObject*                   g_pScene;
int                             g_bPerspectiveTexturing = 1;
MdmDrawTexPolyFn                g_pfnMdmDrawTexPoly;
Handle<struct MdmTexLibraryTag> g_hMdmTexDll;
// Initialised to keep source order: SoftTex::Load's palette loop bounds one past the array.
PaletteEntry              g_aSoftTexPalette[256] = {};
class SoftTex*            g_pSoftTexHead         = nullptr;
int                       g_nGameConfigReturnState;
int                       g_nSaveLoadReturnState;
int                       g_nMatchInfoReturnState;
class MatchInfoUIScreen*  g_pScreenMatchInfo;
class SaveLoadUIScreen*   g_pScreenSaveLoad;
class GameConfigUIScreen* g_pScreenGameConfig;
MdmInitDrawPolyFn         g_pfnMdmInitDrawPoly;
int                       g_bBilinearFilter;
int                       g_nFpsTextLength;
extern const float        g_flOne              = 1.0f;
extern const float        g_flDegToRad         = 0.017453292f;
extern const float        g_flHalfPi           = 1.5707964f;
extern const float        g_flLightAzimuth180  = 180.0f;
extern const float        g_flLightAzimuth54   = 54.0f;
extern const float        g_flLightAzimuth303  = 303.0f;
extern const char* const  g_pszEmptyName       = "";
extern const char* const  g_pszDefaultPlanName = "";
LightDir                  g_lightDirs[3]       = {
    { 0.0f, { 0.0f, 0.0f, 1.0f } },
    { 0.0f, { 0.0f, 0.0f, 1.0f } },
    { 0.0f, { 0.0f, 0.0f, 1.0f } }
};
Vec3                g_vLightDirExtra;
const char*         g_pszScanFormat       = "%s";
extern const double g_dNormalizeEpsilon   = 1e-07;
extern const float  g_flZero              = 0.0f;
extern const float  g_flGravityGameEngine = 32.0f;
int                 g_bSessionActive;
GameSession*        g_pGameSession;
RenderOptions       g_renderOptions;
short               g_nMaxPendingSeen;
int                 g_nSavedCursorState;
short               g_wActiveScoreLine;
int                 g_nPlayerMode;
unsigned char       g_bSuppressRokId;
// STRING: HYPERX 0x004bc410
const char*        g_pszCaptionSeparator = " ";
unsigned char      g_bDragOff;
unsigned short*    g_pRightStack;
unsigned short*    g_pLeftStack;
int                g_nPrevWidth;
int                g_nPrevHeight;
char               g_szAwayScore[32] = "Red: %d";
char               g_szHomeScore[32] = "Blue: %d";
int                g_bHomeScoreDirty = 1;
int                g_bAwayScoreDirty = 1;
int                g_nHomeScoreLength;
int                g_nAwayScoreLength;
unsigned char      g_bVictimCam;
KeyRow             g_aKeyRows[40];
float              g_flSolarWindFactor      = 30.0f;
float              g_flSteadyCamBuffer      = 0.9f;
float              g_flCameraLookHeight     = 2.0f;
float              g_flCameraTrackingHeight = 0.5f;
float              g_flCameraMinHeight      = 1.0f;
unsigned char      g_bScanningModesEnabled;
extern const float g_flZeroAgendaItem = 0.0f;
PooledOwnerRoot*   g_apAgendaItemPools[Agenda_Count + 1];
PooledOwnerRoot*   g_apBehaviorRegistry[27];
int                g_nSortedListTotal;
short              g_wMultiplierRestColour1;
short              g_wMultiplierRestColour2;
const float        g_flBaseStatsSentinel = 1.0f;
const float        g_flSpeedRateBase     = 2.0f;
// The eight match-start markers Game::InitMatchMarkers derives, sixteen bytes apart.
// GLOBAL: HYPERX 0x004d1960
Vec3 g_vMarkerGoalMouthSouth;
// GLOBAL: HYPERX 0x004d1970
Vec3 g_vMarkerGoalMouthNorth;
// GLOBAL: HYPERX 0x004d1980
Vec3 g_vMarkerSurfaceCentre;
// GLOBAL: HYPERX 0x004d1990
Vec3 g_vMarkerSurfaceYMinus;
// GLOBAL: HYPERX 0x004d19a0
Vec3 g_vMarkerSurfaceYPlus;
// GLOBAL: HYPERX 0x004d19b0
Vec3 g_vMarkerGoalSouth;
// GLOBAL: HYPERX 0x004d19c0
Vec3 g_vMarkerGoalNorth;
// GLOBAL: HYPERX 0x004d19d0
Vec3 g_vMarkerRimEast;

float               g_flOneAgendaItem      = 1.0f;
float               g_flHalfAgendaItem     = 0.5f;
const float         g_flFrameRateReference = 80.0f;
const double        g_dPeriodScale         = -0.00999;
int                 g_anDebugLineLength[10];
char                g_aDebugLineText[10][150];
short               g_nDebugLineNext;
const double        g_dSurfaceTolerance = 0.01f;
PooledOwnerRoot*    g_pCollInteractorPool;
class AnimRegistry* g_pAnimRegistry;
extern const float  g_flZeroProjection  = 0.0f;
const float         g_flTransformZero   = 0.0f;
const float         g_flTransformOne    = 1.0f;
float               g_flOnePhysicsReset = 1.0f;
char                g_szSfxBarName[20];
char                g_szMusicBarName[20];
const double        g_dHomingSix     = 6.0;
const float         g_flOneThird     = 0.33f;
const float         g_flCollisionOne = 1.0f;
int                 g_nOptionsSavedCdVolume;
int                 g_nOptionsSavedSfxVolume;
int                 g_nOptionDifficulty;
int                 g_nOptionBrutality;
int                 g_nOptionMusicVolume;
int                 g_nOptionSfxVolume;
int                 g_nOptionPeriodLength;
int                 g_nPeriodLengthMs;
int                 g_nOptionTeammateMode;
int                 g_nOptionAttackMode;
struct MessageNode* g_pMessageHead;
struct MessageNode* g_pMessageTail;
// STRING: HYPERX 0x004bb818
const char*        g_pszCharFormat          = "%c";
extern const float g_flCollisionPartDefault = 0.0f;
extern const float g_flZeroMeshBone         = 0.0f;
int                g_nBitmapCacheCount;
BitmapCacheEntry*  g_pBitmapCache;
// STRING: HYPERX 0x004bb810
const char*        g_pszAnimNameFormat = "%s";
extern const float g_flZeroAnimClipSet = 0.0f;
PooledOwnerRoot*   g_pMsgInteractorPool;
extern const float g_flZeroCollisionObj = 0.0f;
int                g_nMatchSetupFlagA;
int                g_nMatchSetupFlagB;
Vec3*              g_apTrackOffsets[5] = {
    &g_vBirdsEyeTrackOffset,
    &g_vFirstPersonTrackOffset,
    &g_vOverShoulderTrackOffset,
    &g_vOverShoulderRokTrackOffset,
    &g_vFallTrackOffset
};
// The first two are zero vectors; the rest are the ellipsoid markers.
Vec3* g_apPositionMarkers[11] = {
    &g_vNoOffset,
    &g_vZeroMarker,
    &g_vMarkerGoalNorth,
    &g_vMarkerGoalSouth,
    &g_vMarkerSurfaceYPlus,
    &g_vMarkerSurfaceYMinus,
    &g_vMarkerSurfaceCentre,
    &g_vMarkerRimEast,
    &g_vMarkerGoalMouthNorth,
    &g_vMarkerGoalMouthSouth,
    nullptr
};
PlanStatFn g_apPlanStatFns[PRN_COUNT];
int        g_nPaletteAnimated;
#include "graphics/transparentqueue.h"
TransparentEntry  g_transparentSiftEntry;
TransparentEntry* g_pTransparentQueue;
short             g_nTransparentCapacity;
short             g_nTransparentCount;
#include "net/gamesession.h"
#include "common/pooledownerroot.h"
#include "anim/animregistry.h"
#include "engine/playerobject.h"
#include "math/point.h"
#include "engine/object.h"
#include "input/cheat.h"
#include "planvm/plannode.h"
char g_bLoadingMode   = 1;
int  g_nJoystickSlotA = 0;
int  g_nJoystickSlotB = 1;

int                g_nBackBufferPixelCount;
unsigned char      g_bBottomUpDIB;
extern const float g_flAppInitZero = 0.0f;
int                g_bSingleStepActive;

struct TransformedVertex* g_pModelVertexBuf;
struct FaceRecord**       g_ppFaceList;
struct FaceRecord*        g_pFaceRecords;

int g_bTwoPlayers;
int g_nPauseRequestStreak;
// One answer per coach condition, written once a frame.
int                 g_anCoachConditions[13];
extern const double g_dDriftLimit = 25.0;

// The one character that decides a plan token is a float rather than an integer.
extern const char* const g_pszDecimalPoint = ".";

// 800, the squared-distance ceiling the attack opportunity condition clamps both thresholds to.
extern const float g_flRangeLimit;
const float        g_flRangeLimit = 800.0f;

extern const float g_flTwoPlan;
const float        g_flTwoPlan = 2.0f;

extern const float g_flSplineReflectScale;
const float        g_flSplineReflectScale = 2.0f;

// The top of the box PlanAction::ReflectOffWall tests the contact point against.
extern const float g_flWallBoxTopY;
const float        g_flWallBoxTopY = 5.0f;

extern const char* const g_pszAnimExtension;
const char* const        g_pszAnimExtension = "wba";

extern const char* const g_apFileTypeNames[9];
const char* const        g_apFileTypeNames[9] = {
    "InvalidType",
    "DB_directory",
    "ModelFileType",
    "ComponentFileType",
    "AnimationFileType",
    "Developmental",
    "Alpha",
    "Beta",
    "Release",
};

extern const char* const g_pszPaletteExtension;
const char* const        g_pszPaletteExtension = "wbp";

// The three statics PhysicsBody::UpdateMovementAxes keeps between frames.
unsigned char       g_bMoveAxesStopHeld;
unsigned char       g_bMoveAxesShootHeld;
Clock::Milliseconds g_moveAxesShootReleaseTick;

// The two tick stamps PlanCond::AnalyzePadA keeps between calls.
Clock::Milliseconds g_rokCameraArmedTick;
Clock::Milliseconds g_rokCameraStateTick;

extern const float g_flAnimBlendZero;
const float        g_flAnimBlendZero = 0.0f;

// Contiguous in the image: the key-config constructor bounds its loops on the object that follows.
Vec3         g_vPlaneSegmentDir                      = { 0 };
char         g_szEmptyString[1]                      = { 0 };
unsigned int g_anKeyConfigAccepted[kKeyBindingCount] = { 0 };
unsigned int g_anKeyConfigKeys[kKeyBindingCount]     = { 0 };

// The animation peek sheet's "Enable Smart Cam" box.
unsigned char g_bSmartCamEnabled = 0;

// The period light's fill and the damage bar's lit and dim palette entries.
int g_nDamageBarColourDim;
int g_nDamageBarColourLit;
int g_nPeriodLightColour;

extern const char* const g_pszEmptyDefaultName = "";

extern const float g_flRatingStart = 0.0f;

extern const float g_flOneAnimPlayer = 1.0f;

extern const float g_flQuatOne = 1.0f;
extern const float g_flQuatTwo = 2.0f;

extern const float g_flCamFracFull = 1.0f;

extern const float g_flGravityHalf = 0.5f;

extern const float g_flChaseSpeedBias     = 3.0f;
extern const float g_flRearViewLeadFrames = 3.0f;
extern const float g_flOneThrow           = 1.0f;
extern const float g_flPiThrow            = 3.1415927410125732f;
extern const float g_flThrowStepsCap      = 3.5f;
extern const float g_flHitSpin            = 3.1415927410125732f;
extern const float g_flSteerBoxSizeX      = 8.0f;
extern const float g_flSteerBoxSizeZ      = 2.0f;
extern const float g_flWallBoxSizeX       = 8.0f;
extern const float g_flWallBoxSizeZ       = 2.0f;
extern const float g_flDegreesPerTurn     = 360.0f;
extern const float g_flMovementAxisDecay  = 0.15f;
extern const float g_flZeroBone           = 0.0f;
extern const float g_flPiCamera           = 3.1415927410125732f;
extern const float g_flDegToRadCamera     = 0.0174532923847436905f;
extern const float g_flCameraStartLookZ   = -100.0f;
extern const float g_flTwoEllipsoid       = 2.0f;
extern const float g_flThreeEllipsoid     = 3.0f;
extern const float g_flPiPlan             = 3.1415927410125732f;
extern const float g_flDegToRadPlayer     = 0.0174532923847436905f;
extern const float g_flZeroPlanState      = 0.0f;
extern const float g_flZeroSpline         = 0.0f;

Vec3 g_vBankRight;

short g_nBankDirRight;

int g_nOptionsSavedDifficulty;
int g_nOptionsSavedBrutality;
int g_nOptionsSavedPeriodLengthMs;
int g_nOptionsSavedTeammateMode;
int g_nOptionsSavedAttackMode;

// The face all eighteen info bar fonts are created from.
char g_abInfoBarFontName[0x50] = { 0 };

TransparentEntry g_renderPushEntry;

char               g_szDebugLightLine[0x64];
extern const float g_flDebugLightDistance = 0.59f;

ClipStagingBuffer g_clipPolyStaging;

unsigned int   g_nSWFaceColour;
FacePolyBuffer g_swFaceBuffer;

// The scores start at minus one, which no score can be, so the first pass builds both labels.
int g_nInfoBarLastSeconds;

extern const float g_flOriginComponent;
extern const float g_flFacingStepBase;
extern const float g_flSteerBoxTopY;

const char* g_pszBitmapExtension = "bmp";

int g_bSoftTexPaletteBuilt;

const float g_flOriginComponent = 0.0f;

const float g_flFacingStepBase = 3.0f;

short g_nCheckImpulseStep;
short g_nCheckImpulseFacing;

short g_nCheckImpulseOtherSlot;
short g_nCheckImpulseSelfSlot;

const float g_flSteerBoxTopY = 5.0f;

extern const float g_flGoalFrameWidth;
const float        g_flGoalFrameWidth = 1.0f;
extern const float g_flGoalSpanWidth;
const float        g_flGoalSpanWidth = 9.0f;

Vec3               g_vLightDirModel;
extern const float g_flVertexW = 1.0f;

PlanOpcodeSlot g_pfnPlanOpcodeHandlers[141];
PlanHandlerFn  g_pfnPlanActionHandlers[140];

int  g_nAnimListNamesLead;
char g_aAnimListNames[35][32];

short g_nTrickSeqStep;
short g_nBoostGrabStep;

extern const float g_flPassAngleStep;
const float        g_flPassAngleStep = 3.0f;

float g_flRokBoneShiftX;
float g_flRokBoneShiftY;
float g_flRokBoneShiftZ;

extern const float g_flVertexWDefault;
const float        g_flVertexWDefault = 1.0f;
Vec3               g_vDispatchFaceNormal;

// Scaling a component header's angles by this is what settles them as degrees on disc.
extern const float g_flDegreesToRadians;
const float        g_flDegreesToRadians = 0.0174532925f;

extern const float g_flPi;
const float        g_flPi = 3.14159274f;

// Two, as float bits read with an integer move.
extern float g_flTwoAgendaItem;
float        g_flTwoAgendaItem = 2.0f;

extern const float g_flOnePlan;
const float        g_flOnePlan = 1.0f;

// The number of periods in a match, compared as a word.
extern const short g_nFinalPeriod;
const short        g_nFinalPeriod = 3;

// Four damage rows of seven approach sectors, and a table by sector alone. The loader fills both.
extern short g_awReactionByDamageAndSector[35];
short        g_awReactionByDamageAndSector[35];

extern const float g_flCamTrackVelocityGain;
const float        g_flCamTrackVelocityGain = 2.0f;

// The debug dialog's IDC_WATCH_ROK checkbox. Clear, the tracking camera takes no new look direction.
extern char g_bCamTrackLookEnabled;
char        g_bCamTrackLookEnabled = 1;

extern short g_nWrongSideFrames;
short        g_nWrongSideFrames;

// Every one ships at minus one, so the first frame after a load rebuilds the row.

// Set during the arena surface pass; the palette then raises the ramp base to the light level.
extern int g_bDromePass;
int        g_bDromePass;

extern const float g_flTwoCamera;
const float        g_flTwoCamera = 2.0f;

extern const float g_flHalfCamera;
const float        g_flHalfCamera = 0.5f;

extern const char* const g_pszAnimShortFormat;
const char* const        g_pszAnimShortFormat = "%hd";

extern const char* const g_pszAnimFloatFormat;
const char* const        g_pszAnimFloatFormat = "%f";

extern const char* const g_pszAnimTokenDelimiters;
const char* const        g_pszAnimTokenDelimiters = "	";

const char* g_pszKeyConfigDefaultName = "";

// GLOBAL: HYPERX 0x004b36f4
extern const float g_flThreePlan;
const float        g_flThreePlan = 3.0f;

// GLOBAL: HYPERX 0x004b36f8
extern const float g_flFourPlan;
const float        g_flFourPlan = 4.0f;

// GLOBAL: HYPERX 0x004b17b8
extern const float g_flFourPlayer;
const float        g_flFourPlayer = 4.0f;

// A second copy of the up axis.
// GLOBAL: HYPERX 0x004b1460
extern const Vec3 g_vRokAxis;
const Vec3        g_vRokAxis = { 0.0f, 0.0f, 1.0f };

int g_nInfoBarLastRokValue;

Vec3 g_vCamPasserAim;

// Float nought, g_flZeroBone, sits four bytes below.
// GLOBAL: HYPERX 0x004b2230
extern const float g_flOneBone;
const float        g_flOneBone = 1.0f;

// GLOBAL: HYPERX 0x004b1438
extern const float g_flViewFracHalf;
const float        g_flViewFracHalf = 0.5f;

// GLOBAL: HYPERX 0x004cf954
extern int g_aaColorSaved[2][20];
int        g_aaColorSaved[2][20];

// GLOBAL: HYPERX 0x004d1a54
extern PlanWord g_planParsedLocalReal;
PlanWord        g_planParsedLocalReal;

// Only the low byte is kept.
// GLOBAL: HYPERX 0x004cb784
extern int g_nPaletteAnimateResult;
int        g_nPaletteAnimateResult;

// The four bytes above each vector are never touched.
Vec3 g_vBubbleSeparation;
Vec3 g_vBubbleOtherCentre;
Vec3 g_vBubbleOwnCentre;

extern const char* const g_pszPercentF = "%f";

// The ratings reader scans a whole number and stores it as a float.
const char* g_pszPercentD = "%d";

extern const char* const g_pszPercentHd = "%hd";

// Milliseconds before the averaging weight is applied, seconds after.
// GLOBAL: HYPERX 0x004d2178
extern float g_flFrameDelta;
float        g_flFrameDelta;

// Whether the period's last ten seconds are being counted down.
// GLOBAL: HYPERX 0x004d2188
extern unsigned char g_bPeriodCountdownActive;
unsigned char        g_bPeriodCountdownActive;

// Open from kMatchStatPossStart to kMatchStatPossStop.
// GLOBAL: HYPERX 0x004d2754
extern char g_bStatIntervalOpen;
char        g_bStatIntervalOpen;

using CoachConditionFn = int (*)();

short g_wSoundSequence;

extern const float g_flSplineTangentStep = 0.1f;

// Stored into a body's flGravAccel; what the thirty-two measures is not decoded.
extern const float g_flGravity = 32.0f;

extern const float g_flMinVelDirSpeed = 0.001f;

extern const float g_flOneCamera = 1.0f;

extern const float g_flFourTracking = 4.0f;

extern const float g_flThreeThrow = 3.0f;

// The bias that makes Math::RandInt's range inclusive at both ends.
extern const float g_flRandRangeBias = 0.4999f;

// One resolver per pronoun, immediately after g_apPlanStatFns.
PlanArgFn g_apPlanArgFns[PRN_COUNT];

BehParamHandlerFn g_apfnBehaviorParamHandlers[17];

short g_wCollInteractorPoolSize = 100;

short g_wMsgInteractorPoolSize = 350;

extern const Vec3 g_vecZeroVelocity = { 0.0f, 0.0f, 0.0f };
extern const Vec3 g_vecZeroPosition = { 0.0f, 0.0f, 0.0f };

float g_flArenaEllipsoidFriction   = 0.01f;
float g_flArenaEllipsoidElasticity = 0.95f;

EllipsoidRatios g_ellipsoidRatios;

int  g_nInstallLevel;
char g_bChampFromJumbotron;

float g_flOneSteering = 1.0f;
int   g_nMainMenuPage;
int   g_nMainMenuTopChoice;
int   g_nMainMenuMatchChoice;
