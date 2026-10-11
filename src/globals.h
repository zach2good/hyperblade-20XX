// Globals shared across translation units. Names follow the Ghidra reading of each address, corrected where wrong.

#include <stdarg.h>

#ifndef HYPERBLADE_GLOBALS_H
#define HYPERBLADE_GLOBALS_H
#include "common/compat.h"

struct FrameTimer;
struct AudioMgr;
struct GameApp;
struct IDirectPlay;

#include "math/vector.h"

#include "common/pathstring.h"
#include "anim/animregistry.h"
#include "ui/radar.h"
#include "graphics/texture.h"
#include "graphics/surfacepoint.h"
#include "planvm/conditiontable.h"
#include "ui/hudsurfaces.h"

class JoystickMgr;

class InfoBarBase;
class CDDBitMap;
class GameOptions;
class CameraObject;
class ColorPalette;
struct StringHeader;
class MessageQueue;

// GLOBAL: HYPERX 0x004ce6c0
extern SurfacePoint g_ptRadarInset;

// GLOBAL: HYPERX 0x004cd4e4
extern String g_strHudDefaultName;
// VC6 numbers globals.cpp's $E functions in definition order, so a dynamic initialiser added there renumbers these.
// SYNTHETIC: HYPERX 0x00439570
// $E8
// SYNTHETIC: HYPERX 0x00439580
// $E7
// g_displayMode's initialiser thunk.
// SYNTHETIC: HYPERX 0x00439520
// $E12
// SYNTHETIC: HYPERX 0x00439530
// $E11

#define STATIONARY_SLOTS 0x30

// Each entry's low word indexes the engine's object array.
// GLOBAL: HYPERX 0x004ce864
extern int g_aStationarySlots[STATIONARY_SLOTS];

// GLOBAL: HYPERX 0x004ce934
extern int g_nStationaryCount;

// A file-scope vector, as in the original, so the plane intersection is not re-entrant.
// GLOBAL: HYPERX 0x004cb7a0
extern Vec3 g_vPlaneSegmentDir;

// GLOBAL: HYPERX 0x004d2b7c
extern unsigned char g_bNetworkGame;

// The first skips the network asset check; the second is the modem profile.
// GLOBAL: HYPERX 0x004d219c
extern int g_bSkipAssetValidation;

// GLOBAL: HYPERX 0x004d21a0
extern int g_bModemPlay;

// GLOBAL: HYPERX 0x004cf054
extern class CD3DTextureManager* g_pTexManager;

// GLOBAL: HYPERX 0x004c1e88
extern int g_nDromeTextured;

// GLOBAL: HYPERX 0x004c1e8c
extern const char* g_pszDromeSection;

// GLOBAL: HYPERX 0x004c1e90
extern const char* g_pszDromeTexturedKey;

// GLOBAL: HYPERX 0x004d21a8
extern int g_nSavedCursorVisible;

class Mixer;
class DSoundMgr;

// Set by -m: the drome loads its shaded models, so the F8 texture toggle is off.
// GLOBAL: HYPERX 0x004cb4f0
extern int g_bShadedDromeModels;

// The first splash frame's palette, which the splash's end deletes.
// GLOBAL: HYPERX 0x004cb4f8
extern HPALETTE g_hSplashOwnedPalette;

// GLOBAL: HYPERX 0x004cb4fc
extern MMRESULT g_nSplashTimerId;

// GLOBAL: HYPERX 0x004cb500
extern int g_nSplashFrame;

// GLOBAL: HYPERX 0x004cb504
extern int g_bSplashBusy;

// GLOBAL: HYPERX 0x004cb5e8
extern class JoystickMgr* g_pJoystickMgr;

// GLOBAL: HYPERX 0x004cb6f0
extern ColorPalette* g_pColorPalette;

// GLOBAL: HYPERX 0x004b40f8
extern const char* const g_pszModelExtension;

// GLOBAL: HYPERX 0x004b4118
extern const char* const g_pszWaveExtension;

// The version a model file must carry, 1.7.
// GLOBAL: HYPERX 0x004b2d40
extern struct FileVersion g_modelVersion;

// GLOBAL: HYPERX 0x004ceed0
extern GameApp g_gameApp;
// SYNTHETIC: HYPERX 0x0044c050
// $E3
// The atexit thunk for g_gameApp's destructor.
// SYNTHETIC: HYPERX 0x0044c060
// $E2

// GLOBAL: HYPERX 0x004ceeb0
extern HPALETTE g_hDisplayPalette;

// GLOBAL: HYPERX 0x004ceebc
extern HWND g_hWndGame;

// GLOBAL: HYPERX 0x004d094c
extern unsigned char g_cMdmTexLoaded;

// The pitch and the surface pointer are each stored to two places, for no known reason.
// GLOBAL: HYPERX 0x004d095c
extern int g_nSurfacePitchCopy;

// GLOBAL: HYPERX 0x004d0a0c
extern int g_nViewportOriginY;

// GLOBAL: HYPERX 0x004d0a10
extern int g_nViewportOriginX;

// GLOBAL: HYPERX 0x004d0a24
extern unsigned char* g_pBackBufferBitsCopy;

// GLOBAL: HYPERX 0x004d0a14
extern int g_nBackBufferHeight;

// GLOBAL: HYPERX 0x004d0a18
extern int g_nBackBufferWidth;

// GLOBAL: HYPERX 0x004ceea4
extern HDC g_hSoftBackDC;

// The HUD art toggle the debug menu flips; the second byte holds one and nothing reads it.
struct RenderFlags
{
    unsigned char bDisplayHUDArt;
    unsigned char bUnread;
    // Never touched alone; the readers load all four bytes and mask the first.
    unsigned short wUnused;
};

// GLOBAL: HYPERX 0x004c2de4
extern RenderFlags g_renderFlags;

// GLOBAL: HYPERX 0x004c2ca0
extern int g_bTripleBufferEnabled;

// GLOBAL: HYPERX 0x004c2c9c
extern int g_bFullScreen;

// Set on a display change and cleared once the software renderer resets its blit rectangle.
// GLOBAL: HYPERX 0x004ceec8
extern char g_bSWClipRectDirty;

// Set to whether the cursor is hidden; Renderer::PresentFrame flips only when it is set.
// GLOBAL: HYPERX 0x004c2c98
extern int g_bCursorHidden;

// GLOBAL: HYPERX 0x004cee84
extern int g_bUseOffscreenSurface;

// GLOBAL: HYPERX 0x004ce610
extern struct IDirectDraw* g_pDDrawDevice;

// GLOBAL: HYPERX 0x004ce938
extern FrameTimer g_frameTimer;
// The atexit registration of g_frameTimer's destructor in frametimer.cpp.
// SYNTHETIC: HYPERX 0x00442920
// $E3
// g_frameTimer's destructor thunk, empty since the destructor is.
// SYNTHETIC: HYPERX 0x00442930
// $E2

// GLOBAL: HYPERX 0x004ce630
extern int g_bHWFlipAvailable;

// GLOBAL: HYPERX 0x004ced64
extern char g_szFpsText[100];

// GLOBAL: HYPERX 0x004cf068
extern GameOptions* g_pGameOptions;

// GLOBAL: HYPERX 0x004d21d0
extern HPALETTE g_hSplashPalette;

// GLOBAL: HYPERX 0x004cf060
extern AudioMgr* g_pAudioMgr;

// GLOBAL: HYPERX 0x004cee70
extern int g_cUseSoftRenderer;

// GLOBAL: HYPERX 0x004cee74
extern int g_bForceSoftwareRender;

// Set by -d: DirectDraw takes the emulation caps and Direct3D skips every hardware device.
// GLOBAL: HYPERX 0x004cee78
extern int g_bEmulationOnly;

// GLOBAL: HYPERX 0x004cee7c
extern int g_cD3DActive;

// GLOBAL: HYPERX 0x004cef49
extern unsigned char g_bIsWindowCreated;

// GLOBAL: HYPERX 0x004cefd0
extern InfoBarBase* g_pRenderer;

// GLOBAL: HYPERX 0x004cf058
extern struct GameEngine* g_pGameEngine;

// GLOBAL: HYPERX 0x004d2764
extern int g_nPrevEngineState;

// GLOBAL: HYPERX 0x004d2778
extern PALETTEENTRY g_aSoftTexPalette[256];

// GLOBAL: HYPERX 0x004cb5dc
extern CameraObject* g_pScene;

// Written by forty functions and read by five.
// GLOBAL: HYPERX 0x004cd4e0
extern int g_hrD3DResult;

// GLOBAL: HYPERX 0x004c8cd4
extern int g_bPerspectiveTexturing;

// GLOBAL: HYPERX 0x004ced5c
extern HMODULE g_hMdmTexDll;

// GLOBAL: HYPERX 0x004d2bb4
extern int g_bBilinearFilter;

// GLOBAL: HYPERX 0x004b2bc8
extern const double g_dNormalizeEpsilon;

// Below three the dialog shows a status line; above it, a scrolling list.
// GLOBAL: HYPERX 0x004d0774
extern int g_nNetSetupState;

struct ClipExtents;
extern ClipExtents g_projRenderParams;

// GLOBAL: HYPERX 0x004cefcc
extern HDC g_hTextDC;

// GLOBAL: HYPERX 0x004ceea8
extern HGDIOBJ g_hSoftBackOldBitmap;

// GLOBAL: HYPERX 0x004ceeac
extern HBITMAP g_hSoftBackBitmap;

// GLOBAL: HYPERX 0x004cb788
extern HANDLE g_hNetworkThread;

// GLOBAL: HYPERX 0x004b3e14
extern const float g_flOne;

// GLOBAL: HYPERX 0x004b3e18
extern const float g_flDegToRad;

// GLOBAL: HYPERX 0x004b3e1c
extern const float g_flHalfPi;

// GLOBAL: HYPERX 0x004b36e4
extern char* const g_pszEmptyName;

// GLOBAL: HYPERX 0x004c6774
extern LightDir g_lightDirs[3];

// GLOBAL: HYPERX 0x004d01b8
extern String g_strPrefs;

struct KeyRow
{
    unsigned char  cFlags;
    unsigned char  cUnused;
    unsigned short nKey;
    unsigned short nIndex;
};

// GLOBAL: HYPERX 0x004d020a
extern KeyRow g_aKeyRows[40];

// GLOBAL: HYPERX 0x004cefe4
extern struct MessageNode* g_pMessageHead;

// GLOBAL: HYPERX 0x004cefe8
extern struct MessageNode* g_pMessageTail;

// GLOBAL: HYPERX 0x004cf064
extern MessageQueue* g_pMessageQueue;

// GLOBAL: HYPERX 0x004b3f2c
extern const float g_flGravityGameEngine;

// GLOBAL: HYPERX 0x004ce92c
extern int g_bSessionActive; // gates the network pump; the disconnect clears it

struct GameSession;

// GLOBAL: HYPERX 0x004ce930
extern GameSession* g_pGameSession;

// GLOBAL: HYPERX 0x004bc474
extern int g_nJoystickId1;

// GLOBAL: HYPERX 0x004bc478
extern int g_nJoystickId2;

// Always nought and one; plan actions index GameEngine::m_apTracked with them.
// GLOBAL: HYPERX 0x004bc47c
extern int g_nJoystickSlotA;

// GLOBAL: HYPERX 0x004bc480
extern int g_nJoystickSlotB;

// GLOBAL: HYPERX 0x004cf06c
extern class KeyboardDeviceObj* g_pInputDevice;

// DATA/ENUMIDS.H's object types, which index the id ranges below.
enum GameObjectType
{
    id_GOB_Null,
    id_GOB_Dromer,
    id_GOB_Rok,
    id_GOB_Drome,
    id_GOB_Ramp,
    id_GOB_NGoal,
    id_GOB_SGoal,
    id_GOB_MainCamera,
    id_GOB_HeadCam,
    id_GOB_StartGun,
    id_GOB_Turnstile,
    id_GOB_Multiplier,
    id_GOB_LaserHurdle,
    id_GOB_GreenRandomizer,
    id_GOB_RedRandomizer,
    id_GOB_God,
    id_GOB_SoundMan,
    id_GOB_LimboPole,
    id_GOB_Hurdle,
    id_GOB_FastLane,
    id_GOB_Explosion,
    id_GOB_Armory,
    id_GOB_KeyBoard,
    id_GOB_GamePad,
    id_GOB_Joystick,
    id_GOB_Mouse,
    id_GOB_SlowLane,
    id_GOB_Mine,
    id_GOB_Swarf,
    id_GOB_Coach,
    id_GOB_BodyPart,
    id_GOB_NUM_OF_TYPES
};

// The first and last model id of each of the thirty one type ranges.
// GLOBAL: HYPERX 0x004c70b0
extern unsigned char g_abObjectTypeBaseline[32];

// GLOBAL: HYPERX 0x004c70d0
extern unsigned char g_abObjectTypeLimit[32];

// GLOBAL: HYPERX 0x004d21b8
extern class Screen* g_pActiveScreen;

// GLOBAL: HYPERX 0x004d21bc
extern int g_nPlayerMode;

#define PLAYER_MODE_EXHIBITION 0
#define PLAYER_MODE_TOURNAMENT 1

// Where Escape paused from: post-match, jumbotron, elsewhere, and the last of the three.
// GLOBAL: HYPERX 0x004c8578
extern unsigned int g_nPostMatchEscState;

// GLOBAL: HYPERX 0x004c857c
extern unsigned int g_nDefaultEscState;

// GLOBAL: HYPERX 0x004c8580
extern unsigned int g_nJumbotronEscState;

// IDC_NET_TRACKING in the physics dialog, which is the original's own resource naming it.
// GLOBAL: HYPERX 0x004d01bc
extern float g_flNetTracking;

// IDC_BANDWIDTH in the physics dialog, which is the original's own resource naming it.
// GLOBAL: HYPERX 0x004d01c0
extern float g_flBandwidth;

// IDC_FORCE_RELAXATION in the physics dialog, which is the original's own resource naming it.
// GLOBAL: HYPERX 0x004d01c4
extern float g_flForceRelaxation;

// IDC_MAX_SPEED in the physics dialog, which is the original's own resource naming it.
// GLOBAL: HYPERX 0x004d01c8
extern float g_flMaxSpeed;

// IDC_MAX_ACCELERATION in the physics dialog, which is the original's own resource naming it.
// GLOBAL: HYPERX 0x004d01cc
extern float g_flMaxAcceleration;

// IDC_SPEED_BURST_FACTOR in the physics dialog, which is the original's own resource naming it.
// GLOBAL: HYPERX 0x004d01d0
extern float g_flSpeedBurstFactor;

// IDC_STOP_FORCE_FACTOR in the physics dialog, which is the original's own resource naming it.
// GLOBAL: HYPERX 0x004d01d4
extern float g_flStopForceFactor;

// IDC_FORCE_INCREMENT in the physics dialog, which is the original's own resource naming it.
// GLOBAL: HYPERX 0x004d01d8
extern float g_flForceIncrement;

// Set while Pause single-steps the match, so focus changes do not resume it.
// GLOBAL: HYPERX 0x004d01dc
extern int g_bSingleStepActive;

// z is IDC_VIEW_STEEPNESS in the physics dialog, the original's own resource naming it; nothing reads x or y.
// GLOBAL: HYPERX 0x004d01f0
extern Vec3 g_vViewSteepness;

// The config file scans it with %d, so the scan writes three bytes past it.
// GLOBAL: HYPERX 0x004d01fc
extern unsigned char g_bVictimCam;

// DATA/ENUMIDS.H's ID_OFS camera tracking offsets, which SetOffsetValues writes by index; the config file reads the first four.
// GLOBAL: HYPERX 0x004d19e8
extern Vec3 g_vBirdsEyeTrackOffset;
// GLOBAL: HYPERX 0x004d19f8
extern Vec3 g_vFirstPersonTrackOffset;
// GLOBAL: HYPERX 0x004d1a08
extern Vec3 g_vOverShoulderTrackOffset;
// GLOBAL: HYPERX 0x004d1a18
extern Vec3 g_vOverShoulderRokTrackOffset;
// GLOBAL: HYPERX 0x004d1a28
extern Vec3 g_vFallTrackOffset;
// The first ID_POS marker, not an ID_OFS offset; nothing writes it.
// GLOBAL: HYPERX 0x004d1a38
extern Vec3 g_vNoOffset;

// ID_OFS's five, which id_OFS_NOOFS counts. No plan passes id_OFS_NOOFS, which would read past it.
// GLOBAL: HYPERX 0x004c6a74
extern Vec3* g_apTrackOffsets[5];

// ID_POS's markers; the last entry is null and nothing sets it.
// GLOBAL: HYPERX 0x004c6a88
extern Vec3* g_apPositionMarkers[11];

// id_POS_NGoalCenter and id_POS_SGoalCenter, the markers a plan names.
#define POS_N_GOAL_CENTER 8
#define POS_S_GOAL_CENTER 9

// GLOBAL: HYPERX 0x004b17ac
extern float g_flOneAgendaItem;

// GLOBAL: HYPERX 0x004b17bc
extern float g_flHalfAgendaItem;

// GLOBAL: HYPERX 0x004cf4e8
extern class PooledOwnerRoot* g_pCollInteractorPool;

// GLOBAL: HYPERX 0x004cf05c
extern class AnimRegistry* g_pAnimRegistry;

// GLOBAL: HYPERX 0x004cb800
extern float g_flScreenDotsPerCmProjection;
// The Projection unit's marked thunks in ddraw.cpp.
// SYNTHETIC: HYPERX 0x00430d30
// $E2
// SYNTHETIC: HYPERX 0x00430d80
// $E8
// SYNTHETIC: HYPERX 0x00430d90
// $E7
// SYNTHETIC: HYPERX 0x00430db0
// $E11
// SYNTHETIC: HYPERX 0x00430de0
// $E14

// GLOBAL: HYPERX 0x004cbb8c
extern int g_nRenderStateTexturePerspective;

// GLOBAL: HYPERX 0x004b15ec
extern float g_flOnePhysicsReset;

// GLOBAL: HYPERX 0x004c8588
extern int g_nBitmapCacheReady;

// GLOBAL: HYPERX 0x004c2df4
extern int g_nSetupBitmapsUnloaded;

// GLOBAL: HYPERX 0x004cf4ec
extern class PooledOwnerRoot* g_pMsgInteractorPool;

// GLOBAL: HYPERX 0x004cede8
extern char g_szClockText[32];

// GLOBAL: HYPERX 0x004b2bd0
extern const float g_flZeroCollisionObj;

// GLOBAL: HYPERX 0x004b1b00
extern const double g_dHomingSix;

// Set once the loader starts, so the renderer knows it is drawing a loading screen.
// GLOBAL: HYPERX 0x004c1e84
extern char g_bLoadingMode;

// The state the awards screen hands on to whatever it launches.
// GLOBAL: HYPERX 0x004c85b8
extern int g_nSetupReturnState;

// GLOBAL: HYPERX 0x004b17a8
extern const float g_flZeroAgendaItem;

// GLOBAL: HYPERX 0x004bb430
extern float g_flSolarWindFactor;

// GLOBAL: HYPERX 0x004bb434
extern float g_flSteadyCamBuffer;

// GLOBAL: HYPERX 0x004bb438
extern float g_flCameraLookHeight;

// GLOBAL: HYPERX 0x004bb43c
extern float g_flCameraTrackingHeight;

// GLOBAL: HYPERX 0x004bb440
extern float g_flCameraMinHeight;

// GLOBAL: HYPERX 0x004cb374
extern unsigned char g_bScanningModesEnabled;

// The blip colours are each skater's and the Rok's, by slot.
// GLOBAL: HYPERX 0x004cef58
extern int g_nRadarLineColour;

// GLOBAL: HYPERX 0x004cef68
extern int g_nRadarBlipColourRok;

// GLOBAL: HYPERX 0x004cef6c
extern int g_nRadarBlipColourDromer3;

// GLOBAL: HYPERX 0x004cef70
extern int g_nRadarBlipColourDromer1;

// GLOBAL: HYPERX 0x004cef74
extern int g_nRadarBlipColourDromer2;

// GLOBAL: HYPERX 0x004cef78
extern int g_nRadarBlipColourDromer0;

// GLOBAL: HYPERX 0x004ced44
extern unsigned short* g_pRightStack;

// GLOBAL: HYPERX 0x004ced48
extern unsigned short* g_pLeftStack;

// GLOBAL: HYPERX 0x004d0950
extern int g_nPrevWidth;

// GLOBAL: HYPERX 0x004d0954
extern int g_nPrevHeight;

// The product of the two above, kept so the blitters do not multiply per frame.
// GLOBAL: HYPERX 0x004d0958
extern int g_nBackBufferPixelCount;

// A bottom-up back buffer; nothing sets it, and the soft renderer builds a top-down DIB.
// GLOBAL: HYPERX 0x004d0a2c
extern unsigned char g_bBottomUpDIB;

// The Drag Off menu toggle: no start drag is broadcast and the period never runs out.
// GLOBAL: HYPERX 0x004ce980
extern unsigned char g_bDragOff;

// GLOBAL: HYPERX 0x004d1794
extern short g_wActiveScoreLine;

// GLOBAL: HYPERX 0x004ce928
extern int g_nSessionState;

// GLOBAL: HYPERX 0x004d0784
extern IDirectPlay* g_pDPlayLobby;

// GLOBAL: HYPERX 0x004cee90
extern RECT g_rcBltDest;

// GLOBAL: HYPERX 0x004d064c
extern struct TransformedVertex* g_pModelVertexBuf;

#define ESTATE_INIT                0
#define ESTATE_LOBBY               3
#define ESTATE_MAIN_MENU           4
#define ESTATE_TOURNAMENT_SETUP    5
#define ESTATE_HEAD_TO_HEAD_SETUP  6
#define ESTATE_MATCH_INFO          7
#define ESTATE_LEAGUE_SETUP        8
#define ESTATE_CHAMPIONSHIP_SETUP  9
#define ESTATE_GAME_CLIPS_VIEWER   10
#define ESTATE_AWARDS              11
#define ESTATE_GAME_CONFIG         12
#define ESTATE_SAVE_LOAD           13
#define ESTATE_MATCH_START         14
#define ESTATE_MATCH_LIVE          15
#define ESTATE_DROME_EXTERIOR      16
#define ESTATE_JUMBOTRON           17
#define ESTATE_PAUSE_FULL          18
#define ESTATE_PAUSE_MINIMAL       19
#define ESTATE_PAUSE_NO_SAVE       20
#define ESTATE_QUIT                22
#define ESTATE_MULTI_SETUP_OPTIONS 23
#define ESTATE_KEY_CONFIG          25
#define ESTATE_FINAL_CEREMONY      26
#define ESTATE_GAME_INTRO          27
#define ESTATE_TRAINING_END        28

class CDDBitMap;

// GLOBAL: HYPERX 0x004cbb7c
extern int g_nD3DVertexUVMask;
// GLOBAL: HYPERX 0x004cbb80
extern int g_nD3DVertexPosMask;
// GLOBAL: HYPERX 0x004cefec
extern char g_bChampFromJumbotron;
// Bit i clips float i of the nine-float vertex; UV is added only on the textured path.
// GLOBAL: HYPERX 0x004ced34
extern int g_nSWVertexUVMask;
// GLOBAL: HYPERX 0x004ced38
extern int g_nSWVertexPosMask;
class Mixer;

// GLOBAL: HYPERX 0x004d21e0
extern Mixer g_mixerWaveOut;
// The atexit registration and destructor thunks for g_mixerWaveOut in soundchannel.cpp.
// SYNTHETIC: HYPERX 0x0048fde0
// $E8

// SYNTHETIC: HYPERX 0x0048fdf0
// $E7

// GLOBAL: HYPERX 0x004d2460
extern DSoundMgr g_dsoundMgr;
// SYNTHETIC: HYPERX 0x0048fcf0
// $E3
// SYNTHETIC: HYPERX 0x0048fd00
// $E2

// GLOBAL: HYPERX 0x004d2320
extern Mixer g_mixerCompactDisc;
// SYNTHETIC: HYPERX 0x0048fd90
// $E3
// SYNTHETIC: HYPERX 0x0048fda0
// $E2

// Seeded at start-up; the spawn and marker tables list it after the six offsets.
// GLOBAL: HYPERX 0x004d1a48
extern Vec3 g_vZeroMarker;
// GLOBAL: HYPERX 0x004d21c0
extern int g_nMainMenuPage;
// GLOBAL: HYPERX 0x004d21c4
extern int g_nMainMenuTopChoice;
// GLOBAL: HYPERX 0x004d21c8
extern int g_nMainMenuMatchChoice;
#endif
