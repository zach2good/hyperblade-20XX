#include "ui/originalui.h"
#include "app/frametimer.h"
#include "app/gameapp.h"
#include "app/menuids.h"
#include "app/window.h"
#include "app/wndproc.h"
#include "anim/animdb.h"
#include "anim/animplayer.h"
#include "common/path.h"
#include "common/peresources.h"
#include "engine/engineobject.h"
#include "engine/cameraobject.h"
#include "engine/game.h"
#include "engine/gameengine.h"
#include "engine/gameoptions.h"
#include "engine/messageids.h"
#include "engine/objectclasses.h"
#include "engine/playerobject.h"
#include "engine/statids.h"
#include "globals.h"
#include "graphics/renderer.h"
#include "input/inputglobals.h"
#include "input/keyboarddeviceobj.h"
#include "physics/physicsbody.h"
#include "planvm/gamebehaviorobject.h"
#include "planvm/msginteractor.h"
#include "planvm/planstate.h"
#include "planvm/planvm.h"
#include "sound/audiomgr.h"
#include "ui/dialogtemplates.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <cstdarg>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <format>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include <imgui.h>

extern unsigned char g_bEnergyOn;
extern unsigned char g_bSmartCamEnabled;
extern char          g_bCamTrackLookEnabled;

namespace
{

using DialogTemplates::ControlKind;

// The menu commands menuids.h has no name for. The original did nothing with any of them.
constexpr auto kIDMenuOneOnOne          = 0x68;
constexpr auto kIDMenuTwoOnTwo          = 0x69;
constexpr auto kIDMenuNetworkConnection = 0x8a;
constexpr auto kIDMenuNetworkPlayers    = 0x8c;

// The dialogs, by resource id.
constexpr auto kIDDialogSplash            = 314;
constexpr auto kIDDialogConfigureKeyboard = 317;
constexpr auto kIDDialogConfigurePhysics  = 318;
constexpr auto kIDDialogPlanEditor        = 319;
constexpr auto kIDDialogRatings           = 320;
constexpr auto kIDDialogDebugParameters   = 321;
constexpr auto kIDDialogAnimationPeek     = OriginalUI::kDialogAnimationPeek;

constexpr std::pair<unsigned short, const char*> s_aDialogNames[] = {
    { 122, "Options chooser" },
    { 136, "Practice chooser" },
    { 137, "Audio visual options" },
    { kIDDialogSplash, "About HyperBlade" },
    { 315, "Loading" },
    { kIDDialogConfigureKeyboard, "Configure keyboard" },
    { kIDDialogConfigurePhysics, "Configure physics" },
    { kIDDialogPlanEditor, "Plan editor" },
    { kIDDialogRatings, "Ratings" },
    { kIDDialogDebugParameters, "Debug parameters" },
    { kIDDialogAnimationPeek, "Animation peek" },
    { 463, "Crash test" },
    { 603, "Waiting for network players" },
    { 604, "Multiplayer setup" },
    { 607, "Select player name and position" },
};

// IDOK and IDCANCEL close a dialog whatever else it does.
constexpr auto kIDOK     = 1;
constexpr auto kIDCancel = 2;

// The camera's eye-track modes, which the camera's plan reads from kMsgMoveEyeTrack.
constexpr auto kEyeTrackBirdsEye     = 0;
constexpr auto kEyeTrackOverShoulder = 2;

// The two goalies' object ids, after the four skaters'.
constexpr auto kGoalieA = 4;
constexpr auto kGoalieB = 5;

// ES_MULTILINE and ES_READONLY.
constexpr auto kEditMultiline = 0x0004u;
constexpr auto kEditReadOnly  = 0x0800u;

// The splash's two buttons, and the labels whose text names a version string to show instead.
constexpr auto kIDSplashPlay         = 1031;
constexpr auto kIDSplashSetup        = 1032;
constexpr auto kIDSplashFirstVersion = 1000;
constexpr auto kIDSplashLastVersion  = 1004;

// Configure Physics' edit boxes and the live constants behind them.
const std::pair<unsigned short, float*> s_aPhysicsFields[] = {
    { 0x3f9, &g_flForceIncrement },
    { 0x3fa, &g_flMaxAcceleration },
    { 0x3fb, &g_flMaxSpeed },
    { 0x3fe, &g_flForceRelaxation },
    { 0x3ff, &g_flStopForceFactor },
    { 0x400, &g_flSpeedBurstFactor },
    { 0x40d, &g_flBandwidth },
    { 0x411, &g_flNetTracking },
    { 0x414, &g_vViewSteepness.z },
};

constexpr auto kIDPhysicsEnergyOn  = 0x40e;
constexpr auto kIDPhysicsVictimCam = 0x41e;

constexpr auto kIDRatingsPlayer   = 0x41f;
constexpr auto kIDRatingsValue    = 0x423;
constexpr auto kIDRatingsCategory = 0x424;

// The Ratings dialog's players, in object id order.
const constexpr char* s_apszRatingsPlayers[] = {
    "RAM (Dr 0)",
    "ROM (Dr 1)",
    "Snake Eyes (Dr 2)",
    "Crash Bug (Dr 3)",
    "Tempura (N Goalie)",
    "Ace High (S Goalie)",
};

const constexpr char* s_apszRatingNames[kRatingCount] = {
    "WeightClass",
    "Stamina",
    "Checking",
    "Balance",
    "Speed",
    "ShotPower",
    "ShotAccuracy",
    "PassAccuracy",
    "OffensiveAbility",
    "LongShotEnable",
    "Immunity",
    "LongPassEnable",
    "MultiPointSpread",
    "RokHandling",
    "ArmoryRange",
    "Damage",
    "Gender",
    "Speedburst0a",
    "Speedburst0b",
    "Speedburst1a",
    "Speedburst1b",
    "Speedburst2a",
    "Speedburst2b",
    "Speedburst3a",
    "Speedburst3b",
    "Speedburst4a",
    "Speedburst4b",
    "Bungee0",
    "Bungee1",
    "Bungee2",
    "Pass0",
    "Pass1",
    "Pass2",
    "WinCloseFreeRok",
    "LoseCloseFreeRok",
    "WinFarFreeRok",
    "LoseFarFreeRok",
    "WinOffWingJob",
    "LoseOffWingJob",
    "Multiplier0",
    "Multiplier1",
    "Turnstile0",
    "Turnstile1",
    "Killball0",
    "Killball1",
    "Randomize0",
    "Randomize1",
    "Fighting0",
    "Fighting1",
    "Fighting2",
    "Fighting3",
    "Fighting4",
    "FlyingRepetoire",
    "HalfpipeRepetoire",
    "CarryRokThrowDist",
    "WinCarryRokRegroupDist",
    "LoseCarryRokRegroupDist",
    "WinAggressionOnOthers",
    "LoseAggressionOnOthers",
    "SubOutDamage",
    "SubInDamage",
    "MultiplierRange",
    "MinSubIn",
};

const constexpr char* s_apszGoalieRatingNames[kGoalieRatingCount] = {
    "WeightClass",
    "Strength",
    "ReactionTime",
    "Balance",
    "Speed",
    "PassAccuracy",
    "Intelligence",
    "UnderPressure",
    "SaveRight",
    "SaveLeft",
    "SaveHigh",
    "SaveLow",
    "ShieldBlocking",
    "CatchingAbility",
    "Aggression",
    "Overall",
};

// Animation Peek shows the first six objects' animation sets, current then previous.
constexpr auto kPeekObjects         = 6;
constexpr auto kIDPeekFirstSet      = 0x468;
constexpr auto kIDPeekFirstPrevSet  = 0x46e;
constexpr auto kIDPeekBungeeLength  = 0x4aa;
constexpr auto kIDPeekSmartCam      = 0x4ab;
constexpr auto kIDPeekScanningModes = 0x4ac;
constexpr auto kIDPeekSteadyCam     = 0x4af;
constexpr auto kIDPeekWatchRok      = 0x4b8;

// Its x, y and z boxes for the main camera's position, offset and direction.
constexpr auto kIDPeekPosition  = 0x4b5;
constexpr auto kIDPeekOffset    = 0x4b9;
constexpr auto kIDPeekDirection = 0x4be;

const std::pair<unsigned short, float*> s_aPeekFields[] = {
    { 0x4a9, &g_flCameraLookHeight },
    { 0x4ad, &g_flCameraTrackingHeight },
    { 0x4ae, &g_flCameraMinHeight },
    { 0x4b0, &g_flSolarWindFactor },
};

constexpr auto kIDKeysAction       = 1008;
constexpr auto kIDKeysKey          = 1009;
constexpr auto kIDKeysSaveAs       = 1011;
constexpr auto kIDKeysLoadConfig   = 1012;
constexpr auto kIDKeysControl      = 1014;
constexpr auto kIDKeysShift        = 1015;
constexpr auto kIDKeysJoystick     = 1020;
constexpr auto kIDKeysKeyboard     = 1021;
constexpr auto kIDKeysUseAsDefault = 1050;
constexpr auto kIDKeysCustomFile   = 1052;

// The original's key list, in its order, with the codes it bound them to.
constexpr std::pair<const char*, unsigned int> s_aKeyChoices[] = {
    { "Up Arrow", Key::kUp },
    { "Down Arrow", Key::kDown },
    { "Left Arrow", Key::kLeft },
    { "Right Arrow", Key::kRight },
    { "Enter Key", Key::kReturn },
    { "Esc Key", Key::kEscape },
    { "Tab Key", Key::kTab },
    { "Space Bar", Key::kSpace },
    { "Left Mouse Button", Key::kLButton },
    { "Right Mouse Button", Key::kRButton },
    { "F1", Key::kF1 },
    { "F2", Key::kF2 },
    { "F3", Key::kF3 },
    { "F4", Key::kF4 },
    { "F5", Key::kF5 },
    { "F6", Key::kF6 },
    { "F7", Key::kF7 },
    { "F8", Key::kF8 },
    { "F9", Key::kF9 },
    { "F10", Key::kF10 },
    { "F11", Key::kF11 },
    { "F12", Key::kF12 },
    { "a", 'A' },
    { "b", 'B' },
    { "c", 'C' },
    { "d", 'D' },
    { "e", 'E' },
    { "f", 'F' },
    { "g", 'G' },
    { "h", 'H' },
    { "i", 'I' },
    { "j", 'J' },
    { "m", 'M' },
    { "n", 'N' },
    { "o", 'O' },
    { "p", 'P' },
    { "q", 'Q' },
    { "r", 'R' },
    { "s", 'S' },
    { "t", 'T' },
    { "x", 'X' },
    { "y", 'Y' },
    { "z", 'Z' },
    { "NUMPAD 0", Key::kNumpad0 },
    { "NUMPAD 1", Key::kNumpad1 },
    { "NUMPAD 2", Key::kNumpad2 },
    { "NUMPAD 3", Key::kNumpad3 },
    { "NUMPAD 4", Key::kNumpad4 },
    { "NUMPAD 5", Key::kNumpad5 },
    { "NUMPAD 6", Key::kNumpad6 },
    { "NUMPAD 7", Key::kNumpad7 },
    { "NUMPAD 8", Key::kNumpad8 },
    { "NUMPAD 9", Key::kNumpad9 },
    { "Decimal Point", Key::kDecimal },
    { "Plus", Key::kAdd },
    { "Minus", Key::kSubtract },
    { "*", Key::kMultiply },
    { "/", Key::kDivide },
    { "UnMapped", 0 },
};

constexpr auto kIDPlanModels     = 1055;
constexpr auto kIDPlanPlans      = 1056;
constexpr auto kIDPlanLoad       = 1032;
constexpr auto kIDPlanCompile    = 1057;
constexpr auto kIDPlanCompileAll = 1058;
constexpr auto kIDPlanBehaviours = 1071;
constexpr auto kIDPlanMessages   = 1072;
constexpr auto kIDPlanProceedTo  = 1074;
constexpr auto kIDPlanSend       = 1075;

// Past any plan's interactor count, so a broken chain cannot loop forever.
constexpr auto kPlanInteractorLimit = 256;

struct ControlState
{
    bool                  bChecked   = false;
    bool                  bDisabled  = false;
    int                   nSelection = -1;
    std::array<char, 128> szText     = {};
};

bool                                                      s_bLoaded = false;
std::vector<DialogTemplates::MenuEntry>                   s_menu;
std::vector<DialogTemplates::Dialog>                      s_dialogs;
std::vector<DialogTemplates::ComboItems>                  s_combos;
std::vector<unsigned short>                               s_open;
std::map<std::pair<unsigned short, size_t>, ControlState> s_controls;

// Combo box lists a dialog fills as it runs, by dialog and control id, over any DLGINIT list.
std::map<std::pair<unsigned short, unsigned short>, std::vector<std::string> > s_runtimeCombos;

// The application icon the splash shows, decoded once and uploaded on first draw.
std::optional<DialogTemplates::Image> s_icon;
SDL_Texture*                          s_pIconTexture = nullptr;

// The rating the Ratings dialog's value box shows, which a category change writes back to.
int s_nRatingsPlayer   = 0;
int s_nRatingsCategory = 0;

// Configure Keyboard's bindings until Accept commits them.
auto s_anKeyEdits = kFactoryKeyBindings;

// enumIDs.h's names for the Plan Editor's message list, and the ids its rows stand for.
std::unique_ptr<PlanVM> s_pPlanSymbols;
std::vector<int>        s_anPlanMessages;

// What the original kept in its menu's check marks rather than in the game.
int  s_nCameraCommand = kIDMenuOverShoulder;
int  s_nSizeCommand   = kIDMenuSizeLarge;
bool s_bGoaliesShown  = true;

// The templates come from the HYPERX.EXE beside the port, once. A missing or unreadable file leaves
// the menu empty rather than stopping the game.
void Load()
{
    if (s_bLoaded)
    {
        return;
    }

    s_bLoaded = true;

    const auto* const pszBase = SDL_GetBasePath();

    if (pszBase == nullptr)
    {
        return;
    }

    const auto resources = PEResources::Read(Path::Native((std::string(pszBase) + "HYPERX.EXE").c_str()));

    if (!resources)
    {
        return;
    }

    auto version = std::vector<unsigned char>();
    auto nIcon   = std::optional<unsigned short>();

    for (const auto& resource : *resources)
    {
        if (resource.nType == PEResources::kTypeVersion)
        {
            version = resource.data;
        }

        if (resource.nType == PEResources::kTypeGroupIcon && resource.strName == "APPICON")
        {
            nIcon = DialogTemplates::PickIcon(resource.data);
        }

        if (resource.nType == PEResources::kTypeMenu && resource.strName == "APPMENU")
        {
            if (auto menu = DialogTemplates::ParseMenu(resource.data))
            {
                s_menu = std::move(*menu);
            }
        }
        else if (resource.nType == PEResources::kTypeDialog && resource.nID != 0)
        {
            if (auto dialog = DialogTemplates::ParseDialog(static_cast<unsigned short>(resource.nID), resource.data))
            {
                s_dialogs.push_back(std::move(*dialog));
            }
        }
        else if (resource.nType == PEResources::kTypeDialogInit && resource.nID != 0)
        {
            for (auto& combo : DialogTemplates::ParseDialogInit(static_cast<unsigned short>(resource.nID), resource.data))
            {
                s_combos.push_back(std::move(combo));
            }
        }
    }

    for (const auto& resource : *resources)
    {
        if (resource.nType == PEResources::kTypeIcon && nIcon && resource.nID == *nIcon)
        {
            s_icon = DialogTemplates::DecodeIcon(resource.data);
        }
    }

    // The splash's labels read as their version string, or keep their key when there is none.
    for (auto& dialog : s_dialogs)
    {
        for (auto& control : dialog.controls)
        {
            if (dialog.nID != kIDDialogSplash || control.nID < kIDSplashFirstVersion || control.nID > kIDSplashLastVersion)
            {
                continue;
            }

            if (auto text = DialogTemplates::VersionString(version, control.strText))
            {
                control.strText = std::move(*text);
            }
        }
    }
}

ControlState& ControlByID(unsigned short nDialog, unsigned short nControl)
{
    for (const auto& dialog : s_dialogs)
    {
        for (size_t n = 0; dialog.nID == nDialog && n < dialog.controls.size(); n++)
        {
            if (dialog.controls[n].nID == nControl)
            {
                return s_controls[{ nDialog, n }];
            }
        }
    }

    // A template without the control; the dialog never draws this state.
    return s_controls[{ nDialog, SIZE_MAX }];
}

void ShowText(unsigned short nDialog, unsigned short nControl, const char* pszText)
{
    auto& state = ControlByID(nDialog, nControl);

    std::snprintf(state.szText.data(), state.szText.size(), "%s", pszText);
}

void ShowFloat(unsigned short nDialog, unsigned short nControl, float flValue)
{
    auto& state = ControlByID(nDialog, nControl);

    std::snprintf(state.szText.data(), state.szText.size(), "%f", flValue);
}

// An empty or unreadable box reads as nought, as the original's did.
float ReadFloat(unsigned short nDialog, unsigned short nControl)
{
    return static_cast<float>(std::atof(ControlByID(nDialog, nControl).szText.data()));
}

CameraObject* MainCamera()
{
    if (g_pGameEngine == nullptr)
    {
        return nullptr;
    }

    return g_pGameEngine->GetCamera(kCameraMainSlot);
}

void InitPhysics()
{
    for (const auto& [nControl, pValue] : s_aPhysicsFields)
    {
        ShowFloat(kIDDialogConfigurePhysics, nControl, *pValue);
    }

    ControlByID(kIDDialogConfigurePhysics, kIDPhysicsEnergyOn).bChecked  = g_bEnergyOn != 0;
    ControlByID(kIDDialogConfigurePhysics, kIDPhysicsVictimCam).bChecked = g_bVictimCam != 0;
}

// The original's OK ignored the two check boxes.
void AcceptPhysics()
{
    for (const auto& [nControl, pValue] : s_aPhysicsFields)
    {
        *pValue = ReadFloat(kIDDialogConfigurePhysics, nControl);
    }
}

// A player's rating, or nothing outside a match.
float* Rating(int nPlayer, int nCategory)
{
    if (g_pGameEngine == nullptr || nPlayer < 0 || nCategory < 0)
    {
        return nullptr;
    }

    auto* const pPlayer = g_pGameEngine->GetPlayer(nPlayer);

    if (pPlayer == nullptr)
    {
        return nullptr;
    }

    return &pPlayer->m_stats.m_aStats[RATING_SLOT(nCategory)];
}

void ShowRating(int nPlayer, int nCategory)
{
    ShowText(kIDDialogRatings, kIDRatingsValue, "");

    if (const auto* const pValue = Rating(nPlayer, nCategory))
    {
        ShowFloat(kIDDialogRatings, kIDRatingsValue, *pValue);
    }
}

void CommitRating(int nPlayer, int nCategory)
{
    if (auto* const pValue = Rating(nPlayer, nCategory))
    {
        *pValue = ReadFloat(kIDDialogRatings, kIDRatingsValue);
    }
}

// Skaters and goalies name their ratings differently.
void FillRatingCategories(int nPlayer)
{
    auto& items = s_runtimeCombos[{ kIDDialogRatings, kIDRatingsCategory }];

    if (nPlayer < kGoalieA)
    {
        items.assign(std::begin(s_apszRatingNames), std::end(s_apszRatingNames));
    }
    else
    {
        items.assign(std::begin(s_apszGoalieRatingNames), std::end(s_apszGoalieRatingNames));
    }
}

void InitRatings()
{
    s_runtimeCombos[{ kIDDialogRatings, kIDRatingsPlayer }].assign(std::begin(s_apszRatingsPlayers), std::end(s_apszRatingsPlayers));
    FillRatingCategories(0);

    ControlByID(kIDDialogRatings, kIDRatingsPlayer).nSelection   = 0;
    ControlByID(kIDDialogRatings, kIDRatingsCategory).nSelection = 0;

    s_nRatingsPlayer   = 0;
    s_nRatingsCategory = 0;
    ShowRating(0, 0);
}

// Changing player keeps the value box as it was. Changing category commits it first.
void RatingsSelectionChanged(unsigned short nControl, int nSelection)
{
    if (nControl == kIDRatingsPlayer)
    {
        FillRatingCategories(nSelection);

        // The goalies have fewer ratings, so a skater's later category has no match.
        auto& category = ControlByID(kIDDialogRatings, kIDRatingsCategory);

        if (category.nSelection >= static_cast<int>(s_runtimeCombos[{ kIDDialogRatings, kIDRatingsCategory }].size()))
        {
            category.nSelection = -1;
        }

        return;
    }

    if (nControl == kIDRatingsCategory)
    {
        CommitRating(s_nRatingsPlayer, s_nRatingsCategory);

        s_nRatingsPlayer   = ControlByID(kIDDialogRatings, kIDRatingsPlayer).nSelection;
        s_nRatingsCategory = nSelection;
        ShowRating(s_nRatingsPlayer, s_nRatingsCategory);
    }
}

void AcceptRatings()
{
    CommitRating(ControlByID(kIDDialogRatings, kIDRatingsPlayer).nSelection, ControlByID(kIDDialogRatings, kIDRatingsCategory).nSelection);
}

const char* AnimSetName(const AnimDB* pSet)
{
    if (pSet == nullptr)
    {
        return "";
    }

    return pSet->GetName();
}

// The main camera's three vectors, each shown in three boxes from its first control id.
std::array<std::pair<unsigned short, Vec3*>, 3> PeekVectors(CameraObject& camera)
{
    return { {
        { kIDPeekPosition, camera.GetPositionPtr() },
        { kIDPeekOffset, camera.m_pSettings },
        { kIDPeekDirection, &camera.GetPhysicsBody()->velDir },
    } };
}

// A camera history window of so many frames, cut to a short as the original did, then clamped.
short HistoryWindow(double dFrames)
{
    return static_cast<short>(std::clamp(static_cast<int>(static_cast<short>(dFrames)), 1, kCameraHistoryCapacity));
}

// The camera averages its position over half the steady-cam buffer and its direction over two fifths.
void SetSteadyCam(float flBuffer)
{
    g_flSteadyCamBuffer = flBuffer;

    auto* const pCamera = MainCamera();

    if (pCamera == nullptr)
    {
        return;
    }

    const auto flRate = g_pGameEngine->m_clock.GetInvDelta();

    pCamera->SetPosHistorySize(HistoryWindow(flRate * flBuffer * 0.5));
    pCamera->SetDirHistorySize(HistoryWindow(flRate * flBuffer * 0.4));
}

void InitPeek()
{
    constexpr auto nDialog = kIDDialogAnimationPeek;

    for (const auto& [nControl, pValue] : s_aPeekFields)
    {
        ShowFloat(nDialog, nControl, *pValue);
    }

    ShowFloat(nDialog, kIDPeekSteadyCam, g_flSteadyCamBuffer);

    ControlByID(nDialog, kIDPeekWatchRok).bChecked      = g_bCamTrackLookEnabled != 0;
    ControlByID(nDialog, kIDPeekSmartCam).bChecked      = g_bSmartCamEnabled != 0;
    ControlByID(nDialog, kIDPeekScanningModes).bChecked = g_bScanningModesEnabled != 0;

    if (g_pGameEngine == nullptr)
    {
        return;
    }

    for (int n = 0; n < kPeekObjects; n++)
    {
        const auto* const pObject = g_pGameEngine->GetSlot(n);

        if (pObject == nullptr || pObject->m_pAnimPlayer == nullptr)
        {
            continue;
        }

        ShowText(nDialog, static_cast<unsigned short>(kIDPeekFirstSet + n), AnimSetName(pObject->m_pAnimPlayer->m_pSet));
        ShowText(nDialog, static_cast<unsigned short>(kIDPeekFirstPrevSet + n), AnimSetName(pObject->m_pAnimPlayer->m_pPrevSet));
    }

    if (auto* const pCamera = MainCamera())
    {
        for (const auto& [nFirst, pVector] : PeekVectors(*pCamera))
        {
            ShowFloat(nDialog, nFirst, pVector->x);
            ShowFloat(nDialog, static_cast<unsigned short>(nFirst + 1), pVector->y);
            ShowFloat(nDialog, static_cast<unsigned short>(nFirst + 2), pVector->z);
        }

        ShowFloat(nDialog, kIDPeekBungeeLength, pCamera->GetBungeeLength());
    }
}

// Deviation: the original stored the direction's y over its x, and cleared the physics dialog's
// energy and VictimCam flags through two controls this dialog does not have.
void AcceptPeek()
{
    constexpr auto nDialog = kIDDialogAnimationPeek;

    for (const auto& [nControl, pValue] : s_aPeekFields)
    {
        *pValue = ReadFloat(nDialog, nControl);
    }

    if (const auto flSteadyCam = ReadFloat(nDialog, kIDPeekSteadyCam); flSteadyCam != g_flSteadyCamBuffer)
    {
        SetSteadyCam(flSteadyCam);
    }

    g_bCamTrackLookEnabled  = static_cast<char>(ControlByID(nDialog, kIDPeekWatchRok).bChecked);
    g_bSmartCamEnabled      = static_cast<unsigned char>(ControlByID(nDialog, kIDPeekSmartCam).bChecked);
    g_bScanningModesEnabled = static_cast<unsigned char>(ControlByID(nDialog, kIDPeekScanningModes).bChecked);

    if (auto* const pCamera = MainCamera())
    {
        for (const auto& [nFirst, pVector] : PeekVectors(*pCamera))
        {
            pVector->x = ReadFloat(nDialog, nFirst);
            pVector->y = ReadFloat(nDialog, static_cast<unsigned short>(nFirst + 1));
            pVector->z = ReadFloat(nDialog, static_cast<unsigned short>(nFirst + 2));
        }

        pCamera->m_flBungeeLength = ReadFloat(nDialog, kIDPeekBungeeLength);
    }
}

void Disable(unsigned short nDialog, std::initializer_list<unsigned short> controls)
{
    for (const auto nControl : controls)
    {
        ControlByID(nDialog, nControl).bDisabled = true;
    }
}

void ShowKeyBinding()
{
    const auto nAction = ControlByID(kIDDialogConfigureKeyboard, kIDKeysAction).nSelection;

    if (nAction < 0 || nAction >= kKeyBindingCount)
    {
        return;
    }

    const auto* const pChoice = std::ranges::find(s_aKeyChoices, s_anKeyEdits[static_cast<size_t>(nAction)], &std::pair<const char*, unsigned int>::second);

    ControlByID(kIDDialogConfigureKeyboard, kIDKeysKey).nSelection = [&]
    {
        if (pChoice == std::end(s_aKeyChoices))
        {
            return -1;
        }

        return static_cast<int>(pChoice - std::begin(s_aKeyChoices));
    }();

    ControlByID(kIDDialogConfigureKeyboard, kIDKeysKeyboard).bChecked = s_anKeyEdits == kFactoryKeyBindings;
}

// Deviation: the original edited a table of forty prototype actions that nothing read. This edits
// the ten bindings the game plays with, which take no Ctrl or Shift and leave the pad to SDL.
void InitKeys()
{
    constexpr auto nDialog = kIDDialogConfigureKeyboard;

    const auto* const pnKeys = [&]() -> const unsigned int*
    {
        if (g_pInputDevice != nullptr)
        {
            return g_pInputDevice->GetDefaultKeyBindings();
        }

        return g_anKeyBindingDefaults;
    }();

    std::copy_n(pnKeys, kKeyBindingCount, s_anKeyEdits.begin());

    s_runtimeCombos[{ nDialog, kIDKeysAction }].assign(std::begin(g_apszKeyBindingNames), std::end(g_apszKeyBindingNames));

    auto& keys = s_runtimeCombos[{ nDialog, kIDKeysKey }];

    keys.clear();

    for (const auto& [pszName, nKey] : s_aKeyChoices)
    {
        keys.emplace_back(pszName);
    }

    ControlByID(nDialog, kIDKeysAction).nSelection     = 0;
    ControlByID(nDialog, kIDKeysUseAsDefault).bChecked = true;
    ShowKeyBinding();

    Disable(nDialog, { kIDKeysControl, kIDKeysShift, kIDKeysJoystick, kIDKeysUseAsDefault, kIDKeysCustomFile, kIDKeysSaveAs, kIDKeysLoadConfig });
}

void KeysSelectionChanged(unsigned short nControl, int nSelection)
{
    if (nControl == kIDKeysKey)
    {
        const auto nAction = ControlByID(kIDDialogConfigureKeyboard, kIDKeysAction).nSelection;

        if (nAction >= 0 && nAction < kKeyBindingCount)
        {
            s_anKeyEdits[static_cast<size_t>(nAction)] = s_aKeyChoices[nSelection].second;
        }
    }

    ShowKeyBinding();
}

void KeysClicked(unsigned short nControl)
{
    if (nControl == kIDKeysKeyboard)
    {
        s_anKeyEdits = kFactoryKeyBindings;
        ShowKeyBinding();
    }
}

// The saved bindings are written as the device commits them, so they hold at the next start.
void AcceptKeys()
{
    if (g_pInputDevice != nullptr)
    {
        g_pInputDevice->CommitKeyBindings(s_anKeyEdits.data(), kKeyBindingCount);

        return;
    }

    for (int n = 0; n < kKeyBindingCount; n++)
    {
        g_anKeyBindingDefaults[n] = s_anKeyEdits[static_cast<size_t>(n)];
        g_pGameOptions->WriteInt("Keys", g_apszKeyBindingNames[n], static_cast<int>(s_anKeyEdits[static_cast<size_t>(n)]));
    }
}

// A combo box opens on its first item, or on none when it has none.
int FirstOrNone(const std::vector<std::string>& items)
{
    if (items.empty())
    {
        return -1;
    }

    return 0;
}

std::string NameOf(const char* pszName)
{
    if (pszName == nullptr)
    {
        return {};
    }

    return pszName;
}

EngineObject* PlanObject()
{
    const auto nObject = ControlByID(kIDDialogPlanEditor, kIDPlanModels).nSelection;

    if (g_pGameEngine == nullptr || nObject < 0 || nObject >= g_pGameEngine->m_nObjectCount)
    {
        return nullptr;
    }

    return g_pGameEngine->m_ppObjects[nObject];
}

// FindPlanByName reports a miss in an error box, and a menu-time state may have no name yet.
int RunningPlanIndex(EngineObject& object)
{
    const auto* const pState = object.GetPlanState();

    if (pState == nullptr || pState->GetName() == nullptr)
    {
        return -1;
    }

    for (short n = 0; n < object.m_nPlanCount; n++)
    {
        if (NameOf(object.GetPlanName(n)) == pState->GetName())
        {
            return n;
        }
    }

    return -1;
}

// The selected plan's state when it is the one the object runs; only that one has behaviours, and
// only once a match has instantiated it.
PlanState* RunningPlan()
{
    auto* const pObject = PlanObject();

    if (pObject == nullptr || pObject->GetPlanState() == nullptr || pObject->GetPlanState()->m_pHead == nullptr)
    {
        return nullptr;
    }

    if (RunningPlanIndex(*pObject) != ControlByID(kIDDialogPlanEditor, kIDPlanPlans).nSelection)
    {
        return nullptr;
    }

    return pObject->GetPlanState();
}

// GetBehaviorByIndex walks off a short chain; this stops at its end.
GameBehaviorObject* BehaviourAt(const PlanState& state, int nIndex)
{
    auto* pAt = state.m_pHead;

    for (int n = 0; pAt != nullptr && n < nIndex && n < state.m_nBehaviorCount; n++)
    {
        pAt = pAt->m_pStandby;
    }

    return pAt;
}

// The messages the plan and the selected behaviour answer, each named once.
void FillPlanMessages()
{
    auto& items = s_runtimeCombos[{ kIDDialogPlanEditor, kIDPlanMessages }];

    items.clear();
    s_anPlanMessages.clear();

    auto* const pState     = RunningPlan();
    const auto  nBehaviour = ControlByID(kIDDialogPlanEditor, kIDPlanBehaviours).nSelection;

    auto* const pBehaviour = [&]() -> GameBehaviorObject*
    {
        if (pState == nullptr || nBehaviour < 0 || nBehaviour >= pState->m_nBehaviorCount)
        {
            return nullptr;
        }

        return BehaviourAt(*pState, nBehaviour);
    }();

    if (pBehaviour != nullptr)
    {
        const auto heads = std::array<Interactor*, 2>{ pState->m_pMsgInteractors, pBehaviour->m_pMsgInteractors };

        for (auto* const pHead : heads)
        {
            auto* pAt = pHead;

            for (int n = 0; pAt != nullptr && n < kPlanInteractorLimit; n++, pAt = pAt->pNext)
            {
                const int nMessage = pAt->GetNamedSlot();

                if (std::ranges::find(s_anPlanMessages, nMessage) != s_anPlanMessages.end())
                {
                    continue;
                }

                const auto* const pszName = s_pPlanSymbols->FindSymbolName(static_cast<unsigned int>(nMessage), "id_MSG_");

                s_anPlanMessages.push_back(nMessage);
                items.push_back([&]
                                {
                                    if (pszName != nullptr)
                                    {
                                        return std::string(pszName);
                                    }

                                    return std::format("Message {}", nMessage);
                                }());
            }
        }
    }

    ControlByID(kIDDialogPlanEditor, kIDPlanMessages).nSelection = FirstOrNone(items);
}

void FillPlanBehaviours()
{
    auto& items = s_runtimeCombos[{ kIDDialogPlanEditor, kIDPlanBehaviours }];

    items.clear();

    auto* const pState = RunningPlan();

    if (pState != nullptr)
    {
        for (auto* pAt = pState->m_pHead; pAt != nullptr && std::ssize(items) < pState->m_nBehaviorCount; pAt = pAt->m_pStandby)
        {
            items.push_back(NameOf(pAt->GetName()));
        }
    }

    ControlByID(kIDDialogPlanEditor, kIDPlanBehaviours).nSelection = [&]
    {
        if (pState == nullptr)
        {
            return -1;
        }

        return static_cast<int>(pState->m_nIndex);
    }();

    FillPlanMessages();
}

void FillPlans()
{
    auto& items = s_runtimeCombos[{ kIDDialogPlanEditor, kIDPlanPlans }];

    items.clear();

    auto* const pObject = PlanObject();

    for (short n = 0; pObject != nullptr && n < pObject->m_nPlanCount; n++)
    {
        items.push_back(NameOf(pObject->GetPlanName(n)));
    }

    ControlByID(kIDDialogPlanEditor, kIDPlanPlans).nSelection = [&]
    {
        if (pObject == nullptr)
        {
            return -1;
        }

        return RunningPlanIndex(*pObject);
    }();

    FillPlanBehaviours();
}

// No plan sources ship, so Compile and Compile All have nothing to read.
void InitPlanEditor()
{
    if (s_pPlanSymbols == nullptr)
    {
        s_pPlanSymbols = std::make_unique<PlanVM>();
        s_pPlanSymbols->LoadSymbolTables();
    }

    auto& models = s_runtimeCombos[{ kIDDialogPlanEditor, kIDPlanModels }];

    models.clear();

    for (short n = 0; g_pGameEngine != nullptr && n < g_pGameEngine->m_nObjectCount; n++)
    {
        models.push_back(NameOf(g_pGameEngine->m_ppObjects[n]->m_pszName.get()));
    }

    ControlByID(kIDDialogPlanEditor, kIDPlanModels).nSelection = FirstOrNone(models);
    FillPlans();

    Disable(kIDDialogPlanEditor, { kIDPlanCompile, kIDPlanCompileAll });

    // Plans run only in a match.
    for (const auto nControl : { kIDPlanLoad, kIDPlanProceedTo, kIDPlanSend })
    {
        ControlByID(kIDDialogPlanEditor, static_cast<unsigned short>(nControl)).bDisabled = g_gameApp.m_nEngineState != kEstateMatchLive;
    }
}

void PlanSelectionChanged(unsigned short nControl)
{
    switch (nControl)
    {
        case kIDPlanModels:
            FillPlans();
            break;

        case kIDPlanPlans:
            FillPlanBehaviours();
            break;

        case kIDPlanBehaviours:
            FillPlanMessages();
            break;

        default:
            break;
    }
}

// Deviation: Load Plan restarts the plan already in memory rather than reading it off the disc
// again, and says nothing when it works.
void PlanButton(unsigned short nControl)
{
    auto* const pObject    = PlanObject();
    const auto  nPlan      = ControlByID(kIDDialogPlanEditor, kIDPlanPlans).nSelection;
    const auto  nBehaviour = ControlByID(kIDDialogPlanEditor, kIDPlanBehaviours).nSelection;

    if (pObject == nullptr || nPlan < 0 || g_gameApp.m_nEngineState != kEstateMatchLive)
    {
        return;
    }

    auto* const pState = RunningPlan();

    switch (nControl)
    {
        case kIDPlanLoad:
            pObject->InstantiateAIPlan(static_cast<short>(nPlan));
            pObject->GetPlanState()->ActivateBehavior(0);
            FillPlanBehaviours();
            break;

        case kIDPlanProceedTo:
            if (pState == nullptr)
            {
                // STRING: HYPERX 0x004c69dc
                g_gameApp.Error("This is not the Plan you think it is.  Press the Load Plan button.");
            }
            else if (nBehaviour >= 0)
            {
                pState->ActivateBehavior(static_cast<unsigned int>(nBehaviour));
                FillPlanMessages();
            }

            break;

        case kIDPlanSend:
        {
            const auto nMessage = ControlByID(kIDDialogPlanEditor, kIDPlanMessages).nSelection;

            if (pState == nullptr || pState->m_nIndex != nBehaviour)
            {
                // STRING: HYPERX 0x004c6a20
                g_gameApp.Error("This is not the Behavior you think.  Press the Load Plan and Proceed To buttons.");
            }
            else if (nMessage >= 0)
            {
                pObject->PostSceneEvent(s_anPlanMessages[static_cast<size_t>(nMessage)], 0);
            }

            break;
        }

        default:
            break;
    }
}

// What each dialog's WM_INITDIALOG read from the game.
void InitDialog(unsigned short nDialog)
{
    switch (nDialog)
    {
        case kIDDialogConfigurePhysics:
            InitPhysics();
            break;

        case kIDDialogRatings:
            InitRatings();
            break;

        case kIDDialogAnimationPeek:
            InitPeek();
            break;

        case kIDDialogConfigureKeyboard:
            InitKeys();
            break;

        case kIDDialogPlanEditor:
            InitPlanEditor();
            break;

        default:
            break;
    }
}

// What each dialog's OK wrote back.
void AcceptDialog(unsigned short nDialog)
{
    switch (nDialog)
    {
        case kIDDialogConfigurePhysics:
            AcceptPhysics();
            break;

        case kIDDialogRatings:
            AcceptRatings();
            break;

        case kIDDialogAnimationPeek:
            AcceptPeek();
            break;

        case kIDDialogConfigureKeyboard:
            AcceptKeys();
            break;

        default:
            break;
    }
}

void Open(unsigned short nDialog)
{
    if (std::find(s_open.begin(), s_open.end(), nDialog) == s_open.end())
    {
        InitDialog(nDialog);
        s_open.push_back(nDialog);
    }
}

void SetWindowSize(int nCommand, int cx, int cy)
{
    auto* const pWindow = Window::Native().As<SDL_Window>();

    if (pWindow != nullptr && SDL_SetWindowSize(pWindow, cx, cy))
    {
        s_nSizeCommand = nCommand;
    }
}

// What MainWindow::OnCommand did with each command, against the same game state.
void Run(int nCommand)
{
    switch (nCommand)
    {
        case kIDMenuNewGame:
            if (g_pGameEngine != nullptr)
            {
                g_pGameEngine->RestartMatch();
            }

            break;

        case kIDMenuExit:
        {
            if (g_pAudioMgr != nullptr)
            {
                g_pAudioMgr->StopCD();
            }

            auto* const pWindow = Window::Native().As<SDL_Window>();

            // The close request the window's own close button sends, as the original posted WM_CLOSE.
            auto event = SDL_Event();

            event.type            = SDL_EVENT_WINDOW_CLOSE_REQUESTED;
            event.window.windowID = SDL_GetWindowID(pWindow);

            SDL_PushEvent(&event);
            break;
        }

        case kIDMenuOverShoulder:
        case kIDMenuBirdsEye:
            if (auto* const pCamera = MainCamera())
            {
                const auto nMode = [&]
                {
                    if (nCommand == kIDMenuBirdsEye)
                    {
                        return kEyeTrackBirdsEye;
                    }

                    return kEyeTrackOverShoulder;
                }();

                pCamera->PostSceneEvent(kMsgMoveEyeTrack, nMode);
                s_nCameraCommand = nCommand;
            }

            break;

        case kIDMenuSizeLarge:
            SetWindowSize(nCommand, 640, 480);
            break;

        case kIDMenuSizeMedium:
            SetWindowSize(nCommand, 480, 360);
            break;

        case kIDMenuSizeSmall:
            SetWindowSize(nCommand, 320, 240);
            break;

        case kIDRenderMDMTextures:
        case kIDRenderBilinearFilter:
        case kIDRenderPerspective:
            WndProc::HandleRenderMenuCommand(static_cast<unsigned short>(nCommand));
            break;

        case kIDMenuFrameTimes:
            g_frameTimer.bFrameTimesOn = g_frameTimer.bFrameTimesOn == 0;
            g_frameTimer.Restart();
            break;

        case kIDMenuDisplayHUDArt:
            g_renderFlags.bDisplayHUDArt = g_renderFlags.bDisplayHUDArt == 0;
            break;

        case kIDMenuShowGoalies:
            if (g_pGameEngine != nullptr)
            {
                if (s_bGoaliesShown)
                {
                    g_pGameEngine->RemoveFromSortedList(kGoalieA);
                    g_pGameEngine->RemoveFromSortedList(kGoalieB);
                }
                else
                {
                    g_pGameEngine->InsertObjectSorted(kGoalieA);
                    g_pGameEngine->InsertObjectSorted(kGoalieB);
                }

                s_bGoaliesShown = !s_bGoaliesShown;
            }

            break;

        case kIDMenuDragOff:
            g_bDragOff = g_bDragOff == 0;
            break;

        case kIDMenuSoundOff:
            if (g_pAudioMgr != nullptr)
            {
                g_pAudioMgr->wNoSound = static_cast<short>(g_pAudioMgr->wNoSound == 0);
            }

            break;

        case kIDMenuConfigureKeys:
            Open(kIDDialogConfigureKeyboard);
            break;

        case kIDMenuPhysicsConfig:
            Open(kIDDialogConfigurePhysics);
            break;

        case kIDMenuPlanEditor:
            Open(kIDDialogPlanEditor);
            break;

        case kIDMenuRatings:
            Open(kIDDialogRatings);
            break;

        case kIDMenuDebugDialog:
            Open(kIDDialogDebugParameters);
            break;

        case kIDMenuAbout:
            Open(kIDDialogSplash);
            break;

        case kIDMenuMIDIOff:
        case kIDMenuNetworkConnection:
        case kIDMenuNetworkPlayers:
        case kIDMenuTwoPlayer:
        case kIDMenuOneOnOne:
        case kIDMenuTwoOnTwo:
            // The original fell through to DefWindowProc for these, or returned at once.
            break;

        default:
            break;
    }
}

bool IsChecked(const DialogTemplates::MenuEntry& entry)
{
    switch (entry.nCommand)
    {
        case kIDMenuOverShoulder:
        case kIDMenuBirdsEye:
            return s_nCameraCommand == entry.nCommand;

        case kIDMenuSizeLarge:
        case kIDMenuSizeMedium:
        case kIDMenuSizeSmall:
            return s_nSizeCommand == entry.nCommand;

        case kIDRenderMDMTextures:
            return g_cMdmTexLoaded != 0;

        case kIDRenderBilinearFilter:
            return g_bBilinearFilter != 0;

        case kIDRenderPerspective:
            return g_bPerspectiveTexturing != 0;

        case kIDMenuFrameTimes:
            return g_frameTimer.bFrameTimesOn != 0;

        case kIDMenuDisplayHUDArt:
            return g_renderFlags.bDisplayHUDArt != 0;

        case kIDMenuShowGoalies:
            return s_bGoaliesShown;

        case kIDMenuDragOff:
            return g_bDragOff != 0;

        case kIDMenuSoundOff:
            return g_pAudioMgr != nullptr && g_pAudioMgr->wNoSound != 0;

        default:
            return (entry.nFlags & DialogTemplates::kMenuChecked) != 0;
    }
}

bool IsEnabled(const DialogTemplates::MenuEntry& entry)
{
    switch (entry.nCommand)
    {
        case kIDRenderMDMTextures:
        case kIDRenderBilinearFilter:
        case kIDRenderPerspective:
            // The original greyed these until textures could load; the toggles act only then.
            return Renderer::GetRenderOptions().bTextured != 0;

        case kIDMenuSizeLarge:
        case kIDMenuSizeMedium:
        case kIDMenuSizeSmall:
            return true;

        case kIDMenuNewGame:
        case kIDMenuOverShoulder:
        case kIDMenuBirdsEye:
        case kIDMenuShowGoalies:
            return g_pGameEngine != nullptr;

        default:
            return (entry.nFlags & DialogTemplates::kMenuGrayed) == 0;
    }
}

// The Windows 95 palette: the button face, its two light and two shadow edges, and the selection.
constexpr auto kColourFace       = IM_COL32(192, 192, 192, 255);
constexpr auto kColourLight      = IM_COL32(255, 255, 255, 255);
constexpr auto kColourLightEdge  = IM_COL32(223, 223, 223, 255);
constexpr auto kColourShadow     = IM_COL32(128, 128, 128, 255);
constexpr auto kColourDark       = IM_COL32(0, 0, 0, 255);
constexpr auto kColourNavy       = IM_COL32(0, 0, 128, 255);
constexpr auto kColourCaptionEnd = IM_COL32(16, 132, 208, 255);
constexpr auto kColourScrollBg   = IM_COL32(224, 224, 224, 255);
constexpr auto kColourTooltip    = IM_COL32(255, 255, 225, 255);

// The string Windows measures a dialog font's average character width over.
constexpr auto kAverageSample = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

void FillPixels(ImDrawList* pList, float x0, float y0, float x1, float y1, ImU32 nColour)
{
    pList->AddRectFilled(ImVec2(x0, y0), ImVec2(x1, y1), nColour);
}

// The two-pixel 3D edge. Raised is a button or a window; sunken is a field, or a pressed button.
void DrawBevel(ImDrawList* pList, const ImVec2& min, const ImVec2& max, bool bRaised)
{
    struct Edges
    {
        ImU32 nOuterTopLeft;
        ImU32 nInnerTopLeft;
        ImU32 nInnerBottomRight;
        ImU32 nOuterBottomRight;
    };

    const auto edges = [&]
    {
        if (bRaised)
        {
            return Edges{ kColourLight, kColourLightEdge, kColourShadow, kColourDark };
        }

        return Edges{ kColourShadow, kColourDark, kColourLightEdge, kColourLight };
    }();

    const auto x0 = std::floor(min.x);
    const auto y0 = std::floor(min.y);
    const auto x1 = std::floor(max.x);
    const auto y1 = std::floor(max.y);

    FillPixels(pList, x0, y0, x1, y0 + 1, edges.nOuterTopLeft);
    FillPixels(pList, x0, y0, x0 + 1, y1, edges.nOuterTopLeft);
    FillPixels(pList, x0, y1 - 1, x1, y1, edges.nOuterBottomRight);
    FillPixels(pList, x1 - 1, y0, x1, y1, edges.nOuterBottomRight);
    FillPixels(pList, x0 + 1, y0 + 1, x1 - 1, y0 + 2, edges.nInnerTopLeft);
    FillPixels(pList, x0 + 1, y0 + 1, x0 + 2, y1 - 1, edges.nInnerTopLeft);
    FillPixels(pList, x0 + 1, y1 - 2, x1 - 1, y1 - 1, edges.nInnerBottomRight);
    FillPixels(pList, x1 - 2, y0 + 1, x1 - 1, y1 - 1, edges.nInnerBottomRight);
}

// An etched line or frame, as a group box and SS_ETCHED* draw: shadow, with light just inside.
void DrawEtched(ImDrawList* pList, const ImVec2& min, const ImVec2& max)
{
    const auto x0 = std::floor(min.x);
    const auto y0 = std::floor(min.y);
    const auto x1 = std::floor(max.x);
    const auto y1 = std::floor(max.y);

    if (y1 - y0 <= 2)
    {
        FillPixels(pList, x0, y0, x1, y0 + 1, kColourShadow);
        FillPixels(pList, x0, y0 + 1, x1, y0 + 2, kColourLight);

        return;
    }

    pList->AddRect(ImVec2(x0 + 1, y0 + 1), ImVec2(x1, y1), kColourLight);
    pList->AddRect(ImVec2(x0, y0), ImVec2(x1 - 1, y1 - 1), kColourShadow);
}

// A popup menu is a raised window.
void BevelCurrentWindow()
{
    auto* const pList = ImGui::GetWindowDrawList();
    const auto  pos   = ImGui::GetWindowPos();
    const auto  size  = ImGui::GetWindowSize();

    pList->PushClipRectFullScreen();
    DrawBevel(pList, pos, ImVec2(pos.x + size.x, pos.y + size.y), true);
    pList->PopClipRect();
}

// A highlighted menu item's text is white on the navy bar. ImGui colours text by style rather than
// by state, so the highlight is worked out before the item draws.
bool PushHighlightText(const char* pszLabel, float flWidth, bool bMenu)
{
    const auto pos        = ImGui::GetCursorScreenPos();
    const auto flHeight   = ImGui::GetTextLineHeightWithSpacing();
    const auto bHovered   = ImGui::IsWindowHovered() && ImGui::IsMouseHoveringRect(pos, ImVec2(pos.x + flWidth, pos.y + flHeight));
    const auto bHighlight = bHovered || (bMenu && ImGui::IsPopupOpen(pszLabel));

    if (bHighlight)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, kColourLight);
    }

    return bHighlight;
}

void DrawEntries(const std::vector<DialogTemplates::MenuEntry>& entries, bool bMenuBar)
{
    for (const auto& entry : entries)
    {
        const auto strLabel = DialogTemplates::StripMnemonic(entry.strText);

        if (!entry.children.empty())
        {
            if (OriginalUI::BeginMenu(strLabel.c_str(), bMenuBar))
            {
                DrawEntries(entry.children, false);
                ImGui::EndMenu();
            }

            continue;
        }

        if (entry.nCommand == 0)
        {
            OriginalUI::Separator();
            continue;
        }

        if (OriginalUI::MenuItem(strLabel.c_str(), nullptr, IsChecked(entry), IsEnabled(entry)))
        {
            Run(entry.nCommand);
        }
    }
}

const char* DialogName(unsigned short nDialog)
{
    for (const auto& [nID, pszName] : s_aDialogNames)
    {
        if (nID == nDialog)
        {
            return pszName;
        }
    }

    return "";
}

// The largest font size, down to six tenths of the current one, at which the label wraps into its
// box without breaking a word. The original set some of these in a narrower face than ImGui's.
float FitFontSize(const std::string& strLabel, const ImVec2& size)
{
    const auto flMin  = ImGui::GetFontSize() * 0.6f;
    auto       flSize = ImGui::GetFontSize();

    for (; flSize > flMin; flSize -= 1.0f)
    {
        ImGui::PushFont(nullptr, flSize);

        auto bFits = ImGui::CalcTextSize(strLabel.c_str(), nullptr, false, size.x).y <= size.y;

        for (size_t nStart = 0; bFits && nStart < strLabel.size();)
        {
            const auto nEnd = (std::min)(strLabel.find_first_of(" \n", nStart), strLabel.size());

            bFits  = ImGui::CalcTextSize(strLabel.data() + nStart, strLabel.data() + nEnd).x <= size.x;
            nStart = nEnd + 1;
        }

        ImGui::PopFont();

        if (bFits)
        {
            return flSize;
        }
    }

    return flSize;
}

void DrawLabel(const DialogTemplates::Control& control, const std::string& strLabel, const ImVec2& size)
{
    const auto flAlign = DialogTemplates::LabelAlignment(control);

    if (flAlign != 0.0f)
    {
        const auto flWidth = ImGui::CalcTextSize(strLabel.c_str()).x;

        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (std::max)(0.0f, (size.x - flWidth) * flAlign));
        ImGui::TextUnformatted(strLabel.c_str());

        return;
    }

    // Only a label boxed for several lines wraps. ImGui's font runs wider than MS Sans Serif, so
    // wrapping a one-line label breaks text the original fits; the scaled dialog leaves it room.
    if (size.y < ImGui::GetFontSize() * 2.0f)
    {
        ImGui::TextUnformatted(strLabel.c_str());

        return;
    }

    ImGui::PushFont(nullptr, FitFontSize(strLabel, size));
    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + size.x);
    ImGui::TextUnformatted(strLabel.c_str());
    ImGui::PopTextWrapPos();
    ImGui::PopFont();
}

const std::vector<std::string>* ComboItemsFor(unsigned short nDialog, unsigned short nControl)
{
    if (const auto it = s_runtimeCombos.find({ nDialog, nControl }); it != s_runtimeCombos.end())
    {
        return &it->second;
    }

    for (const auto& combo : s_combos)
    {
        if (combo.nDialog == nDialog && combo.nControl == nControl)
        {
            return &combo.items;
        }
    }

    return nullptr;
}

// Radio buttons select exclusively within a run that a WS_GROUP control starts.
void SelectRadio(const DialogTemplates::Dialog& dialog, size_t nIndex)
{
    auto nFirst = nIndex;

    while (nFirst > 0 && (dialog.controls[nFirst].dwStyle & DialogTemplates::kStyleGroup) == 0 && DialogTemplates::Classify(dialog.controls[nFirst - 1]) == ControlKind::RadioButton)
    {
        nFirst--;
    }

    for (auto n = nFirst; n < dialog.controls.size(); n++)
    {
        const auto& control = dialog.controls[n];

        if (n > nFirst && ((control.dwStyle & DialogTemplates::kStyleGroup) != 0 || DialogTemplates::Classify(control) != ControlKind::RadioButton))
        {
            break;
        }

        s_controls[{ dialog.nID, n }].bChecked = n == nIndex;
    }
}

// The texture lives as long as the renderer, which is the life of the game.
SDL_Texture* IconTexture()
{
    if (s_pIconTexture != nullptr || !s_icon)
    {
        return s_pIconTexture;
    }

    auto* const pRenderer = SDL_GetRenderer(Window::Native().As<SDL_Window>());

    if (pRenderer == nullptr)
    {
        return nullptr;
    }

    s_pIconTexture = SDL_CreateTexture(pRenderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, s_icon->nWidth, s_icon->nHeight);

    if (s_pIconTexture != nullptr)
    {
        SDL_SetTextureBlendMode(s_pIconTexture, SDL_BLENDMODE_BLEND);
        SDL_UpdateTexture(s_pIconTexture, nullptr, s_icon->rgba.data(), s_icon->nWidth * 4);
    }

    return s_pIconTexture;
}

void SelectionChanged(unsigned short nDialog, unsigned short nControl, int nSelection)
{
    switch (nDialog)
    {
        case kIDDialogRatings:
            RatingsSelectionChanged(nControl, nSelection);
            break;

        case kIDDialogConfigureKeyboard:
            KeysSelectionChanged(nControl, nSelection);
            break;

        case kIDDialogPlanEditor:
            PlanSelectionChanged(nControl);
            break;

        default:
            break;
    }
}

void Clicked(unsigned short nDialog, unsigned short nControl)
{
    if (nDialog == kIDDialogConfigureKeyboard)
    {
        KeysClicked(nControl);
    }
}

// True when the button closes its dialog.
bool PressButton(unsigned short nDialog, unsigned short nControl)
{
    if (nControl == kIDOK)
    {
        AcceptDialog(nDialog);

        return true;
    }

    if (nControl == kIDCancel)
    {
        return true;
    }

    if (nDialog == kIDDialogPlanEditor)
    {
        PlanButton(nControl);
    }

    // The splash's Setup leaves the match for the lobby; Play goes back to it.
    if (nDialog == kIDDialogSplash && nControl == kIDSplashSetup)
    {
        Game::SetEngineState(kEstateLobby);
        g_nPrevEngineState = kEstateMatchLive;
    }

    return nDialog == kIDDialogSplash && (nControl == kIDSplashPlay || nControl == kIDSplashSetup);
}

// BS_DEFPUSHBUTTON, which Windows 95 draws with an extra black frame.
bool IsDefaultButton(const DialogTemplates::Control& control)
{
    return (control.dwStyle & 0x0Fu) == 0x01u;
}

void DrawControl(const DialogTemplates::Dialog& dialog, size_t nIndex, const DialogTemplates::Scale& scale, bool& bOpen)
{
    const auto& control  = dialog.controls[nIndex];
    const auto  strLabel = DialogTemplates::StripMnemonic(control.strText);
    const auto  size     = ImVec2(control.cx * scale.x, control.cy * scale.y);
    const auto  topLeft  = ImGui::GetCursorScreenPos();
    auto* const pList    = ImGui::GetWindowDrawList();

    auto& state = s_controls[{ dialog.nID, nIndex }];

    switch (DialogTemplates::Classify(control))
    {
        case ControlKind::PushButton:
        {
            if (OriginalUI::Button(strLabel.c_str(), size.x, size.y, IsDefaultButton(control)) && PressButton(dialog.nID, control.nID))
            {
                bOpen = false;
            }

            break;
        }

        case ControlKind::CheckBox:
            if (OriginalUI::Checkbox(strLabel.c_str(), &state.bChecked))
            {
                Clicked(dialog.nID, control.nID);
            }

            break;

        case ControlKind::RadioButton:
            if (OriginalUI::RadioButton(strLabel.c_str(), state.bChecked))
            {
                SelectRadio(dialog, nIndex);
                Clicked(dialog.nID, control.nID);
            }

            break;

        case ControlKind::GroupBox:
        {
            const auto flHalf = ImGui::GetFontSize() * 0.5f;

            DrawEtched(pList, ImVec2(topLeft.x, topLeft.y + flHalf), ImVec2(topLeft.x + size.x, topLeft.y + size.y));

            // The caption sits on the frame's top edge, over a patch of the face colour.
            const auto textSize = ImGui::CalcTextSize(strLabel.c_str());

            pList->AddRectFilled(ImVec2(topLeft.x + 6, topLeft.y), ImVec2(topLeft.x + 10 + textSize.x, topLeft.y + textSize.y), kColourFace);
            ImGui::SetCursorScreenPos(ImVec2(topLeft.x + 8, topLeft.y));
            ImGui::TextUnformatted(strLabel.c_str());
            break;
        }

        case ControlKind::Label:
            DrawLabel(control, strLabel, size);
            break;

        case ControlKind::Frame:
            DrawEtched(pList, topLeft, ImVec2(topLeft.x + size.x, topLeft.y + size.y));
            break;

        case ControlKind::ScrollBar:
            FillPixels(pList, topLeft.x, topLeft.y, topLeft.x + size.x, topLeft.y + size.y, kColourScrollBg);
            break;

        case ControlKind::Edit:
        {
            const auto flags = [&]
            {
                if ((control.dwStyle & kEditReadOnly) != 0)
                {
                    return ImGuiInputTextFlags_ReadOnly;
                }

                return ImGuiInputTextFlags_None;
            }();

            if ((control.dwStyle & kEditMultiline) != 0)
            {
                ImGui::InputTextMultiline("##edit", state.szText.data(), state.szText.size(), size, flags);
            }
            else
            {
                ImGui::SetNextItemWidth(size.x);
                ImGui::InputText("##edit", state.szText.data(), state.szText.size(), flags);
            }

            DrawBevel(pList, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), false);
            break;
        }

        case ControlKind::ComboBox:
        {
            const auto* const pItems = ComboItemsFor(dialog.nID, control.nID);

            const auto* const pszPreview = [&]() -> const char*
            {
                if (pItems != nullptr && state.nSelection >= 0 && static_cast<size_t>(state.nSelection) < pItems->size())
                {
                    return (*pItems)[static_cast<size_t>(state.nSelection)].c_str();
                }

                return "";
            }();

            ImGui::SetNextItemWidth(size.x);

            const auto bOpenList = ImGui::BeginCombo("##combo", pszPreview);

            DrawBevel(pList, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), false);

            if (bOpenList)
            {
                BevelCurrentWindow();

                for (size_t n = 0; pItems != nullptr && n < pItems->size(); n++)
                {
                    // A drop-down's list follows the pointer, white on navy, rather than marking the current item.
                    const auto bWhite = PushHighlightText((*pItems)[n].c_str(), ImGui::GetContentRegionAvail().x, false);

                    ImGui::PushStyleColor(ImGuiCol_Header, IM_COL32(0, 0, 0, 0));

                    const auto bChosen = ImGui::Selectable((*pItems)[n].c_str(), state.nSelection == static_cast<int>(n));

                    ImGui::PopStyleColor();

                    // A drop-down opens scrolled to its current item.
                    if (state.nSelection == static_cast<int>(n))
                    {
                        ImGui::SetItemDefaultFocus();
                    }

                    if (bWhite)
                    {
                        ImGui::PopStyleColor();
                    }

                    if (bChosen && state.nSelection != static_cast<int>(n))
                    {
                        state.nSelection = static_cast<int>(n);

                        SelectionChanged(dialog.nID, control.nID, state.nSelection);
                    }
                }

                ImGui::EndCombo();
            }

            break;
        }

        case ControlKind::ListBox:
            if (ImGui::BeginListBox("##list", size))
            {
                ImGui::EndListBox();
            }

            DrawBevel(pList, ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), false);
            break;

        case ControlKind::Picture:
            // ponytail: the one picture any dialog names is APPICON; add a lookup when another appears.
            if (auto* const pTexture = IconTexture(); pTexture != nullptr && control.strText == "APPICON")
            {
                ImGui::Image(ImTextureRef(reinterpret_cast<ImTextureID>(pTexture)), size);
            }

            break;

        case ControlKind::Other:
            DrawEtched(pList, topLeft, ImVec2(topLeft.x + size.x, topLeft.y + size.y));
            ImGui::TextDisabled("%s", control.strClass.c_str());
            break;
    }
}

// Draws one dialog at its template's layout. False once it has closed.
bool DrawDialog(const DialogTemplates::Dialog& dialog)
{
    const auto strID    = std::to_string(dialog.nID);
    const auto strTitle = [&]
    {
        if (dialog.strCaption.empty())
        {
            return std::string(DialogName(dialog.nID));
        }

        return dialog.strCaption;
    }();

    ImGui::Begin(("###original" + strID).c_str(), nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse);

    auto bOpen = OriginalUI::DrawWindowFrame(strTitle.c_str());

    // Windows measures the dialog font's average character to size a dialog unit.
    const auto scale  = DialogTemplates::ScaleFor(ImGui::CalcTextSize(kAverageSample).x / 52.0f, ImGui::GetFontSize());
    const auto origin = ImGui::GetCursorPos();

    for (size_t n = 0; n < dialog.controls.size(); n++)
    {
        const auto& control = dialog.controls[n];

        if ((control.dwStyle & DialogTemplates::kStyleVisible) == 0)
        {
            continue;
        }

        ImGui::PushID(static_cast<int>(n));
        ImGui::SetCursorPos(ImVec2(origin.x + control.x * scale.x, origin.y + control.y * scale.y));
        ImGui::BeginDisabled((control.dwStyle & DialogTemplates::kStyleDisabled) != 0 || s_controls[{ dialog.nID, n }].bDisabled);

        DrawControl(dialog, n, scale, bOpen);

        ImGui::EndDisabled();
        ImGui::PopID();
    }

    // The window sizes itself to the template, not to wherever the last control ended.
    ImGui::SetCursorPos(ImVec2(origin.x + dialog.cx * scale.x, origin.y + dialog.cy * scale.y));
    ImGui::Dummy(ImVec2(0.0f, 0.0f));
    ImGui::End();

    return bOpen;
}

struct TabRect
{
    ImVec2 min;
    ImVec2 max;
    bool   bSelected;
};

// ponytail: one property sheet at a time; nest them and this needs a stack.
std::vector<TabRect> s_tabs;

// The space between a property sheet's raised edge and what it holds.
constexpr auto kSheetInset = 6.0f;

// A Windows 95 tab: white up the left and along the top with a cut corner, dark down the right.
// The selected one stands two pixels proud of the rest and opens into the panel below it.
void DrawTab(ImDrawList* pList, const TabRect& tab)
{
    auto x0 = std::floor(tab.min.x);
    auto y0 = std::floor(tab.min.y);
    auto x1 = std::floor(tab.max.x);
    auto y1 = std::floor(tab.max.y);

    if (tab.bSelected)
    {
        x0 -= 2;
        x1 += 2;
        y0 -= 2;
        y1 += 2;

        // Clear the strips it grows into, and the panel's edge under it, but not its label.
        FillPixels(pList, x0, y0, x1, y0 + 2, kColourFace);
        FillPixels(pList, x0, y0, x0 + 2, y1, kColourFace);
        FillPixels(pList, x1 - 2, y0, x1, y1, kColourFace);
        FillPixels(pList, x0, y1 - 2, x1, y1, kColourFace);
    }
    else
    {
        y0 += 2;
    }

    FillPixels(pList, x0, y0 + 2, x0 + 1, y1, kColourLight);
    FillPixels(pList, x0 + 1, y0 + 1, x0 + 2, y0 + 2, kColourLight);
    FillPixels(pList, x0 + 2, y0, x1 - 2, y0 + 1, kColourLight);
    FillPixels(pList, x1 - 2, y0 + 1, x1 - 1, y0 + 2, kColourDark);
    FillPixels(pList, x1 - 1, y0 + 2, x1, y1, kColourDark);
    FillPixels(pList, x1 - 2, y0 + 2, x1 - 1, y1, kColourShadow);
}

// An arc of a ring, half way round from flFrom, in turns of pi.
void DrawArc(ImDrawList* pList, const ImVec2& centre, float flRadius, float flFrom, ImU32 nColour)
{
    constexpr auto kPi = 3.14159265f;

    pList->PathArcTo(centre, flRadius, flFrom * kPi, (flFrom + 1.0f) * kPi, 12);
    pList->PathStroke(nColour, 1.0f);
}

} // namespace

OriginalUI::Theme::Theme()
{
    const auto vars = std::array{
        std::pair{ ImGuiStyleVar_WindowRounding, 0.0f },
        std::pair{ ImGuiStyleVar_ChildRounding, 0.0f },
        std::pair{ ImGuiStyleVar_FrameRounding, 0.0f },
        std::pair{ ImGuiStyleVar_PopupRounding, 0.0f },
        std::pair{ ImGuiStyleVar_ScrollbarRounding, 0.0f },
        std::pair{ ImGuiStyleVar_GrabRounding, 0.0f },
        std::pair{ ImGuiStyleVar_TabRounding, 0.0f },
        std::pair{ ImGuiStyleVar_WindowBorderSize, 0.0f },
        std::pair{ ImGuiStyleVar_FrameBorderSize, 0.0f },
        std::pair{ ImGuiStyleVar_PopupBorderSize, 0.0f },
    };

    for (const auto& [var, flValue] : vars)
    {
        ImGui::PushStyleVar(var, flValue);
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4.0f, 4.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 2.0f));
    m_nVars = static_cast<int>(vars.size()) + 2;

    // A border, where one is drawn, is a sunken edge: shadow around, light below and to the right.
    const auto colours = std::array{
        std::pair{ ImGuiCol_Text, kColourDark },
        std::pair{ ImGuiCol_TextDisabled, kColourShadow },
        std::pair{ ImGuiCol_WindowBg, kColourFace },
        std::pair{ ImGuiCol_PopupBg, kColourFace },
        std::pair{ ImGuiCol_MenuBarBg, kColourFace },
        std::pair{ ImGuiCol_Border, kColourShadow },
        std::pair{ ImGuiCol_BorderShadow, kColourLight },
        std::pair{ ImGuiCol_FrameBg, kColourLight },
        std::pair{ ImGuiCol_FrameBgHovered, kColourLight },
        std::pair{ ImGuiCol_FrameBgActive, kColourLight },
        std::pair{ ImGuiCol_Button, kColourFace },
        std::pair{ ImGuiCol_ButtonHovered, kColourFace },
        std::pair{ ImGuiCol_ButtonActive, kColourFace },
        std::pair{ ImGuiCol_Header, kColourNavy },
        std::pair{ ImGuiCol_HeaderHovered, kColourNavy },
        std::pair{ ImGuiCol_HeaderActive, kColourNavy },
        std::pair{ ImGuiCol_CheckMark, kColourDark },
        std::pair{ ImGuiCol_CheckboxSelectedBg, kColourLight },
        std::pair{ ImGuiCol_SliderGrab, kColourShadow },
        std::pair{ ImGuiCol_SliderGrabActive, kColourDark },
        std::pair{ ImGuiCol_Separator, kColourShadow },
        std::pair{ ImGuiCol_SeparatorHovered, kColourShadow },
        std::pair{ ImGuiCol_SeparatorActive, kColourShadow },
        std::pair{ ImGuiCol_Tab, kColourFace },
        std::pair{ ImGuiCol_TabHovered, kColourFace },
        std::pair{ ImGuiCol_TabSelected, kColourFace },
        std::pair{ ImGuiCol_TabSelectedOverline, kColourNavy },
        std::pair{ ImGuiCol_TabDimmed, kColourFace },
        std::pair{ ImGuiCol_TabDimmedSelected, kColourFace },
        std::pair{ ImGuiCol_TabDimmedSelectedOverline, kColourShadow },
        std::pair{ ImGuiCol_TableHeaderBg, kColourFace },
        std::pair{ ImGuiCol_TableBorderStrong, kColourShadow },
        std::pair{ ImGuiCol_TableBorderLight, kColourLightEdge },
        std::pair{ ImGuiCol_TableRowBgAlt, kColourLightEdge },
        std::pair{ ImGuiCol_ScrollbarBg, kColourScrollBg },
        std::pair{ ImGuiCol_ScrollbarGrab, kColourFace },
        std::pair{ ImGuiCol_ScrollbarGrabHovered, kColourFace },
        std::pair{ ImGuiCol_ScrollbarGrabActive, kColourFace },
        std::pair{ ImGuiCol_TextSelectedBg, kColourNavy },
        std::pair{ ImGuiCol_TextLink, kColourNavy },
        std::pair{ ImGuiCol_InputTextCursor, kColourDark },
        std::pair{ ImGuiCol_NavCursor, kColourDark },
    };

    for (const auto& [colour, nValue] : colours)
    {
        ImGui::PushStyleColor(colour, nValue);
    }

    m_nColours = static_cast<int>(colours.size());
}

OriginalUI::Theme::~Theme()
{
    ImGui::PopStyleColor(m_nColours);
    ImGui::PopStyleVar(m_nVars);
}

// Windows 98's caption is a navy-to-blue bar, the title in white, and a close box.
bool OriginalUI::DrawWindowFrame(const char* pszTitle)
{
    BevelCurrentWindow();

    auto* const pList    = ImGui::GetWindowDrawList();
    const auto  pos      = ImGui::GetCursorScreenPos();
    const auto  flWidth  = ImGui::GetContentRegionAvail().x;
    const auto  flHeight = ImGui::GetFontSize() + 5.0f;
    const auto  end      = ImVec2(pos.x + flWidth, pos.y + flHeight);

    pList->AddRectFilledMultiColor(pos, end, kColourNavy, kColourCaptionEnd, kColourCaptionEnd, kColourNavy);
    pList->AddText(ImVec2(pos.x + 4, pos.y + 2.5f), kColourLight, pszTitle);
    pList->AddText(ImVec2(pos.x + 5, pos.y + 2.5f), kColourLight, pszTitle);

    const auto box = ImVec2(flHeight + 2.0f, flHeight - 4.0f);

    ImGui::SetCursorScreenPos(ImVec2(end.x - box.x - 2.0f, pos.y + 2.0f));

    const auto bClosed  = ImGui::InvisibleButton("##close", box);
    const auto bPressed = ImGui::IsItemActive() && ImGui::IsItemHovered();
    const auto min      = ImGui::GetItemRectMin();
    const auto max      = ImGui::GetItemRectMax();

    pList->AddRectFilled(min, max, kColourFace);
    DrawBevel(pList, min, max, !bPressed);

    // The close box's cross, two pixels thick as Marlett draws it.
    const auto centre = ImVec2(std::floor((min.x + max.x) * 0.5f), std::floor((min.y + max.y) * 0.5f));
    const auto flArm  = std::floor(box.y * 0.25f) + 0.5f;

    pList->AddLine(ImVec2(centre.x - flArm, centre.y - flArm), ImVec2(centre.x + flArm, centre.y + flArm), kColourDark, 2.0f);
    pList->AddLine(ImVec2(centre.x - flArm, centre.y + flArm), ImVec2(centre.x + flArm, centre.y - flArm), kColourDark, 2.0f);

    ImGui::SetCursorScreenPos(ImVec2(pos.x, end.y + 4.0f));

    return !bClosed;
}

bool OriginalUI::BeginMenu(const char* pszLabel, bool bOnBar)
{
    const auto flWidth = [&]
    {
        if (bOnBar)
        {
            return ImGui::CalcTextSize(pszLabel).x + ImGui::GetStyle().ItemSpacing.x * 2.0f;
        }

        return ImGui::GetContentRegionAvail().x;
    }();

    const auto bWhite = PushHighlightText(pszLabel, flWidth, true);
    const auto bOpen  = ImGui::BeginMenu(pszLabel);

    if (bWhite)
    {
        ImGui::PopStyleColor();
    }

    if (bOpen)
    {
        BevelCurrentWindow();
    }

    return bOpen;
}

bool OriginalUI::MenuItem(const char* pszLabel, const char* pszShortcut, bool bChecked, bool bEnabled)
{
    const auto bWhite  = bEnabled && PushHighlightText(pszLabel, ImGui::GetContentRegionAvail().x, false);
    const auto bChosen = ImGui::MenuItem(pszLabel, pszShortcut, bChecked, bEnabled);

    if (bWhite)
    {
        ImGui::PopStyleColor();
    }

    return bChosen;
}

// The bevels are the edges, so ImGui's own border is turned off under them.
bool OriginalUI::Button(const char* pszLabel, float cx, float cy, bool bDefault)
{
    // Windows 95 sized a button to at least 75 pixels, with room around its label inside the bevel.
    constexpr auto kMinWidth = 75.0f;

    const auto size = [&]
    {
        if (cx > 0.0f || cy > 0.0f)
        {
            return ImVec2(cx, cy);
        }

        return ImVec2((std::max)(kMinWidth, ImGui::CalcTextSize(pszLabel, nullptr, true).x + 16.0f), ImGui::GetFrameHeight() + 4.0f);
    }();

    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);

    const auto bClicked = ImGui::Button(pszLabel, size);

    ImGui::PopStyleVar();

    const auto  bPressed = ImGui::IsItemActive() && ImGui::IsItemHovered();
    auto* const pList    = ImGui::GetWindowDrawList();
    auto        min      = ImGui::GetItemRectMin();
    auto        max      = ImGui::GetItemRectMax();

    if (bDefault)
    {
        pList->AddRect(min, max, kColourDark);
        min = ImVec2(min.x + 1, min.y + 1);
        max = ImVec2(max.x - 1, max.y - 1);
    }

    DrawBevel(pList, min, max, !bPressed);

    return bClicked;
}

bool OriginalUI::Checkbox(const char* pszLabel, bool* pbChecked)
{
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);

    const auto bClicked = ImGui::Checkbox(pszLabel, pbChecked);

    ImGui::PopStyleVar();

    const auto min = ImGui::GetItemRectMin();
    const auto box = ImGui::GetFrameHeight();

    DrawBevel(ImGui::GetWindowDrawList(), min, ImVec2(min.x + box, min.y + box), false);

    return bClicked;
}

bool OriginalUI::RadioButton(const char* pszLabel, bool bActive)
{
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);

    const auto bClicked = ImGui::RadioButton(pszLabel, bActive);

    ImGui::PopStyleVar();

    // ImGui's own centre and radius for the ring, so the edges sit on it.
    const auto  flSize   = ImGui::GetFrameHeight();
    const auto  min      = ImGui::GetItemRectMin();
    const auto  centre   = ImVec2(std::round(min.x + flSize * 0.5f), std::round(min.y + flSize * 0.5f));
    const auto  flRadius = (flSize - 1.0f) * 0.5f;
    auto* const pList    = ImGui::GetWindowDrawList();

    // Sunken: shadow then dark round the top left, white then light round the bottom right.
    DrawArc(pList, centre, flRadius, 0.75f, kColourShadow);
    DrawArc(pList, centre, flRadius - 1.0f, 0.75f, kColourDark);
    DrawArc(pList, centre, flRadius, 1.75f, kColourLight);
    DrawArc(pList, centre, flRadius - 1.0f, 1.75f, kColourLightEdge);

    return bClicked;
}

void OriginalUI::SeparatorText(const char* pszLabel)
{
    constexpr auto kLead = 8.0f;

    ImGui::Dummy(ImVec2(0.0f, 2.0f));

    auto* const pList   = ImGui::GetWindowDrawList();
    const auto  pos     = ImGui::GetCursorScreenPos();
    const auto  flWidth = ImGui::GetContentRegionAvail().x;
    const auto  text    = ImGui::CalcTextSize(pszLabel);
    const auto  flLine  = std::floor(pos.y + text.y * 0.5f);

    DrawEtched(pList, ImVec2(pos.x, flLine), ImVec2(pos.x + kLead - 3.0f, flLine + 2.0f));
    DrawEtched(pList, ImVec2(pos.x + kLead + text.x + 3.0f, flLine), ImVec2(pos.x + flWidth, flLine + 2.0f));

    ImGui::SetCursorScreenPos(ImVec2(pos.x + kLead, pos.y));
    ImGui::TextUnformatted(pszLabel);
}

void OriginalUI::Separator()
{
    const auto pos     = ImGui::GetCursorScreenPos();
    const auto flWidth = ImGui::GetContentRegionAvail().x;

    ImGui::Dummy(ImVec2(flWidth, 4.0f));
    DrawEtched(ImGui::GetWindowDrawList(), ImVec2(pos.x, pos.y + 1.0f), ImVec2(pos.x + flWidth, pos.y + 3.0f));
}

bool OriginalUI::BeginTabBar(const char* pszID)
{
    // EndTabBar draws the edges once every tab is laid out, so ImGui draws none of its own.
    ImGui::PushStyleVar(ImGuiStyleVar_TabBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_TabBarBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_TabBarOverlineSize, 0.0f);

    s_tabs.clear();

    if (ImGui::BeginTabBar(pszID))
    {
        return true;
    }

    ImGui::PopStyleVar(3);

    return false;
}

bool OriginalUI::BeginTabItem(const char* pszLabel, bool bSelect)
{
    const auto flags = [&]
    {
        if (bSelect)
        {
            return ImGuiTabItemFlags_SetSelected;
        }

        return ImGuiTabItemFlags_None;
    }();

    const auto bOpen = ImGui::BeginTabItem(pszLabel, nullptr, flags);

    s_tabs.push_back({ ImGui::GetItemRectMin(), ImGui::GetItemRectMax(), bOpen });

    if (bOpen)
    {
        ImGui::Dummy(ImVec2(0.0f, kSheetInset * 0.5f));
        ImGui::Indent(kSheetInset);
    }

    return bOpen;
}

void OriginalUI::EndTabItem()
{
    ImGui::Unindent(kSheetInset);
    ImGui::EndTabItem();
}

void OriginalUI::EndTabBar()
{
    ImGui::EndTabBar();
    ImGui::PopStyleVar(3);

    if (s_tabs.empty())
    {
        return;
    }

    auto* const pList = ImGui::GetWindowDrawList();
    const auto& style = ImGui::GetStyle();
    const auto  pos   = ImGui::GetWindowPos();
    const auto  x0    = std::floor(pos.x + style.WindowPadding.x);
    const auto  x1    = std::floor(pos.x + ImGui::GetWindowSize().x - style.WindowPadding.x);
    const auto  y0    = std::floor(s_tabs.front().max.y);
    const auto  y1    = std::floor(ImGui::GetCursorScreenPos().y);

    // The window grows to its last item, so this keeps the panel's bottom edge inside it.
    ImGui::Dummy(ImVec2(0.0f, kSheetInset * 0.5f));

    pList->PushClipRectFullScreen();
    DrawBevel(pList, ImVec2(x0, y0), ImVec2(x1, y1), true);

    // The selected tab last, so it stands over its neighbours.
    for (const auto& tab : s_tabs)
    {
        if (!tab.bSelected)
        {
            DrawTab(pList, tab);
        }
    }

    for (const auto& tab : s_tabs)
    {
        if (tab.bSelected)
        {
            DrawTab(pList, tab);
        }
    }

    pList->PopClipRect();
}

bool OriginalUI::Selectable(const char* pszLabel, bool bSelected, int nFlags)
{
    const auto nRow = [&]
    {
        if (bSelected)
        {
            return kColourNavy;
        }

        return IM_COL32(0, 0, 0, 0);
    }();

    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, nRow);
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, nRow);

    if (bSelected)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, kColourLight);
    }

    const auto bClicked = ImGui::Selectable(pszLabel, bSelected, nFlags);

    if (bSelected)
    {
        ImGui::PopStyleColor();
    }

    ImGui::PopStyleColor(2);

    return bClicked;
}

void OriginalUI::SetItemTooltip(const char* pszFormat, ...)
{
    ImGui::PushStyleColor(ImGuiCol_PopupBg, kColourTooltip);
    ImGui::PushStyleColor(ImGuiCol_Border, kColourDark);
    ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 1.0f);

    va_list args;

    va_start(args, pszFormat);
    ImGui::SetItemTooltipV(pszFormat, args);
    va_end(args);

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);
}

void OriginalUI::DrawMenus()
{
    Load();

    DrawEntries(s_menu, true);

    if (!BeginMenu("Dialogs", true))
    {
        return;
    }

    if (s_dialogs.empty())
    {
        ImGui::TextDisabled("No HYPERX.EXE beside the port to read them from.");
    }

    for (const auto& dialog : s_dialogs)
    {
        const auto strLabel = std::to_string(dialog.nID) + "  " + DialogName(dialog.nID);

        if (MenuItem(strLabel.c_str(), nullptr, false, true))
        {
            Open(dialog.nID);
        }
    }

    ImGui::EndMenu();
}

void OriginalUI::DrawDialogs()
{
    const auto theme = Theme();
    const auto open  = s_open;

    for (const auto nDialog : open)
    {
        const auto it = std::find_if(
            s_dialogs.begin(),
            s_dialogs.end(),
            [&](const DialogTemplates::Dialog& dialog)
            {
                return dialog.nID == nDialog;
            });

        if (it == s_dialogs.end() || !DrawDialog(*it))
        {
            std::erase(s_open, nDialog);
        }
    }
}

void OriginalUI::OpenDialog(unsigned short nDialog)
{
    Load();
    Open(nDialog);
}

bool OriginalUI::HasOpenDialogs()
{
    return !s_open.empty();
}
