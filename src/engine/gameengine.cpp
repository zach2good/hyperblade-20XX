#include "common/compat.h"
#include "common/clock.h"
#include <cstdint>
#include "planvm/gamebehaviorobject.h"
#include "planvm/decisionlog.h"
#include "gameengine.h"
#include "engine/framepacing.h"
#include "globals.h"
#include "planvm/planvmglobals.h"
#include "common/pooledownerroot.h"
#include "common/cglideonsurfacepool.h"
#include "common/cslideonrailpool.h"
#include "common/cprojectandbouncepool.h"
#include "common/cbeattachedpool.h"
#include "common/csteertotargetpool.h"
#include "common/csetpositionrelativepool.h"
#include "common/csteertopositionpool.h"
#include "common/csteerbetweenfacingpool.h"
#include "common/csteertowardpool.h"
#include "common/cadjustaccelerationpool.h"
#include "common/cavoidobjectspool.h"
#include "common/cbungeetowardpool.h"
#include "common/csetattitudepool.h"
#include "common/crelaxyawpool.h"
#include "common/cstepanimationpool.h"
#include "common/cmonitorkeyspool.h"
#include "common/cmonitorheldkeyspool.h"
#include "planvm/cspeedburstpool.h"
#include "common/crelaxtopositionpool.h"
#include "common/ctimeractivitypool.h"
#include "common/ctrackpool.h"
#include "common/ctracktrajpool.h"
#include "common/cwatchpool.h"
#include "common/ctrackpasserreceiverpool.h"
#include "common/cviewitpool.h"
#include "common/cpansoundpool.h"
#include "common/cdopplersndpool.h"
#include "common/cheadtrackpool.h"
#include "common/cpaletteeffectpool.h"
#include "common/cdisplayshotpowerpool.h"
#include "common/cupdateshotpowerpool.h"
#include "common/cdisplaypassbarpool.h"
#include "common/cexplosionanimationpool.h"
#include "common/ccheckaipool.h"
#include "common/ccircletrackpool.h"
#include "objectdef.h"
#include "devicector.h"
#include "planvm/planvm.h"
#include "objectclasses.h"
#include "engine/collisionobjectids.h"
#include "engine/messageids.h"
#include "engine/objectkind.h"
#include "engine/modelids.h"
#include "engine/dromeobject.h"
#include "engine/goalobject.h"
#include "engine/weaponobject.h"
#include "physics/physicsbody.h"
#include "common/staticinit.h"
#include "planvm/damagegate.h"
#include "math/vector.h"
#include "common/pooledclasses.h"
#include "cameraobject.h"
#include "engineglobals.h"
#include "gameoptions.h"
#include "net/session.h"
#include "arenastate.h"
#include "teamplayerrow.h"
#include "graphics/modeldatabase.h"
#include "graphics/meshmaterial.h"
#include "ui/uiglobals.h"
#include "teamsblock.h"
#include "ui/torchbitmaps.h"
#include "ui/saveload.h"
#include "ui/screens/dromeexteriorscreen.h"
#include "statids.h"
#include "objectstats.h"
#include "messagequeue.h"
#include "graphics/texcacheentry.h"
#include "common/fileobj.h"
#include "common/pathstring.h"
struct GameApp;
#include "physics/boundbubble.h"
#include "graphics/renderer.h"
#include "ui/infobarbase.h"
#include "teamblock.h"
#include "input/keyboarddeviceobj.h"
#include "rampobject.h"
#include "planvm/planopcodetable.h"
#include "saveblock.h"
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <new>
#include <cstdio>
#include "playerobject.h"
#include "sound/audiomgr.h"
#include "rokobject.h"
#include "common/stringpool.h"
#include "app/gameapp.h"
#include "net/gamesession.h"
#include "net/cplayer.h"
#include "particlesystem.h"
#include "planvm/collinteractor.h"
#include "ui/saveobject.h"
#include "engine/engineobject.h"
#include "engine/coachobject.h"
#include "ui/infobar.h"
#include "planvm/behaviorregistry.h"
#include "math/mathops.h"
#include "engine/msgevent.h"
#include "engine/planbehaviourword.h"
#include "common/parseutil.h"
#include "planvm/planinteractorfactories.h"
#include "app/window.h"
#include <SDL3/SDL.h>
#include "app/stringids.h"
#include "sound/soundids.h"
#include "engine/objectphase.h"

float        g_flUnusedAgendaItemA = g_flZeroAgendaItem;
float        g_flUnusedAgendaItemB = g_flZeroAgendaItem;
float        g_flUnusedAgendaItemC = g_flZeroAgendaItem;
float        g_flUnusedGameEngineA = g_flZeroGameEngine;
float        g_flUnusedGameEngineB = g_flZeroGameEngine;
float        g_flUnusedGameEngineC = g_flZeroGameEngine;
SurfacePoint g_ptPlanEditorOrigin(0x1c, 0x1c);

#define STAT_MINIMUM 100.0f

constexpr auto kTeamKeywordCount = 9;

// FUNCTION: HYPERX 0x0040b970
void GameEngine::InitAgendaItemPools()
{
    int nSlot;

    PooledOwnerRoot** ppEntry = g_apAgendaItemPools;

    for (nSlot = 0; nSlot < Agenda_Count; nSlot++)
    {
        if (*ppEntry != nullptr)
        {
            (*ppEntry)->Init();
        }

        ppEntry++;
    }
}

// The statement order is the original's: the first pool built goes into slot eleven.
// FUNCTION: HYPERX 0x0040bbf0
void GameEngine::CreateAgendaItemPools()
{
    g_apAgendaItemPools[Agenda_CGlideOnSurface]       = new CGlideOnSurfacePool;
    g_apAgendaItemPools[Agenda_CSlideOnRail]          = new CSlideOnRailPool;
    g_apAgendaItemPools[Agenda_CProjectAndBounce]     = new CProjectAndBouncePool;
    g_apAgendaItemPools[Agenda_CBeAttached]           = new CBeAttachedPool;
    g_apAgendaItemPools[Agenda_CSteerToTarget]        = new CSteerToTargetPool;
    g_apAgendaItemPools[Agenda_CSetPositionRelative]  = new CSetPositionRelativePool;
    g_apAgendaItemPools[Agenda_CSteerToPosition]      = new CSteerToPositionPool;
    g_apAgendaItemPools[Agenda_CSteerBetweenFacing]   = new CSteerBetweenFacingPool;
    g_apAgendaItemPools[Agenda_CSteerToward]          = new CSteerTowardPool;
    g_apAgendaItemPools[Agenda_CAdjustAcceleration]   = new CAdjustAccelerationPool;
    g_apAgendaItemPools[Agenda_CAvoidObjects]         = new CAvoidObjectsPool;
    g_apAgendaItemPools[Agenda_CBungeeToward]         = new CBungeeTowardPool;
    g_apAgendaItemPools[Agenda_CSetAttitude]          = new CSetAttitudePool;
    g_apAgendaItemPools[Agenda_CRelaxYaw]             = new CRelaxYawPool;
    g_apAgendaItemPools[Agenda_CStepAnimation]        = new CStepAnimationPool;
    g_apAgendaItemPools[Agenda_CMonitorKeys]          = new CMonitorKeysPool;
    g_apAgendaItemPools[Agenda_CMonitorHeldKeys]      = new CMonitorHeldKeysPool;
    g_apAgendaItemPools[Agenda_CSpeedBurst]           = new CSpeedBurstPool;
    g_apAgendaItemPools[Agenda_CRelaxToPosition]      = new CRelaxToPositionPool;
    g_apAgendaItemPools[Agenda_CTimerActivity]        = new CTimerActivityPool;
    g_apAgendaItemPools[Agenda_CTrack]                = new CTrackPool;
    g_apAgendaItemPools[Agenda_CTrackTraj]            = new CTrackTrajPool;
    g_apAgendaItemPools[Agenda_CWatch]                = new CWatchPool;
    g_apAgendaItemPools[Agenda_CTrackPasser_Receiver] = new CTrackPasser_ReceiverPool;
    g_apAgendaItemPools[Agenda_CViewIt]               = new CViewItPool;
    g_apAgendaItemPools[Agenda_CPanSound]             = new CPanSoundPool;
    g_apAgendaItemPools[Agenda_CDopplerSnd]           = new CDopplerSndPool;
    g_apAgendaItemPools[Agenda_CHeadTrack]            = new CHeadTrackPool;
    g_apAgendaItemPools[Agenda_CPaletteEffect]        = new CPaletteEffectPool;
    g_apAgendaItemPools[Agenda_CDisplayShotPower]     = new CDisplayShotPowerPool;
    g_apAgendaItemPools[Agenda_CUpdateShotPower]      = new CUpdateShotPowerPool;
    g_apAgendaItemPools[Agenda_CDisplayPassBar]       = new CDisplayPassBarPool;
    g_apAgendaItemPools[Agenda_CExplosionAnimation]   = new CExplosionAnimationPool;
    g_apAgendaItemPools[Agenda_CCheckAI]              = new CCheckAIPool;
    g_apAgendaItemPools[Agenda_CCircleTrack]          = new CCircleTrackPool;
    g_apAgendaItemPools[Agenda_Count]                 = nullptr;
}

// Runs each pool's Destroy slot and stores its answer back, stopping at the first null.

// FUNCTION: HYPERX 0x0040c070
void GameEngine::DestroyAgendaItemPools()
{
    short iSlot = 0;

    auto* pNode = g_apAgendaItemPools[0];

    while (pNode != nullptr)
    {
        g_apAgendaItemPools[iSlot] = pNode->Destroy();

        iSlot++;

        pNode = g_apAgendaItemPools[iSlot];
    }
}

// Colours the multiplier's two materials by the rok's value; the first call saves their rest colours.

// FUNCTION: HYPERX 0x00463220
void GameEngine::ShowRokValue(short nRokValue)
{
    auto* const pObject = g_pGameEngine->m_ppSlots[kSlotMultiplier];

    if (pObject == nullptr)
    {
        return;
    }

    auto* const pMaterials = pObject->m_pModelBlock->m_pMaterials;

    if (g_bMultiplierColoursUnsaved != 0)
    {
        g_wMultiplierRestColour1    = static_cast<short>(pMaterials[1].nColour);
        g_wMultiplierRestColour2    = static_cast<short>(pMaterials[2].nColour);
        g_bMultiplierColoursUnsaved = 0;
    }

    switch (nRokValue)
    {
        case -4:
            pMaterials[1].nColour = g_nMultiplierColourNegative;
            pMaterials[2].nColour = g_nMultiplierColourNegative;
            break;

        case -2:
            pMaterials[1].nColour = g_wMultiplierRestColour1;
            pMaterials[2].nColour = g_nMultiplierColourNegative;
            break;

        case 2:
            pMaterials[1].nColour = g_wMultiplierRestColour1;
            pMaterials[2].nColour = g_nMultiplierColourPositive;
            break;

        case 4:
            pMaterials[1].nColour = g_nMultiplierColourPositive;
            pMaterials[2].nColour = g_nMultiplierColourPositive;
            break;

        default:
            pMaterials[1].nColour = g_wMultiplierRestColour1;
            pMaterials[2].nColour = g_wMultiplierRestColour2;
            break;
    }
}

// Counts the mines, then the swarfs, within ten units of slot nWhich.
// FUNCTION: HYPERX 0x00465530
int GameEngine::CountEntitiesInRange(short nWhich)
{
    int           iEntity;
    EngineObject* pSelf;
    EngineObject* pOther;
    const Vec3*   pSelfPos;
    const Vec3*   pOtherPos;
    float         flDx;
    float         flDy;
    float         flDz;

    const auto nFirst  = g_pGameEngine->GetMineCount();
    const auto nSecond = g_pGameEngine->GetSwarfCount();

    auto nInRange = 0;

    for (iEntity = 0; iEntity < nFirst; iEntity++)
    {
        pSelf  = g_pGameEngine->m_ppSlots[nWhich];
        pOther = g_pGameEngine->m_ppSlots[static_cast<short>(iEntity + kSlotMine)];

        pSelfPos  = pSelf->GetPositionPtr();
        pOtherPos = pOther->GetPositionPtr();

        flDx = pOtherPos->x - pSelfPos->x;
        flDy = pOtherPos->y - pSelfPos->y;
        flDz = pOtherPos->z - pSelfPos->z;

        if (static_cast<float>(sqrt(static_cast<double>(flDz * flDz + flDy * flDy + flDx * flDx))) <= 10.0f)
        {
            nInRange++;
        }
    }

    for (iEntity = 0; iEntity < nSecond; iEntity++)
    {
        pSelf  = g_pGameEngine->m_ppSlots[nWhich];
        pOther = g_pGameEngine->m_ppSlots[static_cast<short>(iEntity + kSlotSwarf)];

        pSelfPos  = pSelf->GetPositionPtr();
        pOtherPos = pOther->GetPositionPtr();

        flDx = pOtherPos->x - pSelfPos->x;
        flDy = pOtherPos->y - pSelfPos->y;
        flDz = pOtherPos->z - pSelfPos->z;

        if (static_cast<float>(sqrt(static_cast<double>(flDz * flDz + flDy * flDy + flDx * flDx))) <= 10.0f)
        {
            nInRange++;
        }
    }

    return nInRange;
}

// The resting byte of four of the five queue tables, repeated over each table.
constexpr auto kQueueTargetByte = 0x8e;
constexpr auto kQueueIdleByte   = 0x66;
constexpr auto kQueueFlagByte   = 0x02;

constexpr auto kHiddenTeamA = 8;
constexpr auto kHiddenTeamB = 2;

// FUNCTION: HYPERX 0x00469380
int GameEngine::IsTeamWinning(unsigned char nTeam)
{
    if ((nTeam & 1) != 0)
    {
        return g_pGameEngine->GetAwayScore() > g_pGameEngine->GetHomeScore();
    }

    return g_pGameEngine->GetHomeScore() > g_pGameEngine->GetAwayScore();
}

// FUNCTION: HYPERX 0x0047acf0
unsigned short GameEngine::TogglePause(int bRestyle)
{
    if (g_pGameEngine == nullptr)
    {
        return 1;
    }

    if (g_pGameEngine->m_wRunState == kEngineRunPlaying)
    {
        g_pGameEngine->SetRunState(0, 0);

        g_nSavedCursorState = GameApp::SetCursorVisible(1);

        if (bRestyle != 0)
        {
            Window::ApplyFullscreenStyle();
        }
    }
    else
    {
        g_pGameEngine->SetRunState(1, 0);

        GameApp::SetCursorVisible(g_nSavedCursorState);

        if (bRestyle != 0)
        {
            Window::ApplyFullscreenStyle();
        }
    }

    return g_pGameEngine->m_wRunState;
}

// FUNCTION: HYPERX 0x00480d20
GameEngine::GameEngine()
{
    m_nLoadCost        = 0;
    m_nLoadedCount     = 0;
    m_nObjectCount     = 0;
    m_nQueuedSlotCount = 0;
    m_nSlotCount       = 0;
    m_ppSlots          = nullptr;
    m_ppObjects        = nullptr;
    memset(m_apCameras, 0, sizeof(m_apCameras));

    m_wCameraCount = 0;

    memset(m_abTypeHighWater, 0, sizeof(m_abTypeHighWater));

    m_nViewedSlot = 0;

    memset(m_ppColObjects, 0, sizeof(m_ppColObjects));

    m_wRunState     = kEngineRunStopped;
    m_nPendingCount = 0;

    memset(m_aQueueMessage, kQueueTargetByte, sizeof(m_aQueueMessage));
    memset(m_aQueueSlot, kQueueIdleByte, sizeof(m_aQueueSlot));
    memset(m_aQueueArg, kQueueIdleByte, sizeof(m_aQueueArg));
    memset(m_aQueueFromInput, kQueueFlagByte, sizeof(m_aQueueFromInput));

    memset(&m_clock, 0, sizeof(m_clock));

    m_flGravityDt     = g_flZeroGameEngine;
    m_flPrevGravityDt = g_flZeroGameEngine;

    m_wUnusedB     = 0;
    m_wUnusedC     = 0;
    m_wUnusedD     = 0;
    m_wDeviceCount = 0;

    m_apTracked[0] = nullptr;
    m_apTracked[1] = nullptr;
    m_pTeamA       = nullptr;
    m_pTeamB       = nullptr;
    m_pArena       = nullptr;

    m_nTeamHomeIndex = 5;
    m_nTeamAwayIndex = 10;
    m_nArenaIndex    = 1;

    m_nPending = 0;

    auto* const pTeams = new TeamsBlock;

    m_pTeamTable = pTeams;

    auto* const pSave = new SaveObject(kHiddenTeamA, kHiddenTeamB);

    m_pSaveObject = pSave;

    // STRING: HYPERX 0x004c7268
    pSave->ParseGauntletConfig("gauntlet.cfg");

    m_nScoreHome = -1;
    m_nScoreAway = -1;

    m_nDifficulty  = static_cast<DifficultyLevel>(g_pGameOptions->ReadDifficulty());
    m_nBrutality   = g_pGameOptions->ReadBrutality();
    m_periodLength = Clock::Milliseconds{ g_pGameOptions->ReadPeriodLength() };

    m_nAttackMode   = g_pGameOptions->ReadInt(g_pszOptionsSection, g_pszAttackModeKey, 0);
    m_nTeammateMode = g_pGameOptions->ReadInt(g_pszOptionsSection, g_pszTeammateModeKey, 0);

    g_bSkipAssetValidation = g_pGameOptions
                                 // STRING: HYPERX 0x004c725c
                                 ->ReadInt(g_pszOptionsSection, "NetNoVerify", 0);

    g_bModemPlay = g_pGameOptions
                       // STRING: HYPERX 0x004c7250
                       ->ReadInt(g_pszOptionsSection, "ModemPlay", 0);

    m_nScreenSize = -1;

    ClearAwards();

    m_bMatchOver          = 0;
    m_bEndPeriodRequested = 0;
}

constexpr auto kAwardAggregate = 0;
constexpr auto kAwardAnvil     = 1;
constexpr auto kAwardCrush     = 2;
constexpr auto kAwardArcher    = 3;
constexpr auto kAwardSuper     = 4;
constexpr auto kAwardRazor     = 5;

constexpr auto kAwardJudgedFirst = 1;
constexpr auto kAwardJudged      = 4;

// The home score that wins the Super award.
constexpr auto kSuperAwardScore = 0x1e;

constexpr auto kBracketEnd = (-1);

// A drawn score with any away row still damaged restarts the bracket; awards nought and five
// are written and never read.
// FUNCTION: HYPERX 0x00480fc0
void GameEngine::AdvanceTournamentBracket()
{
    int aAwardReady[kAwardCount];
    int nIndex;
    int nRemaining;
    int nEntry;

    memset(aAwardReady, 0, sizeof(aAwardReady));

    SyncPlayerDataToTeam();

    if (m_nScoreHome == m_nScoreAway && m_nScoreHome != 0)
    {
        for (nIndex = 0; nIndex < kTeamRowCount; nIndex++)
        {
            if (m_pTeamB->aRows[nIndex].aStat[kRatingDamage] > 0.0f)
            {
                m_pSaveObject->BracketIterReset();

                nEntry = m_pSaveObject->BracketIterPeek();

                g_nDromeTeamAway = nEntry;
                g_nDromeArena    = nEntry;

                ClearAwards();

                m_bMatchOver = 1;

                return;
            }
        }
    }
    else if (m_nScoreHome <= m_nScoreAway)
    {
        m_pSaveObject->BracketIterReset();

        nEntry = m_pSaveObject->BracketIterPeek();

        g_nDromeTeamAway = nEntry;
        g_nDromeArena    = nEntry;

        ClearAwards();

        m_bMatchOver = 1;

        return;
    }

    nEntry = m_pSaveObject->BracketIterAdvance();

    g_nDromeTeamAway = nEntry;
    g_nDromeArena    = nEntry;

    if (nEntry == kBracketEnd)
    {
        g_pGameEngine->SetAward(kAwardRazor, 1);
    }

    if (m_nScoreHome >= kSuperAwardScore)
    {
        SetAward(kAwardSuper, 1);

        aAwardReady[kAwardSuper] = 1;
    }

    if (m_nScoreAway == 0)
    {
        SetAward(kAwardCrush, 1);

        aAwardReady[kAwardCrush] = 1;
    }

    aAwardReady[kAwardArcher] = static_cast<unsigned char>(EvalArcherAward());
    aAwardReady[kAwardAnvil]  = static_cast<unsigned char>(EvalAnvilAward());

    char bAllReady = 1;

    auto* pReady = &aAwardReady[kAwardJudgedFirst];

    for (nRemaining = 0; nRemaining < kAwardJudged; nRemaining++)
    {
        if (*pReady == 0)
        {
            bAllReady = 0;
        }

        pReady++;
    }

    if (HasAward(kAwardAggregate) == 0)
    {
        SetAward(kAwardAggregate, bAllReady);
    }
}

#define MAX_FRAME_TIME_MS 200.0f
#define MS_TO_SEC         0.001f

constexpr auto kCountdownStart = Clock::Milliseconds{ 10000 };
constexpr auto kCountdownSecs  = 9;

constexpr auto kFinalPeriodIndex = 3;
constexpr auto kPeriodCount      = 4;

constexpr auto kOvertimeEarly = Clock::Milliseconds{ 10000 };
constexpr auto kOvertimeFinal = Clock::Milliseconds{ 30000 };

// The frame delta, in milliseconds before the scaling and in seconds after it.
extern float g_flFrameDelta;

// Tells the second countdown test the first one has already armed.
extern unsigned char g_bPeriodCountdownActive;

// The five-slot ring holds the mean.
// FUNCTION: HYPERX 0x00481130
void GameEngine::TickClock()
{
    Clock::Milliseconds overtime;
    Clock::Milliseconds elapsed;

    m_clock.flPrevDelta           = m_clock.flDelta;
    m_clock.flPrevDeltaSquared    = m_clock.flDeltaSquared;
    m_clock.flPrevInvDelta        = m_clock.flInvDelta;
    m_clock.flPrevInvDeltaSquared = m_clock.flInvDeltaSquared;
    m_clock.prevTick              = m_clock.tick;

    m_clock.tick = Clock::Now();

    // The simulation steps a fixed tick, however long the frame took to draw.
    g_flFrameDelta = FramePacing::kTickMs;

    g_flFrameDelta = g_flFifthGameEngine * g_flFrameDelta * MS_TO_SEC;

    const auto nSlot       = m_clock.nRingIndex;
    m_clock.flDelta        = m_clock.flDelta - m_clock.aflDeltaRing[nSlot] + g_flFrameDelta;
    m_clock.flDeltaSquared = m_clock.flDelta * m_clock.flDelta;

    m_clock.flInvDelta        = g_flOneGameEngine / m_clock.flDelta;
    m_clock.flInvDeltaSquared = m_clock.flInvDelta * m_clock.flInvDelta;

    m_clock.aflDeltaRing[nSlot] = g_flFrameDelta;
    m_clock.nRingIndex          = static_cast<short>(static_cast<short>(m_clock.nRingIndex + 1) % kClockRingLength);

    if (m_clock.nPeriod >= kFinalPeriodIndex)
    {
        overtime = kOvertimeFinal;
    }
    else
    {
        overtime = kOvertimeEarly;
    }

    if (m_clock.nPauseMark != 1)
    {
        if (m_clock.tick - m_clock.periodStart < m_periodLength && m_bEndPeriodRequested == 0 && AreAllPlayersInjured() == 0 && IsHomeTeamInjured() == 0 && IsAwayTeamInjured() == 0)
        {
            return;
        }
    }

    if (m_clock.nPauseMark == 1)
    {
        m_clock.pauseStart = m_clock.tick;
        elapsed            = m_clock.tick - m_clock.periodStart;
    }
    else
    {
        elapsed = m_clock.pauseStart - m_clock.periodStart;
    }

    if (g_bPeriodCountdownActive == 0)
    {
        if (m_periodLength - elapsed <= kCountdownStart)
        {
            g_bPeriodCountdownActive = 1;
            g_nCountdownLastSecs     = kCountdownSecs;
            g_pAudioMgr->PlaySpecific(kSoundEndsir);
        }
    }

    if (g_bPeriodCountdownActive != 0)
    {
        const auto nSecs = static_cast<int>((m_periodLength - elapsed).count()) / 1000;

        if (nSecs < g_nCountdownLastSecs)
        {
            g_nCountdownLastSecs = nSecs;
            g_pAudioMgr->PlaySpecific(kSoundEndsir);
        }
    }

    if (elapsed < m_periodLength || g_bDragOff != 0)
    {
        if (m_clock.nPauseMark == 0 && (IsHomeTeamInjured() != 0 || IsAwayTeamInjured() != 0))
        {
            m_bEndPeriodRequested = 1;
        }

        if (AreAllPlayersInjured() != 0 || m_bEndPeriodRequested != 0)
        {
            m_clock.periodStart = m_clock.pauseStart - m_periodLength;

            if (IsHomeTeamDead() != 0 || IsAwayTeamDead() != 0)
            {
                m_clock.nPeriod = kFinalPeriodIndex;
            }

            m_clock.nPauseMark = 1;
        }
    }
    else if (m_bPeriodEnded == 0)
    {
        if (m_clock.nPeriod == kFinalPeriodIndex)
        {
            BroadcastEvent(kMsgEndOfGame, 0, kSlotGod);
        }
        else
        {
            BroadcastEvent(kMsgEndOfPeriod, 0, kSlotGod);
        }

        if (IsHomeTeamDead() != 0)
        {
            g_wActiveScoreLine = 1;
        }
        else if (IsAwayTeamDead() != 0)
        {
            g_wActiveScoreLine = 0;
        }
        else
        {
            g_wActiveScoreLine = static_cast<short>(GetHomeScore() <= GetAwayScore());
        }

        m_clock.nPauseMark = 0;
        m_bPeriodEnded     = 1;
    }

    if (g_nPlayerMode == kPlayerModeExhibition && (m_clock.tick - m_clock.periodStart) >= m_periodLength + overtime && elapsed >= m_periodLength && g_bDragOff == 0)
    {
        m_clock.nPeriod++;

        if (m_clock.nPeriod == kPeriodCount)
        {
            m_bMatchOver = 1;

            if (g_bSessionActive != 0 && g_pGameSession != nullptr)
            {
                GameSession::Disconnect();
            }
        }

        g_gameApp.m_nEngineState = kEstateJumbotron;
        g_nPrevEngineState       = kEstateMatchLive;
    }

    if (IsTournamentMode() != 0 && (m_clock.tick - m_clock.periodStart) >= m_periodLength + overtime && elapsed >= m_periodLength && g_bDragOff == 0)
    {
        m_clock.nPeriod++;

        if (m_clock.nPeriod == kPeriodCount)
        {
            AdvanceTournamentBracket();
        }

        g_gameApp.m_nEngineState = kEstateJumbotron;
        g_nPrevEngineState       = kEstateMatchLive;
        GameEngine::TogglePause(0);
    }
}

// FUNCTION: HYPERX 0x00481590
char GameEngine::AreAllPlayersInjured()
{
    float flLimit;
    int   i;

    char bAll = 1;

    for (i = 0; i < kPlayerSlotCount; i++)
    {
        flLimit = DamageGate();

        auto* const pPlayer = GetPlayer(static_cast<short>(i));

        if (pPlayer->m_stats.GetScalar(kRatingDamage) > flLimit)
        {
            bAll = 0;
            break;
        }
    }

    return bAll;
}

// FUNCTION: HYPERX 0x00481620
unsigned char GameEngine::IsHomeTeamInjured()
{
    char bBelow = 1;
    int  i;

    for (i = 0; i < kTeamRowCount; i++)
    {
        if (m_pTeamA->aSeat[0] == i)
        {
            float         flSeat0Gate;
            PlayerObject* pSeat0;

            flSeat0Gate = DamageGate();

            pSeat0 = GetPlayer(0);

            if (pSeat0->m_stats.GetScalar(kRatingDamage) > flSeat0Gate)
            {
                bBelow = 0;
                break;
            }
        }
        else if (m_pTeamA->aSeat[1] == i)
        {
            float         flSeat1Gate;
            PlayerObject* pSeat1;

            flSeat1Gate = DamageGate();

            pSeat1 = GetPlayer(2);

            if (pSeat1->m_stats.GetScalar(kRatingDamage) > flSeat1Gate)
            {
                bBelow = 0;
                break;
            }
        }
        else
        {
            if (DamageGate() < m_pTeamA->aRows[i].aStat[kRatingDamage])
            {
                bBelow = 0;
                break;
            }
        }
    }

    return bBelow;
}

// FUNCTION: HYPERX 0x004817a0
unsigned char GameEngine::IsHomeTeamDead()
{
    char bBelow = 1;
    int  i;

    for (i = 0; i < kTeamRowCount; i++)
    {
        if (m_pTeamA->aSeat[0] == i)
        {
            PlayerObject* pSeat0;

            pSeat0 = GetPlayer(0);

            if (pSeat0->m_stats.GetScalar(kRatingDamage) > g_flZeroGameEngine)
            {
                bBelow = 0;
                break;
            }
        }
        else if (m_pTeamA->aSeat[1] == i)
        {
            PlayerObject* pSeat1;

            pSeat1 = GetPlayer(2);

            if (pSeat1->m_stats.GetScalar(kRatingDamage) > g_flZeroGameEngine)
            {
                bBelow = 0;
                break;
            }
        }
        else
        {
            if (m_pTeamA->aRows[i].aStat[kRatingDamage] > g_flZeroGameEngine)
            {
                bBelow = 0;
                break;
            }
        }
    }

    return bBelow;
}

// FUNCTION: HYPERX 0x00481820
unsigned char GameEngine::IsAwayTeamInjured()
{
    char bBelow = 1;
    int  i;

    for (i = 0; i < kTeamRowCount; i++)
    {
        if (m_pTeamB->aSeat[0] == i)
        {
            float         flSeat0Gate;
            PlayerObject* pSeat0;

            flSeat0Gate = DamageGate();

            pSeat0 = GetPlayer(1);

            if (pSeat0->m_stats.GetScalar(kRatingDamage) > flSeat0Gate)
            {
                bBelow = 0;
                break;
            }
        }
        else if (m_pTeamB->aSeat[1] == i)
        {
            float         flSeat1Gate;
            PlayerObject* pSeat1;

            flSeat1Gate = DamageGate();

            pSeat1 = GetPlayer(3);

            if (pSeat1->m_stats.GetScalar(kRatingDamage) > flSeat1Gate)
            {
                bBelow = 0;
                break;
            }
        }
        else
        {
            if (DamageGate() < m_pTeamB->aRows[i].aStat[kRatingDamage])
            {
                bBelow = 0;
                break;
            }
        }
    }

    return bBelow;
}

extern unsigned char g_bPeriodCountdownActive;

#define FULL_WEIGHT 100.0f

constexpr auto kBehaviorIdle = 0;
// The Rok value that leaves the multiplier its rest colours.
constexpr auto kRokValuePlain = 0;

constexpr auto kFirstPeriod = 1;

// Rows of the message queue's text table, the lines they go on, and how long they stay.
constexpr auto kTextExhibitionMode = 0x58;
constexpr auto kTextGauntletMode   = 0x59;
constexpr auto kTextDifficultyBase = 0x5a;
constexpr auto kTextLineMain       = 1;
constexpr auto kTextLineNames      = 2;
constexpr auto kTextDuration       = 0x640;

// FUNCTION: HYPERX 0x004819a0
unsigned char GameEngine::IsAwayTeamDead()
{
    char bBelow = 1;
    int  i;

    for (i = 0; i < kTeamRowCount; i++)
    {
        if (m_pTeamB->aSeat[0] == i)
        {
            PlayerObject* pSeat0;

            pSeat0 = GetPlayer(1);

            if (pSeat0->m_stats.GetScalar(kRatingDamage) > g_flZeroGameEngine)
            {
                bBelow = 0;
                break;
            }
        }
        else if (m_pTeamB->aSeat[1] == i)
        {
            PlayerObject* pSeat1;

            pSeat1 = GetPlayer(3);

            if (pSeat1->m_stats.GetScalar(kRatingDamage) > g_flZeroGameEngine)
            {
                bBelow = 0;
                break;
            }
        }
        else
        {
            if (m_pTeamB->aRows[i].aStat[kRatingDamage] > g_flZeroGameEngine)
            {
                bBelow = 0;
                break;
            }
        }
    }

    return bBelow;
}

// The order is the original's: reset flags, park the rok, idle the AI, then scale the stats.

// FUNCTION: HYPERX 0x00481a20
void GameEngine::BeginMatchPlay()
{
    PlayerObject*    pObject;
    const TeamBlock* pTeam;
    short            nIndex;
    short            nTeam;
    short            nRow;
    int              nSlot;

    m_bEndPeriodRequested    = 0;
    m_bPeriodEnded           = 0;
    m_bFollowingOther        = 0;
    g_bPeriodCountdownActive = 0;

    auto* const pRok = (RokObject*)m_ppSlots[kSlotRok];

    if (pRok != nullptr)
    {
        pRok->m_wRokValue = 0;
        pRok->SetRenderSlotFlags();
        pRok->ActivateAtSpawnSlot(kSpawnSlotAny);
        pRok->GetPhysicsBody()->pAttach = nullptr;
    }

    if (m_ppSlots[kSlotRedRandomizer] != nullptr)
    {
        m_ppSlots[kSlotRedRandomizer]->GetPlanState()->ActivateBehavior(kBehaviorIdle);
    }

    if (m_ppSlots[kSlotMine] != nullptr)
    {
        m_ppSlots[kSlotMine]->GetPhysicsBody()->pAttach = nullptr;
        m_ppSlots[kSlotMine]->GetPlanState()->ActivateBehavior(kBehaviorIdle);
    }

    if (m_ppSlots[kSlotSwarf] != nullptr)
    {
        m_ppSlots[kSlotSwarf]->GetPhysicsBody()->pAttach = nullptr;
        m_ppSlots[kSlotSwarf]->GetPlanState()->ActivateBehavior(kBehaviorIdle);
    }

    GameEngine::ShowRokValue(kRokValuePlain);

    m_clock.periodStart = m_clock.tick;

    for (nIndex = 0; nIndex < m_nObjectCount; nIndex++)
    {
        if (m_ppObjects[nIndex]->m_nKind == ObjectKind::kDromer)
        {
            auto* const pStats  = &((PlayerObject*)m_ppObjects[nIndex])->m_stats;
            pStats->m_aStats[0] = FULL_WEIGHT;

            if (pStats->GetScalar(kRatingDamage) > g_flZeroGameEngine)
            {
                pStats->m_aStats[RATING_SLOT(kRatingDamage)] = FULL_WEIGHT;
            }
        }
    }

    for (nRow = 0; nRow < kTeamRowCount; nRow++)
    {
        if (m_pTeamA->aRows[nRow].aStat[kRatingDamage] > g_flZeroGameEngine)
        {
            m_pTeamA->aRows[nRow].aStat[kRatingDamage] = FULL_WEIGHT;
        }

        if (m_pTeamB->aRows[nRow].aStat[kRatingDamage] > g_flZeroGameEngine)
        {
            m_pTeamB->aRows[nRow].aStat[kRatingDamage] = FULL_WEIGHT;
        }

        m_pTeamA->aReserveSince[nRow] = {};
        m_pTeamB->aReserveSince[nRow] = {};
    }

    for (nTeam = 0; nTeam < 2; nTeam++)
    {
        if (nTeam == 0)
        {
            pTeam = m_pTeamA;
        }
        else
        {
            pTeam = m_pTeamB;
        }

        if (pTeam->aRows[0].aStat[kRatingDamage] > g_flZeroGameEngine && pTeam->aSeat[0] != 0 && pTeam->aSeat[1] != 0)
        {
            nSlot = nTeam;
            SubstitutePlayer(nSlot, 0);
        }
        else
        {
            nSlot   = nTeam;
            pObject = GetPlayer(static_cast<short>(nSlot));
            pObject->m_stats.CopyBaseStats();
            pObject->m_stats.ScaleTo(nSlot);
        }

        if (pTeam->aRows[1].aStat[kRatingDamage] > g_flZeroGameEngine && pTeam->aSeat[0] != 1 && pTeam->aSeat[1] != 1)
        {
            nSlot += 2;
            SubstitutePlayer(nSlot, 1);
        }
        else
        {
            nSlot += 2;
            pObject = GetPlayer(static_cast<short>(nSlot));
            pObject->m_stats.CopyBaseStats();
            pObject->m_stats.ScaleTo(nSlot);
        }
    }

    DeactivateParticleSystems();

    g_pMessageQueue->ProcessAndClear();

    if (m_clock.nPeriod == kFirstPeriod)
    {
        if (IsExhibitionMode() != 0)
        {
            g_pMessageQueue->PushFromTable(kTextExhibitionMode, kTextLineMain, kTextDuration);
        }
        else
        {
            g_pMessageQueue->PushFromTable(kTextGauntletMode, kTextLineMain, kTextDuration);
        }

        g_pMessageQueue->Push(m_pTeamA->GetName(), kTextLineNames, kTextDuration);
        auto* const pszVs = g_gameApp.m_stringPool.LoadText(kStringVs);
        g_pMessageQueue->Push(pszVs, kTextLineNames, kTextDuration);
        g_pMessageQueue->Push(m_pTeamB->GetName(), kTextLineNames, kTextDuration);

        g_pMessageQueue->PushFromTable(static_cast<int>(GetDifficultyLevel()) + kTextDifficultyBase,
                                       kTextLineMain,
                                       kTextDuration);
    }
}

#define MATCH_FRAME_RATE   18.0f
#define MATCH_RATE_SQUARED 324.0f
#define MATCH_STEP         (1.0f / 18.0f)

// Eighteen frames per second; the ring starts at nought and the pause mark at minus one.
// FUNCTION: HYPERX 0x00481d60
void GameEngine::InitMatchTimers()
{
    int nSlot;

    m_clock.flPrevInvDelta = MATCH_FRAME_RATE;
    m_clock.flInvDelta     = MATCH_FRAME_RATE;

    m_clock.tick     = {};
    m_clock.prevTick = {};

    m_clock.flPrevInvDeltaSquared = MATCH_RATE_SQUARED;
    m_clock.flInvDeltaSquared     = MATCH_RATE_SQUARED;

    const auto flStep = g_flOneGameEngine * MATCH_STEP;

    m_clock.flPrevDelta        = flStep;
    m_clock.flDelta            = flStep;
    m_clock.flPrevDeltaSquared = flStep * flStep;
    m_clock.flDeltaSquared     = flStep * flStep;

    for (nSlot = 0; nSlot < kClockRingLength; nSlot++)
    {
        m_clock.aflDeltaRing[nSlot] = g_flFifthGameEngine * m_clock.flDelta;
    }

    m_clock.nRingIndex = 0;
    m_clock.nPeriod    = 1;

    const auto now = Clock::Now();

    m_clock.periodStart = now;
    m_clock.pauseStart  = now;
    m_clock.nPauseMark  = -1;
}

char        g_bMultiplierColoursUnsaved = 1;
int         g_nMultiplierColourPositive = 144;
int         g_nMultiplierColourNegative = 160;
const char* g_pszOptionsSection         = "Options";
const char* g_pszAttackModeKey          = "AttackMode";
const char* g_pszTeammateModeKey        = "TeammateMode";

// The keyword table; the index order is the switch below.
// GLOBAL: HYPERX 0x004c706c
static const char* s_apszKeywords[] = { "Environment", "HomeTeam", "AwayTeam" };

constexpr auto kConfigKeywordCount = 3;

enum ConfigKeyword
{
    KEYWORD_ARENA = 0,
    KEYWORD_HOME  = 1,
    KEYWORD_AWAY  = 2
};

// FUNCTION: HYPERX 0x00481e10
char GameEngine::LoadConfigFile(const char* pszFileName)
{
    // STRING: HYPERX 0x004c41b4
    if (SDL_strcasecmp(pszFileName, "game.cfg") == 0)
    {
        LoadGameConfig(pszFileName);

        return 1;
    }

    LoadObjectsFromConfig(pszFileName);

    return 1;
}

// Reading game.cfg: the arena and the two teams; a known keyword with no value is fatal.
// FUNCTION: HYPERX 0x00481e50
char GameEngine::LoadGameConfig(const char* pszFileName)
{
    OwnedFile  file;
    PathString strPath(String::Concat(g_gameApp.m_strExeDir.m_str, pszFileName).m_pchData, kMaxPathLength);
    char       szToken[kParseTokenMax];
    char*      pLine;
    short      nKeyword;

    if (file.m_file.Open(strPath.m_str.m_pchData, FileMode::kRead) != 0)
    {
        while (file.m_file.GetNextLine(&pLine) > 0)
        {
            auto* pRest = pLine;

            if (pRest != nullptr
                // STRING: HYPERX 0x004c732c
                && ParseUtil::ParseToken(pRest, szToken, nullptr, " \t", kParseTokenMax, &pRest) != 0 && (nKeyword = ParseUtil::FindKeyword(szToken, s_apszKeywords, kConfigKeywordCount)) >= 0)
            {
                switch (nKeyword)
                {
                    case KEYWORD_ARENA:
                        pRest = ParseUtil::SkipPastChar(pRest, g_pszEqualsSign);

                        if (pRest == nullptr
                            // STRING: HYPERX 0x004c7324
                            || ParseUtil::ParseToken(pRest, szToken, nullptr, " \t\n\r", kParseTokenMax, &pRest) == 0)
                        {
                            g_gameApp.Error(
                                // STRING: HYPERX 0x004c7278
                                "Badly formed Game Config file Environment declaration %s!",
                                pRest);

                            return 0;
                        }

                        if (LoadArena(szToken) == 0)
                        {
                            return 0;
                        }
                        break;

                    case KEYWORD_HOME:
                        pRest = ParseUtil::SkipPastChar(pRest, g_pszEqualsSign);

                        if (pRest == nullptr || ParseUtil::ParseToken(pRest, szToken, nullptr, " \t\n\r", kParseTokenMax, &pRest) == 0)
                        {
                            g_gameApp.Error(
                                // STRING: HYPERX 0x004c72b4
                                "Badly formed Game Config file HomeTeam declaration %s!",
                                pRest);

                            return 0;
                        }

                        LoadHomeTeam(szToken);
                        break;

                    case KEYWORD_AWAY:
                        pRest = ParseUtil::SkipPastChar(pRest, g_pszEqualsSign);

                        if (pRest == nullptr || ParseUtil::ParseToken(pRest, szToken, nullptr, " \t\n\r", kParseTokenMax, &pRest) == 0)
                        {
                            g_gameApp.Error(
                                // STRING: HYPERX 0x004c72ec
                                "Badly formed Game Config file AwayTeam declaration %s!",
                                pRest);

                            return 0;
                        }

                        LoadAwayTeam(szToken);
                        break;
                }
            }
        }

        return 1;
    }

    g_gameApp.ReportLastError();
    g_gameApp.OnShutdown();

    return 0;
}

// GLOBAL: HYPERX 0x004c7078
static const char* s_apszCommands[] = {
    "DefineObject", "EndObjectDefinition", "SetMotionParams", "SetRatings", "SetRatings1", "SetRatings2", "SetRatings3", "SetGoalieRatings", "SetGoalieRatings1", "SetGoalieRatings2", "SetGoalieRatings3", "SetSpecialGoalieRatings", "SetCollisionObject",
    // STRING: HYPERX 0x004c7120
    "SetPlans"
};

constexpr auto kCommandCount = 14;

// The fourteen commands an object definition file is written in.
enum ObjectDefCommand
{
    CMD_DEFINE_OBJECT          = 0,
    CMD_END_DEFINITION         = 1,
    CMD_MOTION_PARAMS          = 2,
    CMD_RATINGS                = 3,
    CMD_RATINGS_1              = 4,
    CMD_RATINGS_2              = 5,
    CMD_RATINGS_3              = 6,
    CMD_GOALIE_RATINGS         = 7,
    CMD_GOALIE_RATINGS_1       = 8,
    CMD_GOALIE_RATINGS_2       = 9,
    CMD_GOALIE_RATINGS_3       = 10,
    CMD_GOALIE_RATINGS_SPECIAL = 11,
    CMD_COLLISION_OBJECT       = 12,
    CMD_PLANS                  = 13,
};

// Fourteen commands; the ratings commands write without checking a definition exists.
// FUNCTION: HYPERX 0x00482170
ObjectDef* GameEngine::ParseObjectDefinition(FileObj* pFile, unsigned int* pnModelId)
{
    char* pLine;
    char  szCommand[kParseTokenMax];
    short nCommand;

    ObjectDef* pDef      = nullptr;
    char       bComplete = 0;

    while (pFile->GetNextLine(&pLine) > 0)
    {
        // STRING: HYPERX 0x004c732c
        if (pLine != nullptr && ParseUtil::ParseToken(pLine, szCommand, nullptr, " \t", kParseTokenMax, &pLine) != 0 && (nCommand = ParseUtil::FindKeyword(szCommand, s_apszCommands, kCommandCount)) >= 0)
        {
            switch (nCommand)
            {
                case CMD_DEFINE_OBJECT:
                    pDef = Cmd_DefineObject(pLine, pnModelId);
                    break;

                case CMD_MOTION_PARAMS:
                    if (pDef != nullptr)
                    {
                        Cmd_SetMotionParams(pLine, pDef);
                    }
                    break;

                case CMD_RATINGS:
                case CMD_RATINGS_1:
                    Cmd_SetRatings(pLine, pDef, kRatingsField, 0);
                    break;

                case CMD_RATINGS_2:
                    Cmd_SetRatings(pLine, pDef, kRatingsField, 1);
                    break;

                case CMD_RATINGS_3:
                    Cmd_SetRatings(pLine, pDef, kRatingsField, 2);
                    break;

                case CMD_GOALIE_RATINGS:
                case CMD_GOALIE_RATINGS_1:
                    Cmd_SetRatings(pLine, pDef, kRatingsGoalie, 0);
                    break;

                case CMD_GOALIE_RATINGS_2:
                    Cmd_SetRatings(pLine, pDef, kRatingsGoalie, 1);
                    break;

                case CMD_GOALIE_RATINGS_3:
                    Cmd_SetRatings(pLine, pDef, kRatingsGoalie, 2);
                    break;

                case CMD_GOALIE_RATINGS_SPECIAL:
                    Cmd_SetRatings(pLine, pDef, kRatingsGoalie, kRatingsSpecialGoalie);
                    break;

                case CMD_COLLISION_OBJECT:
                    if (pDef != nullptr)
                    {
                        Cmd_SetCollisionObject(pLine, pDef, *pnModelId);
                    }
                    break;

                case CMD_PLANS:
                    if (pDef != nullptr)
                    {
                        Cmd_SetPlans(pLine, pDef);
                    }
                    break;

                case CMD_END_DEFINITION:
                    if (pDef != nullptr)
                    {
                        bComplete = 1;
                    }
                    break;
            }

            if (pDef == nullptr)
            {
                g_gameApp.Fatal(
                    // STRING: HYPERX 0x004c736c
                    "Game Config file error: missing DefineObject before other parameters %s!",
                    pLine);
            }

            if (bComplete != 0)
            {
                return pDef;
            }
        }
    }

    if (pDef != nullptr)
    {
        // STRING: HYPERX 0x004c7330
        g_gameApp.Fatal("Game Config file error: missing of EndOfDefinition for %s!",
                        pDef->m_pszName);
    }

    return nullptr;
}

// The scanf format the motion reader pushes.
// GLOBAL: HYPERX 0x004b3f14
extern const char* const g_pszPercentF;

// The one the ratings reader pushes, from the same run of format pointers.
// GLOBAL: HYPERX 0x004b3f0c
extern const char* g_pszPercentD;

// And the short one, which the collision reader's component index takes.
// GLOBAL: HYPERX 0x004b3f04
extern const char* const g_pszPercentHd;

constexpr auto kAttributeCount = 6;
constexpr auto kTypeCount      = 32;

enum ObjectAttribute
{
    ATTR_TYPE        = 1,
    ATTR_MODEL       = 2,
    ATTR_NAME        = 3,
    ATTR_MATERIAL    = 4,
    ATTR_OWN_PALETTE = 5
};

enum ObjectAttributeSeen
{
    SEEN_TYPE     = 1,
    SEEN_MODEL    = 2,
    SEEN_NAME     = 4,
    SEEN_REQUIRED = 7
};

#define NAME_SPACER '_'

// SGoal counts as NGoal and HeadCam as MainCamera; the last of the 32 type names never matches.
// FUNCTION: HYPERX 0x00482390
ObjectDef* GameEngine::Cmd_DefineObject(char* pszLine, unsigned int* pnModelId)
{
    const char* apszAttributes[kAttributeCount] = {
        // STRING: HYPERX 0x004c7650
        "None",
        // STRING: HYPERX 0x004c7648
        "type",
        // STRING: HYPERX 0x004c7640
        "modfile",
        // STRING: HYPERX 0x004c7638
        "name",
        // STRING: HYPERX 0x004c5538
        "mat",
        // STRING: HYPERX 0x004c7628
        "UseMyPallette"
    };

    char* pAt;
    char  szToken[kParseTokenMax];
    short nType;

    auto* const pDef = new ObjectDef(nullptr);

    auto nSeen = 0;

    const char* apszTypes[kTypeCount] = {
        // STRING: HYPERX 0x004c7620
        "Null",
        // STRING: HYPERX 0x004c7618
        "Dromer",
        // STRING: HYPERX 0x004c1610
        "Rok",
        // STRING: HYPERX 0x004c7610
        "Drome",
        // STRING: HYPERX 0x004c7608
        "Ramp",
        // STRING: HYPERX 0x004c7600
        "NGoal",
        // STRING: HYPERX 0x004c75f8
        "SGoal",
        // STRING: HYPERX 0x004c75ec
        "MainCamera",
        // STRING: HYPERX 0x004c75e4
        "HeadCam",
        // STRING: HYPERX 0x004c75d8
        "StartGun",
        // STRING: HYPERX 0x004c75cc
        "Turnstile",
        // STRING: HYPERX 0x004c75c0
        "Multiplier",
        // STRING: HYPERX 0x004c75b4
        "LaserHurdle",
        // STRING: HYPERX 0x004c75a4
        "GreenRandomizer",
        // STRING: HYPERX 0x004c7594
        "RedRandomizer",
        // STRING: HYPERX 0x004c7590
        "God",
        // STRING: HYPERX 0x004c7584
        "SoundMan",
        // STRING: HYPERX 0x004c7578
        "LimboPole",
        // STRING: HYPERX 0x004c7570
        "Hurdle",
        // STRING: HYPERX 0x004c7564
        "FastLane",
        // STRING: HYPERX 0x004c7558
        "Explosion",
        // STRING: HYPERX 0x004c7550
        "Armory",
        // STRING: HYPERX 0x004c7544
        "Keyboard",
        // STRING: HYPERX 0x004bc4ec
        "Gamepad",
        // STRING: HYPERX 0x004c7538
        "Joystick",
        // STRING: HYPERX 0x004c7530
        "Mouse",
        // STRING: HYPERX 0x004c7524
        "SlowLane",
        // STRING: HYPERX 0x004c15fc
        "Mine",
        // STRING: HYPERX 0x004c15f4
        "Swarf",
        // STRING: HYPERX 0x004c751c
        "Coach",
        // STRING: HYPERX 0x004c7510
        "BodyPart",
        // STRING: HYPERX 0x004c7508
        "Generic"
    };

    const ObjectKind anTypeIds[kTypeCount] = {
        ObjectKind::kNull, ObjectKind::kDromer, ObjectKind::kRok, ObjectKind::kDrome, ObjectKind::kRamp, ObjectKind::kGoal, ObjectKind::kGoal, ObjectKind::kCamera, ObjectKind::kCamera, ObjectKind::kStartGun, ObjectKind::kTurnstile, ObjectKind::kMultiplier, ObjectKind::kLaserHurdle, ObjectKind::kGreenRandomizer, ObjectKind::kRedRandomizer, ObjectKind::kGod, ObjectKind::kGeneric, ObjectKind::kLimboPole, ObjectKind::kHurdle, ObjectKind::kFastLane, ObjectKind::kExplosion, ObjectKind::kArmory, ObjectKind::kKeyboard, ObjectKind::kGamepad, ObjectKind::kJoystick, ObjectKind::kMouse, ObjectKind::kSlowLane, ObjectKind::kMine, ObjectKind::kSwarf, ObjectKind::kCoach, ObjectKind::kBodyPart, ObjectKind::kGeneric
    };

    auto* pRest = pszLine;

    memset((void*)pDef, 0, sizeof(ObjectDef));

    while (pRest != nullptr)
    {
        // STRING: HYPERX 0x004c7504
        if (ParseUtil::ParseToken(pRest, szToken, nullptr, " =\t", kParseTokenMax, &pRest) == 0)
        {
            break;
        }

        const auto nAttribute = ParseUtil::FindKeyword(szToken, apszAttributes, kAttributeCount);

        if (nAttribute <= 0)
        {
            break;
        }

        switch (nAttribute)
        {
            case ATTR_TYPE:
                pRest = ParseUtil::SkipPastChar(pRest, g_pszEqualsSign);

                if (pRest == nullptr
                    // STRING: HYPERX 0x004c7324
                    || ParseUtil::ParseToken(pRest, szToken, nullptr, " \t\n\r", kParseTokenMax, &pRest) == 0 || (nType = ParseUtil::FindKeyword(szToken, apszTypes, kGOBNumOfTypes)) < 0)
                {
                    g_gameApp.Fatal(
                        // STRING: HYPERX 0x004c7464
                        "Game Config file error: bad type keyword %s!",
                        szToken);
                    break;
                }

                pDef->m_nTypeId = anTypeIds[nType];

                if (nType == kGOBDromer)
                {
                    if (pnModelId == nullptr)
                    {
                        g_gameApp.Fatal(
                            // STRING: HYPERX 0x004c74d0
                            "ERROR: ParseModelObjDef: dromer model ID is NULL!");
                    }

                    pDef->m_nType = kGOBDromer;
                }
                else
                {
                    pDef->m_nType = nType;

                    if (pDef->m_nType == kGOBBodyPart)
                    {
                        if (static_cast<int>(*pnModelId) < kSlotBodyPart || static_cast<int>(*pnModelId) > kSlotBodyPartLast)
                        {
                            // STRING: HYPERX 0x004c74b0
                            g_gameApp.Fatal("Invalid model ID for bodypart\n");
                        }
                    }
                    else if (m_abTypeHighWater[nType] > g_abObjectTypeLimit[nType])
                    {
                        if (nType != kGOBRok || g_bSuppressRokId == 0)
                        {
                            g_gameApp.Fatal(
                                // STRING: HYPERX 0x004c7494
                                "Error: Too many %s objects!",
                                apszTypes[nType]);
                        }
                    }
                    else if (nType != kGOBRok || g_bSuppressRokId == 0)
                    {
                        *pnModelId = m_abTypeHighWater[nType];

                        m_abTypeHighWater[nType]++;
                    }
                }

                nSeen |= SEEN_TYPE;
                break;

            case ATTR_MODEL:
                pRest = ParseUtil::SkipPastChar(pRest, g_pszEqualsSign);

                if (pRest == nullptr || ParseUtil::ParseToken(pRest, szToken, nullptr, " \t\n\r", kParseTokenMax, &pRest) == 0)
                {
                    g_gameApp.Fatal(
                        // STRING: HYPERX 0x004c7438
                        "Game Config file error: Bad model name: %s!",
                        szToken);
                    break;
                }

                pDef->m_pszModel = strcpy(new char[strlen(szToken) + 1], szToken);

                nSeen |= SEEN_MODEL;
                break;

            case ATTR_NAME:
                pRest = ParseUtil::SkipPastChar(pRest, g_pszEqualsSign);

                if (pRest == nullptr || ParseUtil::ParseToken(pRest, szToken, nullptr, " \t\n\r", kParseTokenMax, &pRest) == 0)
                {
                    g_gameApp.Fatal(
                        // STRING: HYPERX 0x004c7408
                        "Game Config file error: Bad object name: %s!",
                        szToken);
                    break;
                }

                pAt = szToken;

                while (*pAt != 0)
                {
                    if (*pAt == NAME_SPACER)
                    {
                        *pAt = ' ';
                    }

                    pAt++;
                }

                pDef->m_pszName = strcpy(new char[strlen(szToken) + 1], szToken);

                nSeen |= SEEN_NAME;
                break;

            case ATTR_MATERIAL:
                pRest = ParseUtil::SkipPastChar(pRest, g_pszEqualsSign);

                if (pRest == nullptr || ParseUtil::ParseToken(pRest, szToken, nullptr, " \t\n\r", kParseTokenMax, &pRest) == 0)
                {
                    g_gameApp.Fatal(
                        // STRING: HYPERX 0x004c73d8
                        "Game Config file error: Bad matXref name: %s!",
                        szToken);
                    break;
                }

                pDef->m_pszMaterial = strcpy(new char[strlen(szToken) + 1], szToken);
                break;

            case ATTR_OWN_PALETTE:
                pDef->m_bOwnPalette = 1;
                break;
        }
    }

    if (static_cast<unsigned char>(nSeen & SEEN_REQUIRED) != SEEN_REQUIRED)
    {
        g_gameApp.Fatal(
            // STRING: HYPERX 0x004c73b8
            "Incomplete Game Object def: %s!",
            pszLine);
    }

    pDef->m_nRatingsMode = 0;

    return pDef;
}

constexpr auto kMotionKeywordCount = 9;

// The seven motion keys a definition's parameter block names.
enum ObjectDefKey
{
    KEY_POS   = 2,
    KEY_ACCEL = 3,
    KEY_SPEED = 4,
    KEY_AZIM  = 5,
    KEY_ELEV  = 6,
    KEY_FRIC  = 7,
    KEY_ELAS  = 8,
};

// Neither complaint names the failing field, and neither stops the walk.
// FUNCTION: HYPERX 0x00482b00
char GameEngine::Cmd_SetMotionParams(char* pszLine, ObjectDef* pDef)
{
    const char* apszKeywords[kMotionKeywordCount] = {
        "None",
        "name",
        // STRING: HYPERX 0x004c76a8
        "pos",
        // STRING: HYPERX 0x004c76a0
        "accel",
        // STRING: HYPERX 0x004c0b2c
        "speed",
        // STRING: HYPERX 0x004c7698
        "azim",
        // STRING: HYPERX 0x004c7690
        "elev",
        // STRING: HYPERX 0x004c7688
        "fric",
        // STRING: HYPERX 0x004c7680
        "elas"
    };

    char szToken[kParseTokenMax];

    auto* pszCursor = pszLine;

    while (pszCursor != nullptr)
    {
        if (ParseUtil::ParseToken(pszCursor, szToken, nullptr, " =\t", kParseTokenMax, &pszCursor) == 0)
        {
            break;
        }

        pszCursor = ParseUtil::SkipToChar(pszCursor, g_pszEqualsSign);

        if (pszCursor == nullptr)
        {
            break;
        }

        const auto nKeyword = ParseUtil::FindKeyword(szToken, apszKeywords, kMotionKeywordCount);

        if (nKeyword <= 0)
        {
            break;
        }

        switch (nKeyword)
        {
            case KEY_POS:
                pszCursor++;

                if (
                    ParseUtil::ParseToken(pszCursor, szToken, nullptr, " \t\n\r", kParseTokenMax, &pszCursor) == 0 || sscanf(szToken, g_pszPercentF, &pDef->m_vPos.x) != 1 || ParseUtil::ParseToken(pszCursor, szToken, nullptr, " \t\n\r", kParseTokenMax, &pszCursor) == 0 || sscanf(szToken, g_pszPercentF, &pDef->m_vPos.y) != 1 || ParseUtil::ParseToken(pszCursor, szToken, nullptr, " \t\n\r", kParseTokenMax, &pszCursor) == 0 || sscanf(szToken, g_pszPercentF, &pDef->m_vPos.z) != 1)
                {
                    g_gameApp.Fatal("Game Config file error: bad type keyword %s!", szToken);
                }

                break;

            case KEY_ACCEL:
                pszCursor++;

                if (ParseUtil::ParseToken(pszCursor, szToken, nullptr, " \t\n\r", kParseTokenMax, &pszCursor) == 0 || sscanf(szToken, g_pszPercentF, &pDef->m_flAccel) != 1)
                {
                    g_gameApp.Fatal(
                        // STRING: HYPERX 0x004c7658
                        "Game Config file error: accel param %s!",
                        szToken);
                }

                break;

            case KEY_SPEED:
                pszCursor++;

                if (ParseUtil::ParseToken(pszCursor, szToken, nullptr, " \t\n\r", kParseTokenMax, &pszCursor) == 0 || sscanf(szToken, g_pszPercentF, &pDef->m_flSpeed) != 1)
                {
                    g_gameApp.Fatal("Game Config file error: accel param %s!", szToken);
                }

                break;

            case KEY_AZIM:
                pszCursor++;

                if (ParseUtil::ParseToken(pszCursor, szToken, nullptr, " \t\n\r", kParseTokenMax, &pszCursor) == 0 || sscanf(szToken, g_pszPercentF, &pDef->m_flAzimuth) != 1)
                {
                    g_gameApp.Fatal("Game Config file error: accel param %s!", szToken);
                }

                break;

            case KEY_ELEV:
                pszCursor++;

                if (ParseUtil::ParseToken(pszCursor, szToken, nullptr, " \t\n\r", kParseTokenMax, &pszCursor) == 0 || sscanf(szToken, g_pszPercentF, &pDef->m_flElevation) != 1)
                {
                    g_gameApp.Fatal("Game Config file error: accel param %s!", szToken);
                }

                break;

            case KEY_FRIC:
                pszCursor++;

                if (ParseUtil::ParseToken(pszCursor, szToken, nullptr, " \t\n\r", kParseTokenMax, &pszCursor) == 0 || sscanf(szToken, g_pszPercentF, &pDef->m_flFriction) != 1)
                {
                    g_gameApp.Fatal("Game Config file error: accel param %s!", szToken);
                }

                break;

            case KEY_ELAS:
                pszCursor++;

                if (ParseUtil::ParseToken(pszCursor, szToken, nullptr, " \t\n\r", kParseTokenMax, &pszCursor) == 0 || sscanf(szToken, g_pszPercentF, &pDef->m_flElasticity) != 1)
                {
                    g_gameApp.Fatal("Game Config file error: accel param %s!", szToken);
                }

                break;
        }
    }

    return 1;
}

constexpr auto kAttrCount   = 6;
constexpr auto kBubbleCount = 0x1b;

enum ComponentAttribute
{
    ATTR_TYPE_KEY   = 0,
    ATTR_COMPONENT  = 1,
    ATTR_RADIUS     = 2,
    ATTR_CENTROID   = 3,
    ATTR_ATTACH     = 4,
    ATTR_USE_DETAIL = 5
};

// The collision bubble kinds, in the order of their keywords.
constexpr auto kBubbleDromerAwareness = 0x00;
constexpr auto kBubbleDromerModel     = 0x01;
constexpr auto kBubbleDromerBody      = 0x02;
constexpr auto kBubbleDromerScoop     = 0x03;
constexpr auto kBubbleRamp            = 0x04;
constexpr auto kBubbleRok             = 0x05;
constexpr auto kBubbleGoalSide        = 0x06;
constexpr auto kBubbleGoalZone        = 0x07;
constexpr auto kBubbleGoalCase        = 0x08;
constexpr auto kBubbleGoalPort        = 0x09;
constexpr auto kBubbleCamera          = 0x0a;
constexpr auto kBubbleHeadCam         = 0x0b;
constexpr auto kBubbleMultiplier      = 0x0c;
constexpr auto kBubbleTurnstile       = 0x0d;
constexpr auto kBubbleStartgun        = 0x0e;
constexpr auto kBubbleLaserHurdle     = 0x0f;
constexpr auto kBubbleArmory          = 0x10;
constexpr auto kBubbleLimboPole       = 0x11;
constexpr auto kBubbleFastLane        = 0x12;
constexpr auto kBubbleExplosion       = 0x13;
constexpr auto kBubbleHurdle          = 0x14;
constexpr auto kBubbleRandomizer      = 0x15;
constexpr auto kBubbleKludge          = 0x16;
constexpr auto kBubbleSlowLane        = 0x17;
constexpr auto kBubbleMine            = 0x18;
constexpr auto kBubbleSwarf           = 0x19;
constexpr auto kBubbleBodyPart        = 0x1a;

#define COLLISION_MISSING   -9999999.0f
#define COLLISION_NO_RADIUS -1.0f

inline int GameEngine::FindTeamDefinition(FileObj* pFile, char** ppLine, char* pszToken)
{
    do
    {
        // STRING: HYPERX 0x004c732c
        if (ParseUtil::ParseToken(*ppLine, pszToken, nullptr, " \t", kParseTokenMax, ppLine) != 0 && SDL_strcasecmp(pszToken, "DefineTeam") == 0)
        {
            return 1;
        }
    } while (pFile->GetNextLine(ppLine) > 0);

    return 0;
}

// The environment and dromext errors each name the other's field.
// FUNCTION: HYPERX 0x00482e70
TeamBlock* GameEngine::ParseTeamHTMFile(FileObj* pFile)
{
    const char* apszKeyword[kTeamKeywordCount] = {
        // STRING: HYPERX 0x004c7638
        "name",
        // STRING: HYPERX 0x004c7860
        "idlogo",
        // STRING: HYPERX 0x004c7858
        "tslogo",
        // STRING: HYPERX 0x004c7850
        "tstext",
        // STRING: HYPERX 0x004c7848
        "tstitle",
        // STRING: HYPERX 0x004c7840
        "tsanim",
        // STRING: HYPERX 0x004c7834
        "environment",
        // STRING: HYPERX 0x004c7828
        "DefineTeam",
        // STRING: HYPERX 0x004c7820
        "dromext"
    };

    char       szToken[kParseTokenMax];
    TeamBlock* pTeam;
    char*      pLine;
    char*      pAt;
    short      nKeyword;

    pTeam = new TeamBlock;

    if (pFile->GetNextLine(&pLine) <= 0)
    {
        goto quit;
    }

    if (FindTeamDefinition(pFile, &pLine, szToken) == 0)
    {
        return nullptr;
    }

    memset((void*)pTeam, 0, sizeof(TeamBlock));

    while (pLine != nullptr)
    {
        // STRING: HYPERX 0x004c7504
        if (ParseUtil::ParseToken(pLine, szToken, nullptr, " =\t", kParseTokenMax, &pLine) == 0)
        {
            break;
        }

        pLine = ParseUtil::SkipToChar(pLine, g_pszEqualsSign);

        if (pLine == nullptr)
        {
            break;
        }

        nKeyword = ParseUtil::FindKeyword(szToken, apszKeyword, kTeamKeywordCount);

        if (nKeyword < 0)
        {
            break;
        }

        switch (nKeyword)
        {
            case 0:
                pLine++;

                if (ParseUtil::ParseToken(pLine, szToken, nullptr, " \t\n\r", kParseTokenMax, &pLine) == 0)
                {
                    // STRING: HYPERX 0x004c7800
                    g_gameApp.Error("Team asgn error: Team name: %s!", szToken);

                    goto failed;
                }

                pAt = szToken;

                while (*pAt != 0)
                {
                    if (*pAt == '_')
                    {
                        *pAt = ' ';
                    }

                    pAt++;
                }

                strcpy(pTeam->szName, szToken);

                break;

            case 1:
                pLine++;

                if (ParseUtil::ParseToken(pLine, szToken, nullptr, " \t\n\r", kParseTokenMax, &pLine) == 0)
                {
                    // STRING: HYPERX 0x004c77d8
                    g_gameApp.Error("Team asgn error: Team LogoBMPname: %s!", szToken);

                    goto failed;
                }

                strcpy(pTeam->szIdLogo, szToken);

                break;

            case 2:
                pLine++;

                if (ParseUtil::ParseToken(pLine, szToken, nullptr, " \t\n\r", kParseTokenMax, &pLine) == 0)
                {
                    // STRING: HYPERX 0x004c77b0
                    g_gameApp.Error("Team asgn error: Team select logo: %s!", szToken);

                    goto failed;
                }

                strcpy(pTeam->szTsLogo, szToken);

                break;

            case 3:
                pLine++;

                if (ParseUtil::ParseToken(pLine, szToken, nullptr, " \t\n\r", kParseTokenMax, &pLine) == 0)
                {
                    // STRING: HYPERX 0x004c7788
                    g_gameApp.Error("Team asgn error: Team select text: %s!", szToken);

                    goto failed;
                }

                strcpy(pTeam->szTsText, szToken);

                break;

            case 4:
                pLine++;

                if (ParseUtil::ParseToken(pLine, szToken, nullptr, " \t\n\r", kParseTokenMax, &pLine) == 0)
                {
                    // STRING: HYPERX 0x004c7760
                    g_gameApp.Error("Team asgn error: Team select title: %s!", szToken);

                    goto failed;
                }

                strcpy(pTeam->szTsTitle, szToken);

                break;

            case 5:
                pLine++;

                if (ParseUtil::ParseToken(pLine, szToken, nullptr, " \t\n\r", kParseTokenMax, &pLine) == 0)
                {
                    // STRING: HYPERX 0x004c772c
                    g_gameApp.Error("Team asgn error: Team select animation name: %s!", szToken);

                    goto failed;
                }

                strcpy(pTeam->szTsAnim, szToken);

                break;

            case 6:
                pLine++;

                if (ParseUtil::ParseToken(pLine, szToken, nullptr, " \t\n\r", kParseTokenMax, &pLine) == 0)
                {
                    // STRING: HYPERX 0x004c76f4
                    g_gameApp.Error("Team asgn error: Team select Drome exterior name: %s!", szToken);

                    goto failed;
                }

                strcpy(pTeam->szEnvironment, szToken);

                break;

            case 8:
                pLine++;

                if (ParseUtil::ParseToken(pLine, szToken, nullptr, " \t\n\r", kParseTokenMax, &pLine) == 0)
                {
                    // STRING: HYPERX 0x004c76c8
                    g_gameApp.Error("Team asgn error: Team environment name: %s!", szToken);

                    goto failed;
                }

                strcpy(pTeam->szDromeExt, szToken);

                break;
        }
    }

    if (pTeam->szName[0] == 0)
    {
        // STRING: HYPERX 0x004c76ac
        g_gameApp.Error("Incomplete Team def:\n%s!", pLine);

        return nullptr;
    }

    return pTeam;

failed:
    delete pTeam;

quit:
    return nullptr;
}

// A RandomizerBubble on a RedRandomizer becomes the next kind ("MAJOR KLUDGE HERE").

// FUNCTION: HYPERX 0x00483300
char GameEngine::Cmd_SetCollisionObject(char* pszLine, ObjectDef* pDef, unsigned int nModelId)
{
    const char* apszAttributes[kAttrCount] = {
        "type",
        // STRING: HYPERX 0x004c7bcc
        "component",
        // STRING: HYPERX 0x004c7bc4
        "radius",
        // STRING: HYPERX 0x004c7bb8
        "centroid",
        // STRING: HYPERX 0x004c7bac
        "attachPoint",
        // STRING: HYPERX 0x004c7ba0
        "useDetail"
    };

    const char* apszBubbles[kBubbleCount] = {
        // STRING: HYPERX 0x004c7b88
        "DromerAwarenessBubble",
        // STRING: HYPERX 0x004c7b74
        "DromerModelBubble",
        // STRING: HYPERX 0x004c7b60
        "DromerBodyBubble",
        // STRING: HYPERX 0x004c7b4c
        "DromerScoopBubble",
        // STRING: HYPERX 0x004c7b40
        "RampBubble",
        // STRING: HYPERX 0x004c7b34
        "RokBubble",
        // STRING: HYPERX 0x004c7b24
        "GoalSideBubble",
        // STRING: HYPERX 0x004c7b14
        "GoalZoneBubble",
        // STRING: HYPERX 0x004c7b04
        "GoalCaseBubble",
        // STRING: HYPERX 0x004c7af4
        "GoalPortBubble",
        // STRING: HYPERX 0x004c7ae4
        "CameraBubble",
        // STRING: HYPERX 0x004c7ad4
        "HeadCamBubble",
        // STRING: HYPERX 0x004c7ac0
        "MultiplierBubble",
        // STRING: HYPERX 0x004c7ab0
        "TurnstileBubble",
        // STRING: HYPERX 0x004c7aa0
        "StartgunBubble",
        // STRING: HYPERX 0x004c7a8c
        "LaserHurdleBubble",
        // STRING: HYPERX 0x004c7a7c
        "ArmoryBubble",
        // STRING: HYPERX 0x004c7a6c
        "LimboPoleBubble",
        // STRING: HYPERX 0x004c7a5c
        "FastLaneBubble",
        // STRING: HYPERX 0x004c7a4c
        "ExplosionBubble",
        // STRING: HYPERX 0x004c7a3c
        "HurdleBubble",
        // STRING: HYPERX 0x004c7a28
        "RandomizerBubble",
        // STRING: HYPERX 0x004c7a14
        "MAJOR KLUDGE HERE",
        // STRING: HYPERX 0x004c7a04
        "SlowLaneBubble",
        // STRING: HYPERX 0x004c79f8
        "MineBubble",
        // STRING: HYPERX 0x004c79ec
        "SwarfBubble",
        // STRING: HYPERX 0x004c79dc
        "BodyPartBubble"
    };

    BubbleShape anBubbleShapes[kBubbleCount] = {
        BubbleShape::kSphere, BubbleShape::kSphere, BubbleShape::kSphere, BubbleShape::kAttached, BubbleShape::kCaged, BubbleShape::kSphere, BubbleShape::kSphere, BubbleShape::kSphere, BubbleShape::kSphere, BubbleShape::kSphere, BubbleShape::kSphere, BubbleShape::kSphere, BubbleShape::kSphere, BubbleShape::kSphere, BubbleShape::kSphere, BubbleShape::kSphere, BubbleShape::kSphere, BubbleShape::kSphere, BubbleShape::kSphere, BubbleShape::kSphere, BubbleShape::kSphere, BubbleShape::kSphere, BubbleShape::kSphere, BubbleShape::kSphere, BubbleShape::kSphere, BubbleShape::kSphere, BubbleShape::kSphere
    };

    char szToken[kParseTokenMax];
    Vec3 vCentroid;
    Vec3 vAttach;

    short nBubbleType = -1;
    short nComponent  = -1;

    auto flRadius = COLLISION_NO_RADIUS;
    vCentroid.x   = COLLISION_MISSING;
    vAttach.x     = COLLISION_MISSING;

    char bUseDetail = 0;

    auto* pszCursor = pszLine;

    while (pszCursor != nullptr)
    {
        if (ParseUtil::ParseToken(pszCursor, szToken, nullptr, " =\t", kParseTokenMax, &pszCursor) == 0)
        {
            break;
        }

        const auto nAttribute = ParseUtil::FindKeyword(szToken, apszAttributes, kAttrCount);

        if (nAttribute < 0)
        {
            break;
        }

        switch (nAttribute)
        {
            case ATTR_TYPE_KEY:
                pszCursor = ParseUtil::SkipPastChar(pszCursor, g_pszEqualsSign);

                if (pszCursor != nullptr && ParseUtil::ParseToken(pszCursor, szToken, nullptr, " \t\n\r", kParseTokenMax, &pszCursor) != 0)
                {
                    nBubbleType = ParseUtil::FindKeyword(szToken, apszBubbles, kBubbleCount);

                    if (nBubbleType >= 0)
                    {
                        if (nBubbleType == kBubbleRandomizer && pDef->m_nType == kGOBRedRandomizer)
                        {
                            nBubbleType = kBubbleKludge;
                        }

                        break;
                    }
                }

                g_gameApp.Fatal(
                    // STRING: HYPERX 0x004c799c
                    "Game Config file error: bad collision object type keyword %s!",
                    szToken);
                break;

            case ATTR_COMPONENT:
                pszCursor = ParseUtil::SkipPastChar(pszCursor, g_pszEqualsSign);

                if (pszCursor == nullptr || ParseUtil::ParseToken(pszCursor, szToken, nullptr, " \t\n\r", kParseTokenMax, &pszCursor) == 0 || sscanf(szToken, g_pszPercentHd, &nComponent) != 1)
                {
                    g_gameApp.Fatal(
                        // STRING: HYPERX 0x004c796c
                        "Game Config file error: collision radius %s!",
                        pszLine);
                }

                break;

            case ATTR_RADIUS:
                pszCursor = ParseUtil::SkipPastChar(pszCursor, g_pszEqualsSign);

                if (pszCursor == nullptr || ParseUtil::ParseToken(pszCursor, szToken, nullptr, " \t\n\r", kParseTokenMax, &pszCursor) == 0 || sscanf(szToken, g_pszPercentF, &flRadius) != 1)
                {
                    g_gameApp.Fatal("Game Config file error: collision radius %s!", pszLine);
                }

                break;

            case ATTR_CENTROID:
                pszCursor = ParseUtil::SkipPastChar(pszCursor, g_pszEqualsSign);

                if (pszCursor == nullptr || ParseUtil::ParseToken(pszCursor, szToken, nullptr, " \t\n\r", kParseTokenMax, &pszCursor) == 0 || sscanf(szToken, g_pszPercentF, &vCentroid.x) != 1 || ParseUtil::ParseToken(pszCursor, szToken, nullptr, " \t\n\r", kParseTokenMax, &pszCursor) == 0 || sscanf(szToken, g_pszPercentF, &vCentroid.y) != 1 || ParseUtil::ParseToken(pszCursor, szToken, nullptr, " \t\n\r", kParseTokenMax, &pszCursor) == 0 || sscanf(szToken, g_pszPercentF, &vCentroid.z) != 1)
                {
                    g_gameApp.Fatal(
                        // STRING: HYPERX 0x004c793c
                        "Game Config file error: collision centroid %s!",
                        szToken);
                }

                break;

            case ATTR_ATTACH:
                pszCursor = ParseUtil::SkipPastChar(pszCursor, g_pszEqualsSign);

                if (pszCursor == nullptr || ParseUtil::ParseToken(pszCursor, szToken, nullptr, " \t\n\r", kParseTokenMax, &pszCursor) == 0 || sscanf(szToken, g_pszPercentF, &vAttach.x) != 1 || ParseUtil::ParseToken(pszCursor, szToken, nullptr, " \t\n\r", kParseTokenMax, &pszCursor) == 0 || sscanf(szToken, g_pszPercentF, &vAttach.y) != 1 || ParseUtil::ParseToken(pszCursor, szToken, nullptr, " \t\n\r", kParseTokenMax, &pszCursor) == 0 || sscanf(szToken, g_pszPercentF, &vAttach.z) != 1)
                {
                    g_gameApp.Fatal(
                        // STRING: HYPERX 0x004c7908
                        "Game Config file error: collision attachPoint %s!",
                        pszLine);
                }

                break;

            case ATTR_USE_DETAIL:
                bUseDetail = 1;
                break;
        }
    }

    if (nBubbleType == -1 || nBubbleType >= kBubbleCount)
    {
        g_gameApp.Fatal(
            // STRING: HYPERX 0x004c78d0
            "Game Config error: collision object type missing: %s!",
            pszLine);
    }

    if (flRadius == COLLISION_MISSING)
    {
        g_gameApp.Fatal(
            // STRING: HYPERX 0x004c789c
            "Game Config error: collision radius missing: %s!",
            pszLine);
    }

    if (vCentroid.x == COLLISION_MISSING)
    {
        g_gameApp.Fatal(
            // STRING: HYPERX 0x004c7868
            "Game Config error: collision centroid missing: %s!",
            pszLine);
    }

    BubbleCmd** ppSlot = &pDef->m_pCollisionList;

    // The new command goes on the end, as the old tail's one child.
    BubbleCmd* pTail = nullptr;

    for (BubbleCmd* pNode = *ppSlot; pNode != nullptr; pNode = *ppSlot)
    {
        ppSlot = &pNode->m_pChildren;
        pTail  = pNode;
    }

    if (pTail != nullptr)
    {
        pTail->m_nChildCount++;
    }

    auto* const pNew = new BubbleCmd;

    *ppSlot = pNew;

    memset((void*)pNew, 0, sizeof(BubbleCmd));

    pNew->m_wShape    = anBubbleShapes[nBubbleType];
    pNew->m_nSlot     = nBubbleType;
    pNew->m_pChildren = nullptr;

    if (vAttach.x != COLLISION_MISSING)
    {
        auto* const pExtra = new AttachPak;

        memset(pExtra, 0, sizeof(AttachPak));

        pExtra->nLinkId = nComponent;

        pExtra->vOffset = vAttach;

        pNew->m_pAttach = pExtra;
    }
    else if (bUseDetail != 0)
    {
        pNew->m_pAttach = (AttachPak*)static_cast<std::intptr_t>(kExtraUseDetail);
    }

    MapModifierToId(static_cast<short>(nModelId), pNew);

    auto* const pPos = new BubbleShapeNode;

    pNew->m_pPos = pPos;

    pPos->m_nComponent = nComponent;

    pNew->m_pPos->m_flRadius = flRadius;

    pNew->m_pPos->m_vCentre = vCentroid;

    return 1;
}

constexpr auto kRatingsPlayerCount = 0x40;
constexpr auto kRatingsGoalieCount = 0x11;

#define GOALIE_RATING_DEFAULT 5.0f

// Values are whole; EndRatings or anything past the end stores one past the array.
// FUNCTION: HYPERX 0x00483b50
int GameEngine::Cmd_SetRatings(char* pszLine, ObjectDef* pDef, int nMode, int nLevel)
{
    const char* apszPlayer[kRatingsPlayerCount] = {
        // STRING: HYPERX 0x004c8244
        "id_RAT_WeightClass",
        // STRING: HYPERX 0x004c8234
        "id_RAT_Stamina",
        // STRING: HYPERX 0x004c8224
        "id_RAT_Checking",
        // STRING: HYPERX 0x004c8214
        "id_RAT_Balance",
        // STRING: HYPERX 0x004c8204
        "id_RAT_Speed",
        // STRING: HYPERX 0x004c81f0
        "id_RAT_ShotPower",
        // STRING: HYPERX 0x004c81dc
        "id_RAT_ShotAccuracy",
        // STRING: HYPERX 0x004c81c8
        "id_RAT_PassAccuracy",
        // STRING: HYPERX 0x004c81b0
        "id_RAT_OffensiveAbility",
        // STRING: HYPERX 0x004c8198
        "id_RAT_LongShotEnable",
        // STRING: HYPERX 0x004c8188
        "id_RAT_Immunity",
        // STRING: HYPERX 0x004c8170
        "id_RAT_LongPassEnable",
        // STRING: HYPERX 0x004c8158
        "id_RAT_MultiPointSpread",
        // STRING: HYPERX 0x004c8144
        "id_RAT_RokHandling",
        // STRING: HYPERX 0x004c8130
        "id_RAT_ArmoryRange",
        // STRING: HYPERX 0x004c8120
        "id_RAT_Damage",
        // STRING: HYPERX 0x004c8110
        "id_RAT_Gender",
        // STRING: HYPERX 0x004c80fc
        "id_RAT_Speedburst0a",
        // STRING: HYPERX 0x004c80e8
        "id_RAT_Speedburst0b",
        // STRING: HYPERX 0x004c80d4
        "id_RAT_Speedburst1a",
        // STRING: HYPERX 0x004c80c0
        "id_RAT_Speedburst1b",
        // STRING: HYPERX 0x004c80ac
        "id_RAT_Speedburst2a",
        // STRING: HYPERX 0x004c8098
        "id_RAT_Speedburst2b",
        // STRING: HYPERX 0x004c8084
        "id_RAT_Speedburst3a",
        // STRING: HYPERX 0x004c8070
        "id_RAT_Speedburst3b",
        // STRING: HYPERX 0x004c805c
        "id_RAT_Speedburst4a",
        // STRING: HYPERX 0x004c8048
        "id_RAT_Speedburst4b",
        // STRING: HYPERX 0x004c8038
        "id_RAT_Bungee0",
        // STRING: HYPERX 0x004c8028
        "id_RAT_Bungee1",
        // STRING: HYPERX 0x004c8018
        "id_RAT_Bungee2",
        // STRING: HYPERX 0x004c8008
        "id_RAT_Pass0",
        // STRING: HYPERX 0x004c7ff8
        "id_RAT_Pass1",
        // STRING: HYPERX 0x004c7fe8
        "id_RAT_Pass2",
        // STRING: HYPERX 0x004c7fd0
        "id_RAT_WinCloseFreeRok",
        // STRING: HYPERX 0x004c7fb8
        "id_RAT_LoseCloseFreeRok",
        // STRING: HYPERX 0x004c7fa0
        "id_RAT_WinFarFreeRok",
        // STRING: HYPERX 0x004c7f88
        "id_RAT_LoseFarFreeRok",
        // STRING: HYPERX 0x004c7f70
        "id_RAT_WinOffWingJob",
        // STRING: HYPERX 0x004c7f58
        "id_RAT_LoseOffWingJob",
        // STRING: HYPERX 0x004c7f44
        "id_RAT_Multiplier0",
        // STRING: HYPERX 0x004c7f30
        "id_RAT_Multiplier1",
        // STRING: HYPERX 0x004c7f1c
        "id_RAT_Turnstile0",
        // STRING: HYPERX 0x004c7f08
        "id_RAT_Turnstile1",
        // STRING: HYPERX 0x004c7ef4
        "id_RAT_Killball0",
        // STRING: HYPERX 0x004c7ee0
        "id_RAT_Killball1",
        // STRING: HYPERX 0x004c7ecc
        "id_RAT_Randomize0",
        // STRING: HYPERX 0x004c7eb8
        "id_RAT_Randomize1",
        // STRING: HYPERX 0x004c7ea4
        "id_RAT_Fighting0",
        // STRING: HYPERX 0x004c7e90
        "id_RAT_Fighting1",
        // STRING: HYPERX 0x004c7e7c
        "id_RAT_Fighting2",
        // STRING: HYPERX 0x004c7e68
        "id_RAT_Fighting3",
        // STRING: HYPERX 0x004c7e54
        "id_RAT_Fighting4",
        // STRING: HYPERX 0x004c7e3c
        "id_RAT_FlyingRepetoire",
        // STRING: HYPERX 0x004c7e20
        "id_RAT_HalfpipeRepetoire",
        // STRING: HYPERX 0x004c7e04
        "id_RAT_CarryRokThrowDist",
        // STRING: HYPERX 0x004c7de4
        "id_RAT_WinCarryRokRegroupDist",
        // STRING: HYPERX 0x004c7dc4
        "id_RAT_LoseCarryRokRegroupDist",
        // STRING: HYPERX 0x004c7da4
        "id_RAT_WinAggressionOnOthers",
        // STRING: HYPERX 0x004c7d84
        "id_RAT_LoseAggressionOnOthers",
        // STRING: HYPERX 0x004c7d70
        "id_RAT_SubOutDamage",
        // STRING: HYPERX 0x004c7d5c
        "id_RAT_SubInDamage",
        // STRING: HYPERX 0x004c7d48
        "id_RAT_MultiRange",
        // STRING: HYPERX 0x004c7d38
        "id_RAT_MinSubIn",
        // STRING: HYPERX 0x004c8258
        "EndRatings"
    };

    const char* apszGoalie[kRatingsGoalieCount] = {
        // STRING: HYPERX 0x004c7d24
        "id_GRT_WeightClass",
        // STRING: HYPERX 0x004c7d14
        "id_GRT_Strength",
        // STRING: HYPERX 0x004c7d00
        "id_GRT_ReactionTime",
        // STRING: HYPERX 0x004c7cf0
        "id_GRT_Balance",
        // STRING: HYPERX 0x004c7ce0
        "id_GRT_Speed",
        // STRING: HYPERX 0x004c7ccc
        "id_GRT_PassAccuracy",
        // STRING: HYPERX 0x004c7cb8
        "id_GRT_Intelligence",
        // STRING: HYPERX 0x004c7ca0
        "id_GRT_UnderPressure",
        // STRING: HYPERX 0x004c7c8c
        "id_GRT_SaveRight",
        // STRING: HYPERX 0x004c7c7c
        "id_GRT_SaveLeft",
        // STRING: HYPERX 0x004c7c6c
        "id_GRT_SaveHigh",
        // STRING: HYPERX 0x004c7c5c
        "id_GRT_SaveLow",
        // STRING: HYPERX 0x004c7c44
        "id_GRT_ShieldBlocking",
        // STRING: HYPERX 0x004c7c2c
        "id_GRT_CatchingAbility",
        // STRING: HYPERX 0x004c7c18
        "id_GRT_Aggression",
        // STRING: HYPERX 0x004c7c08
        "id_GRT_Overall",
        "EndRatings"
    };

    const char**  ppszTable;
    char          szToken[kParseTokenMax];
    char          szObject[kParseTokenMax];
    int           nCount;
    int           nLast;
    int           nValue;
    unsigned char nEntry;

    if (pDef == nullptr)
    {
        return -1;
    }

    if (nMode == kRatingsField)
    {
        ppszTable = apszPlayer;
        nCount    = kRatingsPlayerCount;
        nLast     = kRatingCount;
    }
    else
    {
        ppszTable = apszGoalie;
        nCount    = kRatingsGoalieCount;
        nLast     = kGoalieRatingCount;
    }

    if (pDef->m_apRatings[nLevel] == nullptr)
    {
        pDef->m_nRatingsMode = nMode;

        if (nMode == kRatingsField)
        {
            pDef->m_apRatings[nLevel] = new float[kRatingCount];

            for (nEntry = 0; nEntry < kRatingCount; nEntry++)
            {
                pDef->m_apRatings[nLevel][nEntry] = 0.0f;
            }
        }
        else
        {
            pDef->m_apRatings[nLevel] = new float[kGoalieRatingCount];

            for (nEntry = 0; nEntry < kGoalieRatingCount; nEntry++)
            {
                pDef->m_apRatings[nLevel][nEntry] = GOALIE_RATING_DEFAULT;
            }
        }
    }

    auto* const pflRatings = pDef->m_apRatings[nLevel];

    auto* pszCursor = pszLine;

    while (pszCursor != nullptr)
    {
        if (ParseUtil::ParseToken(pszCursor, szToken, nullptr, " =\t", kParseTokenMax, &pszCursor) == 0)
        {
            break;
        }

        pszCursor = ParseUtil::SkipToChar(pszCursor, g_pszEqualsSign);

        if (pszCursor == nullptr)
        {
            break;
        }

        const auto nKeyword = ParseUtil::FindKeyword(szToken, ppszTable, static_cast<short>(nCount));

        if (nKeyword < 0)
        {
            break;
        }

        pszCursor++;

        if (ParseUtil::ParseToken(pszCursor, szToken, nullptr, " \t\n\r", kParseTokenMax, &pszCursor) != 0 && sscanf(szToken, g_pszPercentD, &nValue) == 1)
        {
            if (nKeyword >= nLast)
            {
                g_gameApp.Fatal(
                    // STRING: HYPERX 0x004c7bd8
                    "Game Config file error: ratings for object %s!",
                    szObject);
            }

            pflRatings[nKeyword] = static_cast<float>(nValue);
        }
        else
        {
            g_gameApp.Fatal("Game Config file error: ratings for object %s!", szObject);
        }
    }

    return 0;
}

// The modifier map: a bubble kind on the way in, its CollisionObjectID on the way out.

// FUNCTION: HYPERX 0x00484070
void GameEngine::MapModifierToId(short nModelId, BubbleCmd* pCmd)
{
    short aRow[4][6] = {
        { kCOBDr0Awareness, kCOBDr1Awareness, kCOBDr2Awareness, kCOBDr3Awareness, kCOBDr4Awareness, kCOBDr5Awareness },
        { kCOBDromer0, kCOBDromer1, kCOBDromer2, kCOBDromer3, kCOBDromer4, kCOBDromer5 },
        { kCOBDromerBody0, kCOBDromerBody1, kCOBDromerBody2, kCOBDromerBody3, kCOBDromerBody4, kCOBDromerBody5 },
        { kCOBBatonEzone0, kCOBBatonEzone1, kCOBBatonEzone2, kCOBBatonEzone3, kCOBBatonEzone4, kCOBBatonEzone5 }
    };
    short aPair[4][2] = {
        { kCOBNgoalSide, kCOBSgoalSide },
        { kCOBNgoalZone, kCOBSgoalZone },
        { kCOBNgoalCase, kCOBSgoalCase },
        { kCOBNgoalPort, kCOBSgoalPort }
    };

    switch (static_cast<unsigned short>(pCmd->m_nSlot))
    {
        case kBubbleBodyPart:
            pCmd->m_nSlot = nModelId - g_abObjectTypeBaseline[kGOBBodyPart] + kCOBBodyPart0;
            return;
        case kBubbleDromerAwareness:
            pCmd->m_nSlot = aRow[0][nModelId];
            return;
        case kBubbleDromerModel:
            pCmd->m_nSlot = aRow[1][nModelId];
            return;
        case kBubbleDromerBody:
            pCmd->m_nSlot = aRow[2][nModelId];
            return;
        case kBubbleDromerScoop:
            pCmd->m_nSlot = aRow[3][nModelId];
            if (pCmd->m_wShape == BubbleShape::kAttached)
            {
                pCmd->m_pAttach->nSlot = nModelId;
            }
            break;
        case kBubbleRok:
            pCmd->m_nSlot = kCOBRok;
            return;
        case kBubbleRamp:
            pCmd->m_nSlot = nModelId - g_abObjectTypeBaseline[kGOBRamp] + kCOBRamp;
            return;
        case kBubbleGoalSide:
            pCmd->m_nSlot = aPair[0][nModelId - kSlotNgoal];
            return;
        case kBubbleGoalZone:
            pCmd->m_nSlot = aPair[1][nModelId - kSlotNgoal];
            return;
        case kBubbleGoalCase:
            pCmd->m_nSlot = aPair[2][nModelId - kSlotNgoal];
            return;
        case kBubbleGoalPort:
            pCmd->m_nSlot = aPair[3][nModelId - kSlotNgoal];
            return;
        case kBubbleCamera:
            pCmd->m_nSlot = kCOBCamera;
            return;
        case kBubbleHeadCam:
            pCmd->m_nSlot = kCOBHeadCam;
            return;
        case kBubbleStartgun:
            pCmd->m_nSlot = kCOBStartGun;
            return;
        case kBubbleTurnstile:
            pCmd->m_nSlot = nModelId - g_abObjectTypeBaseline[kGOBTurnstile] + kCOBTurnstile;
            return;
        case kBubbleMultiplier:
            pCmd->m_nSlot = nModelId - g_abObjectTypeBaseline[kGOBMultiplier] + kCOBMultiplier;
            return;
        case kBubbleLaserHurdle:
            pCmd->m_nSlot = nModelId - g_abObjectTypeBaseline[kGOBLaserHurdle] + kCOBLaserHurdle;
            return;
        case kBubbleKludge:
            pCmd->m_nSlot = nModelId - g_abObjectTypeBaseline[kGOBRedRandomizer] + kCOBRedRandomizer;
            return;
        case kBubbleRandomizer:
            pCmd->m_nSlot = nModelId - g_abObjectTypeBaseline[kGOBGreenRandomizer] + kCOBGreenRandomizer;
            return;
        case kBubbleLimboPole:
            pCmd->m_nSlot = nModelId - g_abObjectTypeBaseline[kGOBLimboPole] + kCOBLimboPole;
            return;
        case kBubbleHurdle:
            pCmd->m_nSlot = nModelId - g_abObjectTypeBaseline[kGOBHurdle] + kCOBHurdle;
            return;
        case kBubbleFastLane:
            pCmd->m_nSlot = nModelId - g_abObjectTypeBaseline[kGOBFastLane] + kCOBFastLane;
            return;
        case kBubbleExplosion:
            pCmd->m_nSlot = nModelId - g_abObjectTypeBaseline[kGOBExplosion] + kCOBExplosion;
            return;
        case kBubbleArmory:
            pCmd->m_nSlot = nModelId - g_abObjectTypeBaseline[kGOBArmory] + kCOBArmory;
            return;
        case kBubbleSlowLane:
            pCmd->m_nSlot = nModelId - g_abObjectTypeBaseline[kGOBSlowLane] + kCOBSlowLane;
            return;
        case kBubbleMine:
            pCmd->m_nSlot = nModelId - g_abObjectTypeBaseline[kGOBMine] + kCOBMine;
            return;
        case kBubbleSwarf:
            pCmd->m_nSlot = nModelId - g_abObjectTypeBaseline[kGOBSwarf] + kCOBSwarf;
            return;
        default:
            g_gameApp.Fatal(
                // STRING: HYPERX 0x004c8264
                "Game Config Error: unused mod ID %hd: in SetUpGameObjectAssignment!",
                nModelId);
            break;
    }
}

// This parser's own token bound. src/common/parseutil.h has 0x78 and planscat.cpp uses 0x100.
constexpr auto kPlanTokenMax = 0x50;
constexpr auto kTokensMax    = 30;

// A token longer than the buffer overruns it; nothing bounds the token count.
// FUNCTION: HYPERX 0x00484460
char GameEngine::Cmd_SetPlans(char* pszText, ObjectDef* pDef)
{
    char* apTokens[kTokensMax];
    char  szToken[kPlanTokenMax];

    short nIndex;

    short nParsed = 0;

    memset(szToken, 0, sizeof(szToken));

    auto* pszCursor = pszText;

    while (pszCursor != nullptr)
    {
        // STRING: HYPERX 0x004c7324
        if (ParseUtil::ParseToken(pszCursor, szToken, nullptr, " \t\n\r", static_cast<short>(strlen(pszCursor)), &pszCursor) == 0)
        {
            break;
        }

        char** ppSlot = &apTokens[nParsed];

        nParsed++;

        *ppSlot = strcpy(new char[strlen(szToken) + 1], szToken);
    }

    const auto   nOld  = pDef->m_nPlanCount;
    char** const apOld = pDef->m_apszPlanNames;

    nParsed += nOld;

    pDef->m_nPlanCount = nParsed;

    pDef->m_apszPlanNames = new char*[nParsed];

    if (nOld != 0)
    {
        for (nIndex = 0; nIndex < nOld; nIndex++)
        {
            pDef->m_apszPlanNames[nIndex] = apOld[nIndex];
        }

        delete[] apOld;
    }

    for (nIndex = nOld; nIndex < pDef->m_nPlanCount; nIndex++)
    {
        pDef->m_apszPlanNames[nIndex] = apTokens[nIndex - nOld];
    }

    return 1;
}

// A type's id counter never goes below that type's baseline.
// FUNCTION: HYPERX 0x00484be0
void GameEngine::UnloadArena()
{
    int nType;

    // Objects are removed last first; the count ends one below zero.
    while (m_pArena->m_wCount-- != 0)
    {
        const auto nId = m_pArena->m_pnSlotIds[m_pArena->m_wCount];

        auto* const pObject = m_ppSlots[static_cast<short>(nId)];

        RemoveFromSortedList(static_cast<short>(m_pArena->m_pnSlotIds[m_pArena->m_wCount]));

        UnregisterObject(m_pArena->m_pnSlotIds[m_pArena->m_wCount]);

        for (nType = 0; nType < kGOBNumOfTypes; nType++)
        {
            if (nId >= g_abObjectTypeBaseline[nType] && nId <= g_abObjectTypeLimit[nType])
            {
                if (m_abTypeHighWater[nType] > g_abObjectTypeBaseline[nType])
                {
                    m_abTypeHighWater[nType]--;
                }

                break;
            }
        }

        delete pObject;
    }

    auto* const pArena = m_pArena;

    if (pArena != nullptr)
    {
        delete[] pArena->m_pnSlotIds;
        delete pArena->m_pRokDef;
        delete pArena;
    }

    m_pArena = nullptr;
}

// The type id starts at 102, so a file that names no type still produces something.
// FUNCTION: HYPERX 0x00484cf0
char GameEngine::LoadObjectsFromConfig(const char* pszFileName)
{
    OwnedFile  file;
    PathString strPath(String::Concat(g_gameApp.m_strExeDir.m_str, pszFileName).m_pchData, kMaxPathLength);

    if (file.m_file.Open(strPath.m_str.m_pchData, FileMode::kRead) != 0)
    {
        unsigned int nModelId = kModelIDNone;
        auto*        pDef     = ParseObjectDefinition(&file.m_file, &nModelId);

        while (pDef != nullptr)
        {
            CreateObject(pDef, nModelId);

            delete pDef;

            pDef = ParseObjectDefinition(&file.m_file, &nModelId);
        }

        file.m_file.CloseAndTest();

        return 1;
    }

    g_gameApp.ReportLastError();
    g_gameApp.OnShutdown();

    return 0;
}

constexpr auto kArenaObjectMax = 100;

// Fatal returns, so an object past the hundredth is still built and overruns anModelIds.
// FUNCTION: HYPERX 0x00484e80
char GameEngine::LoadArena(const char* pszArenaPath)
{
    OwnedFile    file;
    unsigned int anModelIds[kArenaObjectMax];

    if (m_pArena != nullptr)
    {
        UnloadArena();
    }

    m_pArena = nullptr;

    PathString strPath(String::Concat(g_gameApp.m_strExeDir.m_str, pszArenaPath).m_pchData, kMaxPathLength);

    if (file.m_file.Open(strPath.m_str.m_pchData, FileMode::kRead) != 0)
    {
        m_pArena = new ArenaState;

        Renderer::SetLoadingMode();

        unsigned int nModelId = kModelIDNone;
        auto*        pDef     = ParseObjectDefinition(&file.m_file, &nModelId);

        while (pDef != nullptr)
        {
            if (m_pArena->m_wCount >= kArenaObjectMax)
            {
                auto* const pApp = &g_gameApp;

                // STRING: HYPERX 0x004c8384
                pApp->Fatal("Too many objects in .ENV file");
            }

            CreateObject(pDef, nModelId);

            anModelIds[m_pArena->m_wCount++] = nModelId;

            if (nModelId == kSlotRok)
            {
                m_pArena->m_pRokDef = pDef;
            }
            else
            {
                delete pDef;
            }

            pDef = ParseObjectDefinition(&file.m_file, &nModelId);
        }

        m_pArena->m_pnSlotIds = new int[m_pArena->m_wCount];

        memcpy(m_pArena->m_pnSlotIds, anModelIds, m_pArena->m_wCount * sizeof(int));

        file.m_file.CloseAndTest();

        return 1;
    }

    g_gameApp.ReportLastError();

    return 0;
}

// Selecting the arena already selected answers true at once.
// FUNCTION: HYPERX 0x004850f0
char GameEngine::SetArena(short nArena)
{
    const char* pszPath;
    int         nIndex;

    if (nArena == static_cast<short>(m_nArenaIndex))
    {
        return 1;
    }

    m_nArenaIndex = nArena;

    auto* const pTable = g_pGameEngine->m_pTeamTable;

    if (nArena < pTable->GetTeamCount())
    {
        pszPath = pTable->m_ppTeams[nArena]->szEnvironment;
    }
    else
    {
        pszPath = nullptr;
    }

    if (LoadArena(pszPath) != 0)
    {
        const int nCount = m_pArena->m_wCount;

        for (nIndex = 0; nIndex < nCount; nIndex++)
        {
            auto* pObject = m_ppSlots[static_cast<short>(m_pArena->m_pnSlotIds[nIndex])];

            if (pObject != nullptr && pObject->GetSlot() != kSlotDrome)
            {
                pObject->CompileAllPlans();
                pObject->SelectAndActivatePlan();
            }
        }

        return 1;
    }

    return 0;
}

// FUNCTION: HYPERX 0x004851a0
void GameEngine::UnloadEntitySet(TeamBlock* pSet)
{
    int iRange;
    int nIndex;

    if (pSet == nullptr)
    {
        return;
    }

    for (nIndex = 0; nIndex < kTeamBlockSlots; nIndex++)
    {
        auto* const pEntity = pSet->apOnIce[nIndex];

        if (pEntity != nullptr)
        {
            const int nType = pEntity->GetSlot();

            for (iRange = 0; iRange < kGOBNumOfTypes; iRange++)
            {
                if (nType >= g_abObjectTypeBaseline[iRange] && nType <= g_abObjectTypeLimit[iRange])
                {
                    if (m_abTypeHighWater[iRange] > g_abObjectTypeBaseline[iRange])
                    {
                        m_abTypeHighWater[iRange]--;
                    }

                    break;
                }
            }

            RemoveFromSortedList(static_cast<short>(nType));

            UnregisterObject(nType);

            delete pEntity;
        }
    }

    delete pSet;
}

// FUNCTION: HYPERX 0x00485250
char GameEngine::UnloadHomeTeam()
{
    auto* const pSet = m_pTeamA;

    if (pSet != nullptr)
    {
        UnloadEntitySet(pSet);

        m_pTeamA = nullptr;

        return 1;
    }

    return 0;
}

// FUNCTION: HYPERX 0x00485280
char GameEngine::UnloadAwayTeam()
{
    auto* const pSet = m_pTeamB;

    if (pSet != nullptr)
    {
        UnloadEntitySet(pSet);

        m_pTeamB = nullptr;

        return 1;
    }

    return 0;
}

constexpr auto kSecondRunFirst = 5;
constexpr auto kSecondRunLast  = 8;
constexpr auto kFirstRunLast   = 2;

// A rematch reloads the same teams; a failed load names the home team whichever half failed.
// FUNCTION: HYPERX 0x004856f0
char GameEngine::SetTeams(short nHome, short nAway)
{
    TeamsBlock* pTable;
    const char* pszTeam;
    int         nSlot;

    Window::SetBusyCursor(true);

    if (nHome != static_cast<short>(m_nTeamHomeIndex) || m_bMatchOver != 0)
    {
        UnloadHomeTeam();
    }

    if (nAway != static_cast<short>(m_nTeamAwayIndex) || m_bMatchOver != 0)
    {
        UnloadAwayTeam();
    }

    if (nHome != static_cast<short>(m_nTeamHomeIndex) || m_bMatchOver != 0)
    {
        m_nTeamHomeIndex = nHome;

        pTable = g_pGameEngine->m_pTeamTable;

        if (nHome < pTable->GetTeamCount())
        {
            pszTeam = pTable->m_ppTeams[nHome]->szPath;
        }
        else
        {
            pszTeam = nullptr;
        }

        if (LoadHomeTeam(pszTeam) == 0)
        {
            // STRING: HYPERX 0x004c83f0
            g_gameApp.Fatal("Error initializing home team\n");
        }

        for (nSlot = 0; nSlot < kTeamBlockSlots; nSlot++)
        {
            if ((nSlot >= kSecondRunFirst && nSlot <= kSecondRunLast) || (nSlot >= 0 && nSlot <= kFirstRunLast))
            {
                m_pTeamA->apOnIce[nSlot]->CompileAllPlans();
            }
        }
    }

    if (nAway != static_cast<short>(m_nTeamAwayIndex) || m_bMatchOver != 0)
    {
        m_nTeamAwayIndex = nAway;

        pTable = g_pGameEngine->m_pTeamTable;

        if (nAway < pTable->GetTeamCount())
        {
            pszTeam = pTable->m_ppTeams[nAway]->szPath;
        }
        else
        {
            pszTeam = nullptr;
        }

        if (LoadAwayTeam(pszTeam) == 0)
        {
            g_gameApp.Fatal("Error initializing home team\n");
        }

        for (nSlot = 0; nSlot < kTeamBlockSlots; nSlot++)
        {
            if ((nSlot >= kSecondRunFirst && nSlot <= kSecondRunLast) || (nSlot >= 0 && nSlot <= kFirstRunLast))
            {
                m_pTeamB->apOnIce[nSlot]->CompileAllPlans();
            }
        }
    }

    m_bMatchOver = 0;

    Window::SetBusyCursor(false);

    return 1;
}

// The body part slots' object ids, the home team's four and then the away team's four.
// GLOBAL: HYPERX 0x004c70f0
extern int g_anBodyPartModelIds[8];

constexpr auto kModelIDUnassigned = (-1);

constexpr auto kTeamSideAway = 1;

// The ten slots, and which of them are the body parts.
constexpr auto kTeamSlotBodyPartFirst = 5;
constexpr auto kTeamSlotBodyPartLast  = 8;
constexpr auto kTeamSlotRok           = 9;

// Three players carry the running id; the fourth and fifth do not.
constexpr auto kTeamPlayersOnDrome = 3;

// Ten slots: five players, four body parts and the team's Rok; only four players get ratings.
// FUNCTION: HYPERX 0x004858b0
TeamBlock* GameEngine::LoadTeamFromFile(const char* pszTeamPath, int nTeam)
{
    OwnedFile  file;
    ObjectDef* pObjDef;
    int        nRow;

    PathString strPath(String::Concat(g_gameApp.m_strExeDir.m_str, pszTeamPath).m_pchData,
                       kMaxPathLength);

    if (file.m_file.Open(strPath.m_str.m_pchData, FileMode::kRead) != 0)
    {
        auto nSlot = 0;

        auto* const pTeamDef = ParseTeamHTMFile(&file.m_file);

        if (pTeamDef == nullptr)
        {
            // STRING: HYPERX 0x004c8478
            g_gameApp.Fatal("Away team definition error in file %s!", pszTeamPath);

            return nullptr;
        }

        strcpy(pTeamDef->szPath, pszTeamPath);

        auto nPlayers   = 0;
        auto nRunningId = nTeam;

        for (; nSlot < kTeamBlockSlots; nSlot++)
        {
            int          nBodyPartRow;
            int          nKind;
            int          nLevel;
            const float* pRatings;
            float*       pRow;
            int          i;

            unsigned int nObjectId = kModelIDNone;

            if (nPlayers < kTeamPlayersOnDrome)
            {
                g_bSuppressRokId = 0;
                nObjectId        = nRunningId;
            }
            else if (nSlot == kTeamSlotRok)
            {
                g_bSuppressRokId = 1;
                nObjectId        = static_cast<unsigned int>(kModelIDUnassigned);
            }
            else if (nSlot >= kTeamSlotBodyPartFirst && nSlot <= kTeamSlotBodyPartLast)
            {
                nBodyPartRow = nSlot - kTeamSlotBodyPartFirst;

                if (nTeam == kTeamSideAway)
                {
                    nBodyPartRow += 4;
                }

                nObjectId = g_anBodyPartModelIds[nBodyPartRow];
            }

            pObjDef = ParseObjectDefinition(&file.m_file, &nObjectId);

            g_bSuppressRokId = 0;

            if (pObjDef == nullptr)
            {
                // STRING: HYPERX 0x004c8454
                g_gameApp.Fatal("Team definition file incomplete %s!", pszTeamPath);
            }

            nKind = pObjDef->m_nType;

            if (nKind == kGOBDromer)
            {
                nPlayers++;
                nRunningId += 2;
            }

            if (nTeam == kTeamSideAway)
            {
                pObjDef->m_bForceRemap = 1;
            }

            if ((pObjDef->m_nType == kGOBDromer && nPlayers <= kTeamPlayersOnDrome) || pObjDef->m_nType == kGOBBodyPart)
            {
                pTeamDef->apOnIce[nSlot] = CreateObject(pObjDef, nObjectId);
            }
            else
            {
                pTeamDef->apOnIce[nSlot] = nullptr;
            }

            switch (nSlot)
            {
                case 0:
                case 1:
                case 2:
                case 3:
                case 4:
                    if (pObjDef->m_nType != kGOBDromer)
                    {
                        // STRING: HYPERX 0x004c8430
                        g_gameApp.Fatal("Team definition file invalid %s!", pszTeamPath);
                    }

                    break;

                case 5:
                case 6:
                case 7:
                case 8:
                    if (pObjDef->m_nType != kGOBBodyPart)
                    {
                        g_gameApp.Fatal("Team definition file invalid %s!", pszTeamPath);
                    }

                    break;
            }

            switch (nSlot)
            {
                case 0:
                    nRow = 0;
                    break;

                case 1:
                    nRow = 1;
                    break;

                case 3:
                    nRow = 2;
                    break;

                case 4:
                    nRow = 3;
                    break;

                default:
                    nRow = -1;
                    break;
            }

            // Only the four outfield seats take ratings.
            if (nRow >= 0)
            {
                nLevel = static_cast<int>(m_nDifficulty);

                if (g_bSessionActive != 0)
                {
                    nLevel = 0;
                }

                pRatings = pObjDef->m_apRatings[nLevel];

                if (pRatings == nullptr)
                {
                    pRatings = pObjDef->m_apRatings[0];
                }

                pRow = pTeamDef->aRows[nRow].aStat;

                for (i = 0; i < kRatingCount; i++)
                {
                    *pRow = *pRatings;

                    pRatings++;
                    pRow++;
                }

                memset(&pTeamDef->aStatRows[nRow], 0, sizeof(TeamStatRow));
            }

            if (nSlot >= kTeamSlotBodyPartFirst && nSlot <= kTeamSlotBodyPartLast)
            {
                delete pObjDef;

                pObjDef = nullptr;
            }

            pTeamDef->apDefs[nSlot] = pObjDef;

            pTeamDef->aSeat[0] = 0;
            pTeamDef->aSeat[1] = 1;
        }

        file.m_file.CloseAndTest();

        return pTeamDef;
    }

    // STRING: HYPERX 0x004c8410
    g_gameApp.Fatal("Can't Open away team file %s!", pszTeamPath);

    return nullptr;
}

// FUNCTION: HYPERX 0x00485c70
char GameEngine::LoadHomeTeam(const char* pszTeamPath)
{
    auto* const pTeam = LoadTeamFromFile(pszTeamPath, 0);

    m_pTeamA = pTeam;

    if (pTeam != nullptr)
    {
        if (g_pRenderer != nullptr)
        {
            g_pRenderer->SetHomeLogo(pTeam->szIdLogo);
        }

        return 1;
    }

    return 0;
}

// FUNCTION: HYPERX 0x00485cb0
char GameEngine::LoadAwayTeam(const char* pszTeamPath)
{
    auto* const pTeam = LoadTeamFromFile(pszTeamPath, 1);

    m_pTeamB = pTeam;

    if (pTeam != nullptr)
    {
        if (g_pRenderer != nullptr)
        {
            g_pRenderer->SetAwayLogo(pTeam->szIdLogo);
        }

        return 1;
    }

    return 0;
}

// FUNCTION: HYPERX 0x00485cf0
ObjectDef* GameEngine::GetArenaRokDef()
{
    if (m_pArena != nullptr)
    {
        auto* const pDef = m_pArena->m_pRokDef;

        if (pDef != nullptr)
        {
            return pDef;
        }
    }

    return nullptr;
}

// FUNCTION: HYPERX 0x00485d10
ObjectDef* GameEngine::GetTeamRokDef(int nSlot)
{
    const TeamBlock* pCatalog;

    if (nSlot < kSlotDromer0 || nSlot > kSlotDromer3)
    {
        return nullptr;
    }

    if ((nSlot & 1) != 0)
    {
        pCatalog = m_pTeamB;
    }
    else
    {
        pCatalog = m_pTeamA;
    }

    if (pCatalog != nullptr)
    {
        auto* const pEntry = pCatalog->apDefs[kTeamSlotRok];

        if (pEntry != nullptr)
        {
            return pEntry;
        }
    }

    return nullptr;
}

// A reserve's recovery since leaving the ice is added once, clamped at a hundred.
// FUNCTION: HYPERX 0x00485d50
char GameEngine::SubstitutePlayer(int nSlot, int nWith)
{
    TeamBlock* pTeam;
    int        nDefIndex;
    int        nSide;

    switch (static_cast<unsigned int>(nWith))
    {
        case 0:
            nDefIndex = 0;
            break;

        case 1:
            nDefIndex = 1;
            break;

        case 2:
            nDefIndex = 3;
            break;

        case 3:
            nDefIndex = 4;
            break;

        default:
            return 0;
    }

    switch (static_cast<unsigned int>(nSlot))
    {
        case 0:
            pTeam = m_pTeamA;
            nSide = 0;
            break;

        case 2:
            pTeam = m_pTeamA;
            nSide = 1;
            break;

        case 1:
            nSide = 0;
            pTeam = m_pTeamB;
            break;

        case 3:
            nSide = 1;
            pTeam = m_pTeamB;
            break;

        default:
            return 0;
    }

    auto* const pPlayer    = (PlayerObject*)pTeam->apOnIce[nSide];
    int         nOnIceSlot = pPlayer->GetSlot();
    auto* const pBody      = &pPlayer->m_stats;

    pBody->CopyBaseStats();

    const auto nWasOn = pTeam->aSeat[nSide];

    memmove(&pTeam->aRows[nWasOn], &pPlayer->m_stats.m_aStats[1], sizeof(TeamPlayerRow));
    memmove(&pTeam->aStatRows[nWasOn], &pPlayer->m_matchStats, sizeof(TeamStatRow));

    pTeam->aReserveSince[nWasOn] = m_clock.tick;

    const auto nStamp = pTeam->aReserveSince[nWith];

    if (nStamp != Clock::Milliseconds{})
    {
        const auto nElapsed = static_cast<int>((m_clock.tick - nStamp).count());
        float      flGate;

        flGate = DamageGate();

        const auto  flRested  = static_cast<float>(static_cast<unsigned int>(nElapsed));
        auto* const pflDamage = &pTeam->aRows[nWith].aStat[kRatingDamage];
        float       flDamage;

        if (100.0f < flRested / (13500.0f / flGate) + *pflDamage)
        {
            flDamage = 100.0f;
        }
        else
        {
            flDamage = flRested / (13500.0f / DamageGate()) + *pflDamage;
        }

        *pflDamage = flDamage;

        pTeam->aReserveSince[nWith] = {};
    }

    ReplaceEntity(nOnIceSlot,
                  pTeam->apDefs[nDefIndex]);

    pPlayer->ApplyRenderSlotPose();

    memmove(&pPlayer->m_stats.m_aStats[1], &pTeam->aRows[nWith], sizeof(TeamPlayerRow));
    memmove((void*)&pPlayer->m_matchStats, &pTeam->aStatRows[nWith], sizeof(TeamStatRow));

    nOnIceSlot = pPlayer->GetSlot();

    pBody->ScaleTo(nOnIceSlot);

    pBody->m_aStats[0] = 100.0f;

    pTeam->aSeat[nSide] = nWith;

    return 1;
}

constexpr auto kSlotTeamOneFirst  = 0;
constexpr auto kSlotTeamTwoFirst  = 1;
constexpr auto kSlotTeamOneSecond = 2;
constexpr auto kSlotTeamTwoSecond = 3;

// DamageGate's third arm asks the difficulty and discards the answer.
// FUNCTION: HYPERX 0x00486030
unsigned char GameEngine::IsFitSubstitute(unsigned int nSlot, int nWhich)
{
    const TeamBlock* pTeam;
    int              nSelf;
    int              nOther;

    switch (nWhich)
    {
        case 0:
            break;

        case 1:
            break;

        case 2:
            break;

        case 3:
            break;

        default:
            return 0;
    }

    switch (nSlot)
    {
        case kSlotTeamOneFirst:
            pTeam  = m_pTeamA;
            nSelf  = 0;
            nOther = 1;
            break;

        case kSlotTeamOneSecond:
            pTeam  = m_pTeamA;
            nSelf  = 1;
            nOther = 0;
            break;

        case kSlotTeamTwoFirst:
            nSelf  = 0;
            nOther = 1;
            pTeam  = m_pTeamB;
            break;

        case kSlotTeamTwoSecond:
            nSelf  = 1;
            nOther = 0;
            pTeam  = m_pTeamB;
            break;

        default:
            return 0;
    }

    if (nWhich == pTeam->aSeat[nSelf])
    {
        return 0;
    }

    if (nWhich == pTeam->aSeat[nOther])
    {
        return 0;
    }

    auto* const pEntity = (PlayerObject*)pTeam->apOnIce[nOther];

    if (nWhich == pEntity->m_wDesiredSubstitute)
    {
        return 0;
    }

    if (DamageGate() < pTeam->aRows[nWhich].aStat[kRatingDamage])
    {
        return 1;
    }

    return 0;
}

// The larger block goes into the 252-byte slots and the smaller into the 196-byte ones.
// FUNCTION: HYPERX 0x00486140
void GameEngine::SyncPlayerDataToTeam()
{
    TeamBlock* pTeam;
    int        nSeat;

    auto nSide = 0;

    do
    {
        if (nSide == 0)
        {
            pTeam = m_pTeamA;
        }
        else
        {
            pTeam = m_pTeamB;
        }

        if (pTeam != nullptr)
        {
            for (nSeat = 0; nSeat < kTeamBlockSeats; nSeat++)
            {
                const auto  nSlot   = pTeam->aSeat[nSeat];
                auto* const pPlayer = (PlayerObject*)pTeam->apOnIce[nSeat];

                if (pPlayer != nullptr)
                {
                    memmove(&pTeam->aRows[nSlot], &pPlayer->m_stats.m_aStats[1], sizeof(TeamPlayerRow));
                    memmove(&pTeam->aStatRows[nSlot], &pPlayer->m_matchStats, sizeof(TeamStatRow));
                }
            }
        }

        nSide++;
    } while (nSide < 2);
}

// Undecoded kinds fall to the base object; construction cost is charged to the install level.
// FUNCTION: HYPERX 0x00486200
EngineObject* GameEngine::CreateObject(ObjectDef* pDef, unsigned int nModelId)
{
    EngineObject* pNew;
    const int     nEntry = nModelId;

    const auto nStart = GameApp::GetInstallLevel();

    switch (pDef->m_nTypeId)
    {
        case ObjectKind::kDromer:
            pNew = new PlayerObject(nEntry, pDef);
            break;

        case ObjectKind::kRok:
            pNew = new RokObject(nEntry, pDef);
            break;

        case ObjectKind::kMine:
        case ObjectKind::kSwarf:
            pNew = new WeaponObject(nEntry, pDef);
            break;

        case ObjectKind::kGoal:
            pNew = new GoalObject(nEntry, pDef);
            break;

        case ObjectKind::kCamera:
        {
            if (m_wCameraCount == kCameraLimit)
            {
                // STRING: HYPERX 0x004c84b8
                g_gameApp.Fatal("Too many cameras!");
            }

            auto* const pCamera = new CameraObject(nEntry, pDef);

            m_apCameras[m_wCameraCount] = pCamera;

            pNew = pCamera;

            m_wCameraCount++;
            break;
        }

        case ObjectKind::kKeyboard:
            if (m_wDeviceCount == kDeviceLimit)
            {
                // STRING: HYPERX 0x004c84a0
                g_gameApp.Fatal("Too many input devices!");
            }

            pNew = new KeyboardDeviceObj(nEntry, pDef);

            m_apDevices[m_wDeviceCount] = pNew;

            m_wDeviceCount++;

            g_pInputDevice = (class KeyboardDeviceObj*)pNew;
            break;

        case ObjectKind::kGamepad:
            if (m_wDeviceCount == kDeviceLimit)
            {
                g_gameApp.Fatal("Too many input devices!");
            }

            pNew = new GamepadDeviceObj(nEntry, pDef);

            m_apDevices[m_wDeviceCount] = pNew;

            m_wDeviceCount++;
            break;

        case ObjectKind::kJoystick:
            if (m_wDeviceCount == kDeviceLimit)
            {
                g_gameApp.Fatal("Too many input devices!");
            }

            pNew = new JoystickDeviceObj(nEntry, pDef);

            m_apDevices[m_wDeviceCount] = pNew;

            m_wDeviceCount++;
            break;

        case ObjectKind::kMouse:
            if (m_wDeviceCount == kDeviceLimit)
            {
                g_gameApp.Fatal("Too many input devices!");
            }

            pNew = new MouseDeviceObj(nEntry, pDef);

            m_apDevices[m_wDeviceCount] = pNew;

            m_wDeviceCount++;
            break;

        case ObjectKind::kDrome:
            pNew = new DromeObject(nEntry, pDef);
            break;

        case ObjectKind::kRamp:
            pNew = new RampObject(nEntry, pDef);
            break;

        case ObjectKind::kExplosion:
            pNew = new ParticleSystem(nEntry, pDef);
            break;

        case ObjectKind::kCoach:
            pNew = new CoachObject(nEntry, pDef);
            break;

        default:
            pNew = new EngineObject(nEntry, pDef);
            break;
    }

    RegisterObject(nEntry, pNew);

    pNew->m_installCost.nLevels += GameApp::GetInstallLevel() - nStart;

    m_nLoadCost -= pNew->m_installCost.nLevels;

    if (nEntry != kSlotDrome)
    {
        InsertObjectSorted(static_cast<short>(nEntry));
    }

    return pNew;
}

// The length of both slot tables, m_ppSlots and m_ppObjects.
constexpr auto kEngineSlotCount = 0x66;

// The first slot array is the camera holder; stubbing it faults the graphics set-up.
// FUNCTION: HYPERX 0x00486680
void GameEngine::Init(const char* pszConfig)
{
    int nIndex;

    auto* const pState = new ArenaState;

    m_pArena = pState;

    m_nViewedSlot = 0;

    for (nIndex = 0; nIndex < kGOBNumOfTypes; nIndex++)
    {
        m_abTypeHighWater[nIndex] = g_abObjectTypeBaseline[nIndex];
    }

    InitMatchTimers();

    m_flGravityDt = g_flGravityGameEngine * m_clock.flDeltaSquared;

    InitPlanSystems();

    m_nPending = -1;

    auto** const ppSlots = new EngineObject*[kEngineSlotCount];

    m_ppSlots = ppSlots;

    memset(ppSlots, 0, kEngineSlotCount * sizeof(EngineObject*));

    auto** const ppObjects = new EngineObject*[kEngineSlotCount];

    m_ppObjects = ppObjects;

    memset(ppObjects, 0, kEngineSlotCount * sizeof(EngineObject*));

    m_nLoadedCount = 0;
    m_nObjectCount = 0;

    LoadConfigFile(pszConfig);

    // STRING: HYPERX 0x004b3f38
    LoadObjectsFromConfig("common.cfg");

    SyncRenderQueueSize();

    m_apTracked[0] = m_ppSlots[0];
    m_apTracked[1] = m_ppSlots[0];
}

// FUNCTION: HYPERX 0x004867b0
char GameEngine::SyncRenderQueueSize()
{
    if (m_nQueuedSlotCount == m_nSlotCount)
    {
        return 0;
    }

    CameraObject** ppCamera   = m_apCameras;
    auto           nRemaining = kCameraLimit;

    do
    {
        if (*ppCamera != nullptr)
        {
            (*ppCamera)->ResizeTransparentQueue(m_nSlotCount);
        }

        ppCamera++;
        nRemaining--;
    } while (nRemaining != 0);

    m_nQueuedSlotCount = m_nSlotCount;

    return 1;
}

// FUNCTION: HYPERX 0x004867f0
void GameEngine::StartMatch()
{
    short nSlot;

    m_wRunState = kEngineRunPlaying;

    if (m_pTeamA == nullptr || m_pTeamB == nullptr)
    {
        // STRING: HYPERX 0x004c84cc
        g_gameApp.Fatal("Home team or Away Team not Assigned!");
    }

    TickClock();

    for (nSlot = 0; nSlot < m_nObjectCount; nSlot++)
    {
        m_ppObjects[nSlot]->CompileAllPlans();

        m_ppObjects[nSlot]->SelectAndActivatePlan();
    }
}

// FUNCTION: HYPERX 0x00486860
void GameEngine::InitPlanSystems()
{
    BehaviorRegistry::Init();
    GameEngine::CreateAgendaItemPools();
    PlanVM::LoadAnimationLists();
    PlanVM::InitRuntimeOpcodeTable();
    BehaviorRegistry::InitAll();
    GameEngine::InitAgendaItemPools();
    PlanInteractorFactories::ResetAll();
}

// FUNCTION: HYPERX 0x00486890
void GameEngine::RegisterObject(int nSlot, EngineObject* pObject)
{
    m_nLoadedCount++;

    m_ppSlots[nSlot] = pObject;

    m_nSlotCount += pObject->GetComponentCount();
}

// FUNCTION: HYPERX 0x004868c0
void GameEngine::UnregisterObject(int nSlot)
{
    EngineObject** ppSlot  = &m_ppSlots[nSlot];
    auto* const    pObject = *ppSlot;

    m_nLoadedCount--;

    *ppSlot = nullptr;

    m_nSlotCount -= pObject->GetComponentCount();
}

// The first overlap found is processed and ends the walk.
// FUNCTION: HYPERX 0x004868f0
void GameEngine::DispatchCollisionToRule(CollInteractor* pChain)
{
    auto* pNode = pChain;

    while (pNode != nullptr)
    {
        if (pNode->GetActiveState() == 1 && pNode->GetActor()->GetPhase() == ObjectPhase::kLocal)
        {
            const auto nOtherSlot = pNode->GetOtherSlot();
            const auto nModelSlot = pNode->GetModelSlot();

            if (nOtherSlot >= 0 && nModelSlot >= 0)
            {
                auto* const pModelBubble = GetColObjPtr(nModelSlot);
                auto* const pOtherBubble = GetColObjPtr(nOtherSlot);

                if (pOtherBubble->SphereSphereOverlap(pModelBubble) != 0)
                {
                    short nOutcome = kDecisionLogFresh;

                    pNode->Process(0, &nOutcome, 0);

                    return;
                }
            }
        }

        // The chain is typed on the base both node kinds share; this list holds CollInteractors.
        pNode = (CollInteractor*)pNode->pNext;
    }
}

// FUNCTION: HYPERX 0x004869c0
void GameEngine::BroadcastCollisionEvents()
{
    short nIndex;

    EngineObject** ppObject = m_ppObjects;

    for (nIndex = 0; nIndex < m_nObjectCount; nIndex++)
    {
        DispatchCollisionToRule((*ppObject)->GetPlanState()->m_pCurrent->m_pCollInteractors);

        DispatchCollisionToRule((*ppObject)->GetPlanState()->m_pCollInteractors);

        ppObject++;
    }
}

// The sender is not told about its own event. A goal stops the clock, a shot restarts it and a
// start drag zeroes it.
// FUNCTION: HYPERX 0x00486a10
void GameEngine::BroadcastEvent(int nEvent, int nParam, int nSender)
{
    short       nIndex;
    static auto s_eventBase = Clock::Now();

    if (g_bDragOff == 0 || nEvent != kMsgStartDrag)
    {
        for (nIndex = 0; nIndex < m_nObjectCount; nIndex++)
        {
            if (m_ppObjects[nIndex]->GetSlot() != nSender)
            {
                m_ppObjects[nIndex]->PostEvent(nEvent, nParam, nSender);
            }
        }
    }

    if (nEvent == kMsgGoalScored)
    {
        s_eventBase = Clock::Now();

        m_clock.nPauseMark = 0;
    }
    else if (nEvent == kMsgStartDrag)
    {
        s_eventBase = Clock::Now();

        m_clock.periodStart = s_eventBase;

        m_clock.pauseStart = s_eventBase;

        m_clock.nPauseMark = 0;
    }
    else if (nEvent == kMsgShoot)
    {
        if (m_clock.nPauseMark == 0)
        {
            m_clock.pauseStart = Clock::Now();

            m_clock.periodStart += m_clock.pauseStart - s_eventBase;
        }

        m_clock.nPauseMark = 1;
    }
}

// Mode three restarts the match, two hands the main camera its view, one is the ordinary frame.

// FUNCTION: HYPERX 0x00486b20
void GameEngine::PerFrameUpdate()
{
    short nIndex;

    EngineObject** ppObject = m_ppObjects;

    for (nIndex = 0; nIndex < m_nObjectCount; nIndex++)
    {
        auto* const pObject = *ppObject;

        ppObject++;

        pObject->m_nFlags &= ~kEngineObjectPosed;
    }

    switch (m_wRunState)
    {
        case kEngineRunRestart:
            GameEngine::RestartMatch();

            GameEngine::TogglePause(1);

            return;

        case kEngineRunPaused:
            GetCamera(kCameraMainSlot)->ViewIt();

            break;

        case kEngineRunPlaying:
            TickClock();

            m_flGravityDt = g_flGravityGameEngine * m_clock.flDeltaSquared;

            ppObject = m_ppObjects;

            for (nIndex = 0; nIndex < m_nObjectCount; nIndex++)
            {
                (*ppObject)->GetPhysicsBody()->PerFrameTick();

                if (g_bSessionActive != 0)
                {
                    (*ppObject)->Postpass();
                }

                ppObject++;
            }

            GameEngine::BroadcastCollisionEvents();
            ProcessInputCommandQueue();
            GameEngine::ClearPendingEvents();

            if (g_pGameSession != nullptr)
            {
                if (g_pGameSession->PumpNetworkCommands() == 0)
                {
                    GameSession::Disconnect();
                }

                if (g_pGameSession != nullptr)
                {
                    GameEngine::SyncNetworkStateToObjects();
                }
            }

            break;

        default:
            break;
    }
}

// FUNCTION: HYPERX 0x00486c20
void GameEngine::SyncNetworkStateToObjects()
{
    short nIndex;

    EngineObject** ppObject = m_ppObjects;

    for (nIndex = 0; nIndex < m_nObjectCount; nIndex++)
    {
        const auto wFromSub = (*ppObject)->GetPlanState()->m_pCurrent->m_wSyncValue;

        auto* const pDetail = (*ppObject)->GetPlanState();

        auto* const pObject = *ppObject;

        ppObject++;

        pObject->m_wSyncedPlan      = pDetail->m_wSyncValue;
        pObject->m_wSyncedBehaviour = wFromSub;
    }
}

// FUNCTION: HYPERX 0x00486c70
char GameEngine::InsertObjectSorted(short nId)
{
    short i;
    short j;

    auto        nCount  = m_nObjectCount;
    auto* const pObject = m_ppSlots[nId];

    for (i = 0; i < nCount; i++)
    {
        if (m_ppObjects[i]->GetSlotWord() == nId)
        {
            return 0;
        }
    }

    const auto nClass = pObject->m_nKind;

    if (nClass == ObjectKind::kKeyboard || nClass == ObjectKind::kGamepad || nClass == ObjectKind::kJoystick || nClass == ObjectKind::kMouse)
    {
        for (j = nCount; j > 0; j--)
        {
            m_ppObjects[j] = m_ppObjects[j - 1];
        }

        m_ppObjects[0] = pObject;
    }
    else if (nClass == ObjectKind::kDromer && nCount > 5)
    {
        // The count is read again here: the search above ran against the old one.
        auto nSlot = pObject->GetSlot();
        nCount     = m_nObjectCount;
        nSlot++;

        for (j = nCount; j > nSlot; j--)
        {
            m_ppObjects[j] = m_ppObjects[j - 1];
        }

        m_ppObjects[nSlot] = pObject;
    }
    else
    {
        m_ppObjects[nCount] = pObject;
    }

    g_nSortedListTotal += pObject->m_wVisibleFaceTotal;
    m_nObjectCount++;

    return 1;
}

// FUNCTION: HYPERX 0x00486d60
char GameEngine::RemoveFromSortedList(short nId)
{
    short i;
    short nShift;

    for (i = 0; i < m_nObjectCount; i++)
    {
        auto* const pEntry = m_ppObjects[i];

        if (pEntry->GetSlotWord() == nId)
        {
            g_nSortedListTotal -= pEntry->m_wVisibleFaceTotal;

            for (nShift = i; nShift < m_nObjectCount - 1; nShift++)
            {
                m_ppObjects[nShift] = m_ppObjects[nShift + 1];
            }

            m_nObjectCount--;

            return 1;
        }
    }

    return 0;
}

// The second argument is never read.
// FUNCTION: HYPERX 0x00486de0
void GameEngine::SetRunState(short bRunning, int)
{
    static auto s_pauseBase = Clock::Now();

    switch (bRunning)
    {
        case 1:
            if (m_clock.nPauseMark == -1)
            {
                m_clock.nPauseMark = 1;

                m_clock.pauseStart = Clock::Now();

                m_clock.periodStart += m_clock.pauseStart - s_pauseBase;
            }

            m_wRunState = kEngineRunPlaying;

            SDL_SetCurrentThreadPriority(SDL_THREAD_PRIORITY_HIGH);

            SDL_DisableScreenSaver();
            break;

        case 0:
            s_pauseBase = Clock::Now();

            m_wRunState = kEngineRunPaused;

            SDL_SetCurrentThreadPriority(SDL_THREAD_PRIORITY_NORMAL);

            if (m_clock.nPauseMark == 1)
            {
                m_clock.nPauseMark = -1;
            }

            SDL_EnableScreenSaver();
            break;
    }
}

// A slot whose object is already released leaves the count wrong.
// FUNCTION: HYPERX 0x00486ee0
GameEngine::~GameEngine()
{
    for (short nSlot = 0; nSlot < kEngineSlotCount; nSlot++)
    {
        auto* const pObject = m_ppSlots[nSlot];

        if (pObject != nullptr)
        {
            delete pObject;

            m_nLoadedCount--;
        }
    }

    delete[] m_ppSlots;
    delete[] m_ppObjects;

    auto* pTeam = m_pTeamA;

    delete pTeam;

    pTeam = m_pTeamB;

    delete pTeam;

    auto* const pArena = m_pArena;

    if (pArena != nullptr)
    {
        delete[] pArena->m_pnSlotIds;
        delete pArena->m_pRokDef;
        delete pArena;
    }

    BehaviorRegistry::DestroyAll();
    GameEngine::DestroyAgendaItemPools();
    PlanInteractorFactories::DestroyAll();

    auto* const pSave = m_pSaveObject;

    delete pSave;
}

// FUNCTION: HYPERX 0x00486fb0
void GameEngine::ReplaceEntity(int nKind, ObjectDef* pDef)
{
    auto* const pObject = m_ppSlots[static_cast<short>(nKind)];

    UnregisterObject(nKind);

    pObject->ReInitModel(pObject->GetSlotWord(), pDef);

    RegisterObject(nKind, pObject);

    SyncRenderQueueSize();
}

// The tournament case clears the score block and both stat arrays.
// FUNCTION: HYPERX 0x00486ff0
void GameEngine::RestartMatch()
{
    short nIndex;
    int   nRow;
    int   nSlot;

    ClearPendingEvents();

    for (nIndex = 0; nIndex < m_nObjectCount; nIndex++)
    {
        m_ppObjects[nIndex]->SelectAndActivatePlan();
    }

    if (m_clock.nPeriod == kFirstPeriod)
    {
        Reset();

        // No active score line.
        g_wActiveScoreLine = -1;

        if (IsTournamentMode() != 0)
        {
            for (nRow = 0; nRow < kTeamRowCount; nRow++)
            {
                for (nSlot = 0; nSlot < kMatchStatCount; nSlot++)
                {
                    m_pTeamA->aStatRows[nRow].aStat[nSlot] = 0.0f;
                }
            }

            auto* const pStatsB = GetPlayer(2);
            auto* const pStatsA = GetPlayer(0);

            for (nSlot = 0; nSlot < kMatchStatCount; nSlot++)
            {
                pStatsA->m_matchStats.m_aStat[nSlot] = 0.0f;
                pStatsB->m_matchStats.m_aStat[nSlot] = 0.0f;
            }
        }
    }

    BeginMatchPlay();

    if (g_bSessionActive != 0)
    {
        SyncNetworkStateToObjects();
    }

    BroadcastEvent(kMsgStartDrag, 0, kSlotGod);
}

// A full queue keeps the five words it wrote and answers false.

// FUNCTION: HYPERX 0x004870b0
char GameEngine::PostSceneMessage(int nSlot, int nMessage, int nArg, int nSender, int bFromInput)
{
    m_aQueueSlot[m_nPendingCount]      = nSlot;
    m_aQueueMessage[m_nPendingCount]   = nMessage;
    m_aQueueArg[m_nPendingCount]       = nArg;
    m_aQueueSender[m_nPendingCount]    = nSender;
    m_aQueueFromInput[m_nPendingCount] = bFromInput;

    if (m_nPendingCount < kPendingQueueLength)
    {
        m_nPendingCount++;
        m_aQueueMessage[m_nPendingCount] = kMsgNomsg;

        return 1;
    }

    return 0;
}

// The lock is taken only around each pull; the pending list at the end runs unguarded.
// FUNCTION: HYPERX 0x00487140
void GameEngine::ProcessInputCommandQueue()
{
    short nPending;
    short nValue;
    short nKind;
    short nIndex;
    int   nCommand;
    int   nPayload;
    short nMessage;
    short nSender;
    short nLow;
    short nWantedOwn;
    short nWantedOther;

    for (nIndex = 0; nIndex < m_nObjectCount; nIndex++)
    {
        auto* const pObject = m_ppObjects[nIndex];
        auto* const pLock   = pObject->GetPhysicsBody()->pPlayer;

        if (pLock != nullptr)
        {
            auto* const pSection = &pLock->m_lock;

            pSection->lock();
            char bQueued = pObject->DequeueInputCommand(&nCommand, &nPayload);
            pSection->unlock();

            while (bQueued != 0)
            {
                if (nCommand == kCommandSetPlanAndBehavior)
                {
                    pObject->InstantiateAIPlan(PlanBehaviourWord::GetPlan(nPayload));
                    pObject->GetPlanState()->ActivateBehavior(PlanBehaviourWord::GetBehaviour(nPayload));
                }
                else if (nCommand == kCommandSetBehavior)
                {
                    pObject->GetPlanState()->ActivateBehavior(static_cast<short>(nPayload));
                }
                else if ((nCommand & kMsgEventCollisionMark) != 0)
                {
                    MsgEvent::Unpack(nCommand, &nKind, &nWantedOther, &nWantedOwn, &nLow);

                    auto* const pNode = pObject->GetPlanState()->WalkLinkedList(nKind);

                    if (pNode != nullptr && pNode->GetOtherSlot() == nWantedOther && pNode->GetModelSlot() == nWantedOwn)
                    {
                        pNode->Process(nPayload, &nLow, 1);
                    }
                }
                else
                {
                    MsgEvent::UnpackScene(nCommand, &nMessage, &nSender, &nValue);

                    pObject->DispatchSceneMessage(nMessage, nPayload, nValue, nSender, 1);
                }

                pSection->lock();
                bQueued = pObject->DequeueInputCommand(&nCommand,
                                                       &nPayload);
                pSection->unlock();
            }
        }
    }

    for (nPending = 0; nPending < m_nPendingCount; nPending++)
    {
        m_ppSlots[static_cast<short>(m_aQueueSlot[nPending])]
            ->DispatchSceneMessage(m_aQueueMessage[nPending], m_aQueueArg[nPending], kDecisionLogFresh, m_aQueueSender[nPending], m_aQueueFromInput[nPending]);
    }
}

// FUNCTION: HYPERX 0x00487340
void GameEngine::ClearPendingEvents()
{
    if (m_nPendingCount > g_nMaxPendingSeen)
    {
        g_nMaxPendingSeen = m_nPendingCount;
    }

    m_nPendingCount    = 0;
    m_aQueueMessage[0] = kMsgNomsg;
}

// FUNCTION: HYPERX 0x00487480
int GameEngine::GetArmoryCount() const
{
    return m_abTypeHighWater[kGOBArmory] - g_abObjectTypeBaseline[kGOBArmory];
}

// FUNCTION: HYPERX 0x00487490
int GameEngine::GetSwarfCount() const
{
    return m_abTypeHighWater[kGOBSwarf] - g_abObjectTypeBaseline[kGOBSwarf];
}

// FUNCTION: HYPERX 0x004874b0
int GameEngine::GetMineCount() const
{
    return m_abTypeHighWater[kGOBMine] - g_abObjectTypeBaseline[kGOBMine];
}

// FUNCTION: HYPERX 0x004874c0
int GameEngine::GetEntityCountOfType(int nType)
{
    if (nType >= 0 && nType < kGOBNumOfTypes)
    {
        return m_abTypeHighWater[nType] - g_abObjectTypeBaseline[nType];
    }

    return 0;
}

// FUNCTION: HYPERX 0x004874f0
int GameEngine::GetHomeScore() const
{
    return m_nScoreHome;
}

// FUNCTION: HYPERX 0x00487500
int GameEngine::GetAwayScore() const
{
    return m_nScoreAway;
}

// FUNCTION: HYPERX 0x00487510
void GameEngine::SetHomeScore(int nScoreHome)
{
    m_nScoreHome = nScoreHome;
}

// FUNCTION: HYPERX 0x00487520
void GameEngine::SetAwayScore(int nScoreAway)
{
    m_nScoreAway = nScoreAway;
}

// FUNCTION: HYPERX 0x00487530
int GameEngine::IncrementHomeScore()
{
    m_nScoreHome++;

    return m_nScoreHome;
}

// FUNCTION: HYPERX 0x00487540
int GameEngine::IncrementAwayScore()
{
    m_nScoreAway++;

    return m_nScoreAway;
}

// FUNCTION: HYPERX 0x00487550
void GameEngine::Reset()
{
    m_nScoreHome = -1;
    m_nScoreAway = -1;

    InfoBar::IncrementScore(5);
    InfoBar::IncrementScore(4);
}

// FUNCTION: HYPERX 0x00487580
void GameEngine::ResetPeriodToFirst()
{
    m_clock.nPeriod = 1;
}

constexpr auto kTeamCount = 2;

// Writes only in tournament player mode.
// FUNCTION: HYPERX 0x00487590
char GameEngine::WriteSaveFile(int nSlot, const char* pszLabel)
{
    char                 szName[kMaxPathLength];
    SaveBlock            block;
    TeamBlock*           pTeam;
    SaveTeam*            pDest;
    int                  nSeat;
    const TeamPlayerRow* pReserve;
    float*               pStat;
    PlayerObject*        pObject;
    int                  nTeam;
    int                  nRow;
    int                  nIndex;

    if (IsTournamentMode() == 0)
    {
        return 0;
    }

    OwnedFile file;

    // STRING: HYPERX 0x004c84f4
    sprintf(szName, "SAVE%02d.BIN", nSlot);

    PathString strPath(String::Concat(g_gameApp.m_strExeDir.m_str, szName).m_pchData, kMaxPathLength);

    if (file.m_file.Open(strPath.m_str.m_pchData, FileMode::kWrite) == 0)
    {
        return 0;
    }

    memset(&block, 0, sizeof(block));

    const auto nLength = static_cast<int>(strlen(pszLabel));

    block.nTag = kSaveBlockTagValue;

    if (nLength > 0)
    {
        memcpy(block.szLabel, pszLabel, nLength);
    }

    block.nBracketPos = m_pSaveObject->GetBracketPos();
    block.nDifficulty = static_cast<int>(m_nDifficulty);

    for (nTeam = 0; nTeam < kTeamCount; nTeam++)
    {
        if (nTeam == 0)
        {
            pTeam = m_pTeamA;
            pDest = &block.teamOne;

            pDest->nTeamIndex = static_cast<short>(m_nTeamHomeIndex);
        }
        else
        {
            pTeam = m_pTeamB;
            pDest = &block.teamTwo;

            pDest->nTeamIndex = static_cast<short>(m_nTeamAwayIndex);
        }

        for (nSeat = 0; nSeat < kTeamBlockSeats; nSeat++)
        {
            memmove(&pTeam->aStatRows[pTeam->aSeat[nSeat]],
                    &((PlayerObject*)pTeam->apOnIce[nSeat])->m_matchStats,
                    sizeof(TeamStatRow));
        }

        TeamStatRow* pSource = pTeam->aStatRows;
        TeamStatRow* pRow    = pDest->aStatRows;

        for (nRow = 0; nRow < kTeamRowCount; nRow++)
        {
            const float* pWalk = pSource->aStat;
            float*       pTo   = pRow->aStat;

            for (nIndex = 0; nIndex < kMatchStatCount; nIndex++)
            {
                *pTo = *pWalk;
                pTo++;
                pWalk++;
            }

            pSource++;
            pRow++;
        }

        for (nIndex = 0, pReserve = pTeam->aRows, pStat = pDest->aDamage;
             nIndex < kTeamRowCount;
             nIndex++, pStat++, pReserve++)
        {
            if (pTeam->aSeat[0] == nIndex)
            {
                pObject = g_pGameEngine->GetPlayer(0);

                *pStat = pObject->m_stats.GetScalar(kRatingDamage);
            }
            else if (pTeam->aSeat[1] == nIndex)
            {
                pObject = g_pGameEngine->GetPlayer(2);

                *pStat = pObject->m_stats.GetScalar(kRatingDamage);
            }
            else
            {
                *pStat = pReserve->aStat[kRatingDamage];
            }
        }

        pDest->aSeat[0] = pTeam->aSeat[0];
        pDest->aSeat[1] = pTeam->aSeat[1];
    }

    memcpy(block.abAward, m_abAward, sizeof(m_abAward));

    if (file.m_file.Write(reinterpret_cast<char*>(&block), sizeof(block)) == sizeof(block))
    {
        return 1;
    }

    return 0;
}

#define ARENA_NONE ((unsigned short)0xffff)

// A team's first seat is its slot; every other seat shares the slot two above it.
inline int GameEngine::SubstituteSlot(int nTeam, int nSeat)
{
    auto nSlot = nTeam;

    if (nSeat != 0)
    {
        nSlot += 2;
    }

    return nSlot;
}

// The teams are set first, because that builds the objects the rest writes into.
// FUNCTION: HYPERX 0x004878f0
char GameEngine::ReadSaveFile(int nSlot)
{
    SaveBlock      block;
    char           szName[kMaxPathLength];
    TeamBlock*     pTeam;
    SaveTeam*      pRecord;
    int            nSeat;
    TeamPlayerRow* pReserve;
    float*         pStat;
    int            nTeam;
    int            nRow;
    int            nIndex;
    int            nWord;
    unsigned int   nStat;

    OwnedFile file;

    // STRING: HYPERX 0x004c84f4
    sprintf(szName, "SAVE%02d.BIN", nSlot);

    PathString strPath(String::Concat(g_gameApp.m_strExeDir.m_str, szName).m_pchData, kMaxPathLength);

    if (file.m_file.Open(strPath.m_str.m_pchData, FileMode::kRead) == 0)
    {
        return 0;
    }

    if (file.m_file.Read(&block, sizeof(block)) != sizeof(block))
    {
        return 0;
    }

    if (block.nTag == kSaveBlockTagValue)
    {
        SetDifficultyLevel(block.nDifficulty);

        m_pSaveObject->SetBracketPos(static_cast<short>(block.nBracketPos));

        ClearPendingEvents();

        m_bMatchOver = 1;

        SetTeams(static_cast<short>(block.teamOne.nTeamIndex), static_cast<short>(block.teamTwo.nTeamIndex));

        m_nArenaIndex = ARENA_NONE;

        SetArena(static_cast<short>(block.teamTwo.nTeamIndex));

        for (short nObject = 0; nObject < m_nObjectCount; nObject++)
        {
            m_ppObjects[nObject]->SelectAndActivatePlan();
        }

        for (nIndex = 0; nIndex < kTeamBlockSeats; nIndex++)
        {
            if (m_pTeamA->aSeat[nIndex] != block.teamOne.aSeat[nIndex])
            {
                SubstitutePlayer(SubstituteSlot(0, nIndex), block.teamOne.aSeat[nIndex]);
            }

            if (m_pTeamB->aSeat[nIndex] != block.teamTwo.aSeat[nIndex])
            {
                SubstitutePlayer(SubstituteSlot(1, nIndex), block.teamTwo.aSeat[nIndex]);
            }
        }

        for (nTeam = 0; nTeam < kTeamCount; nTeam++)
        {
            if (nTeam == 0)
            {
                nWord   = block.teamOne.nTeamIndex;
                pTeam   = m_pTeamA;
                pRecord = &block.teamOne;

                g_nDromeTeamHome = nWord;

                m_pSaveObject->ValidateSetHomeTeam(static_cast<short>(nWord), 1);
            }
            else
            {
                nWord   = block.teamTwo.nTeamIndex;
                pTeam   = m_pTeamB;
                pRecord = &block.teamTwo;

                g_nDromeTeamAway = nWord;
                g_nDromeArena    = nWord;
            }

            TeamStatRow* pRow  = pRecord->aStatRows;
            TeamStatRow* pDest = pTeam->aStatRows;

            for (nRow = 0; nRow < kTeamRowCount; nRow++)
            {
                const float* pWalk = pRow->aStat;
                float*       pTo   = pDest->aStat;

                for (nIndex = 0; nIndex < kMatchStatCount; nIndex++)
                {
                    *pTo = *pWalk;
                    pTo++;
                    pWalk++;
                }

                pDest++;
                pRow++;
            }

            for (nSeat = 0; nSeat < kTeamBlockSeats; nSeat++)
            {
                memmove((void*)&((PlayerObject*)pTeam->apOnIce[nSeat])->m_matchStats,
                        &pTeam->aStatRows[pTeam->aSeat[nSeat]],
                        sizeof(TeamStatRow));
            }

            for (nStat = 0, pReserve = pTeam->aRows, pStat = pRecord->aDamage;
                 nStat < kTeamRowCount;
                 nStat++, pStat++, pReserve++)
            {
                if (static_cast<unsigned int>(pRecord->aSeat[0]) == nStat)
                {
                    ((PlayerObject*)pTeam->apOnIce[0])->m_stats.m_aStats[RATING_SLOT(kRatingDamage)] = *pStat;
                }
                else if (static_cast<unsigned int>(pRecord->aSeat[1]) == nStat)
                {
                    ((PlayerObject*)pTeam->apOnIce[1])->m_stats.m_aStats[RATING_SLOT(kRatingDamage)] = *pStat;
                }
                else
                {
                    pReserve->aStat[kRatingDamage] = *pStat;
                }
            }
        }

        memcpy(m_abAward, block.abAward, sizeof(m_abAward));

        return 1;
    }

    return 0;
}

// Always answers false; the caller can tell success only by the buffer.
// FUNCTION: HYPERX 0x00487cb0
char GameEngine::ReadSlotLabel(int nSlot, char* pszLabel, int nMax)
{
    char      szName[kMaxPathLength];
    SaveBlock block;
    int       nCopied;

    *pszLabel = 0;

    OwnedFile file;

    // STRING: HYPERX 0x004c84f4
    sprintf(szName, "SAVE%02d.BIN", nSlot);

    PathString strPath(String::Concat(g_gameApp.m_strExeDir.m_str, szName).m_pchData,
                       kMaxPathLength);

    if (file.m_file.Open(strPath.m_str.m_pchData, FileMode::kRead) != 0 && file.m_file.Read(&block, sizeof(block)) == sizeof(block) && block.nTag == kSaveBlockTagValue)
    {
        nMax--;

        for (nCopied = 0; nCopied < nMax; nCopied++)
        {
            const auto ch = block.szLabel[nCopied];

            pszLabel[nCopied] = ch;

            if (ch == 0)
            {
                break;
            }
        }

        pszLabel[nCopied] = 0;

        return 0;
    }

    return 0;
}

constexpr auto kSlotOrderCount = 4;

// FUNCTION: HYPERX 0x00487e50
DifficultyLevel GameEngine::GetDifficultyLevel() const
{
    return m_nDifficulty;
}

// kRatingDamage is never copied; a level above two answers no.
// FUNCTION: HYPERX 0x00487e60
char GameEngine::SetDifficultyLevel(unsigned int nLevel)
{
    TeamBlock*    pTeam;
    ObjectDef*    pRecord;
    const float*  pStats;
    PlayerObject* pObject;
    int           nTeam;
    int           nOrder;
    int           nIndex;

    if (nLevel > static_cast<unsigned int>(DifficultyLevel::kMax))
    {
        return 0;
    }

    if (m_nDifficulty == static_cast<DifficultyLevel>(nLevel))
    {
        return 1;
    }

    g_pGameOptions->WriteDifficulty(nLevel);

    for (nTeam = 0; nTeam < kTeamCount; nTeam++)
    {
        if (nTeam == 0)
        {
            pTeam = m_pTeamA;
        }
        else
        {
            pTeam = m_pTeamB;
        }

        for (nOrder = 0; nOrder < kSlotOrderCount; nOrder++)
        {
            pRecord = pTeam->apDefs[g_anTeamSlotOrder[nOrder]];

            if (pRecord != nullptr)
            {
                pStats = pRecord->m_apRatings[nLevel];

                if (pStats != nullptr)
                {
                    if (nOrder < kTeamBlockSeats && pTeam->aSeat[nOrder] == g_anTeamSlotOrder[nOrder])
                    {
                        pObject = (PlayerObject*)pTeam->apOnIce[nOrder];

                        pObject->m_stats.CopyBaseStats();

                        for (nIndex = 0; nIndex < kRatingCount; nIndex++)
                        {
                            if (nIndex != kRatingDamage)
                            {
                                pObject->m_stats.m_aStats[RATING_SLOT(nIndex)] = pStats[nIndex];
                            }
                        }

                        pObject->m_stats.ScaleTo(pObject->GetSlot());
                    }
                    else
                    {
                        for (nIndex = 0; nIndex < kRatingCount; nIndex++)
                        {
                            if (nIndex != kRatingDamage)
                            {
                                pTeam->aRows[nOrder].aStat[nIndex] = pStats[nIndex];
                            }
                        }
                    }
                }
            }
        }
    }

    if (nLevel == 0 && g_bSessionActive == 0)
    {
        pTeam = m_pTeamA;

        pRecord = pTeam->apDefs[2];

        if (pRecord != nullptr)
        {
            pStats = pRecord->m_apRatings[kRatingsSpecialGoalie];

            if (pStats != nullptr)
            {
                pObject = (PlayerObject*)pTeam->apOnIce[2];

                for (nIndex = 0; nIndex < kGoalieRatingCount; nIndex++)
                {
                    pObject->m_stats.m_aStats[RATING_SLOT(nIndex)] = pStats[nIndex];
                }
            }
        }
    }

    m_nDifficulty = static_cast<DifficultyLevel>(nLevel);

    return 1;
}

// FUNCTION: HYPERX 0x00488010
unsigned char GameEngine::IsTournamentMode()
{
    return g_nPlayerMode == kPlayerModeTournament;
}

// FUNCTION: HYPERX 0x00488020
unsigned char GameEngine::IsExhibitionMode()
{
    return g_nPlayerMode == kPlayerModeExhibition;
}

// The smallest drawn area is half the buffer, and each step adds a twelfth.
constexpr auto kScreenSizeMax     = 6;
constexpr auto kScreenTwelfthsMin = 6;

// The drawn area is (step + 6) twelfths of the buffer, centred.
// FUNCTION: HYPERX 0x00488030
void GameEngine::SetScreenSize(int nStep)
{
    LayoutState state;

    if (nStep < 0)
    {
        nStep = 0;
    }
    else if (kScreenSizeMax < nStep)
    {
        nStep = kScreenSizeMax;
    }

    if (nStep != m_nScreenSize)
    {
        m_nScreenSize = nStep;

        g_pGameOptions->WriteScreenSize(nStep);

        g_pScene = nullptr;

        const auto nBufferWidth = g_nBackBufferWidth;

        const auto nTwelfths = nStep + kScreenTwelfthsMin;

        const auto nWidth  = nBufferWidth * nTwelfths / 12;
        const auto nHeight = g_nBackBufferHeight * nTwelfths / 12;

        auto* const pCamera = GetCamera(kCameraMainSlot);

        if (pCamera != nullptr)
        {
            const auto nMarginX = (nBufferWidth - nWidth) / 2;
            const auto nMarginY = (g_nBackBufferHeight - nHeight) / 2;

            state = Widget::InitLayoutState(g_pBackBufferBitsCopy, nBufferWidth, g_nBackBufferHeight, nBufferWidth, nWidth, nHeight, nMarginX, nMarginY);

            pCamera->layout = state;

            pCamera->InitViewTransforms();
        }
    }
}

// FUNCTION: HYPERX 0x00488130
void GameEngine::SetBrutality(int nValue)
{
    if (nValue != m_nBrutality)
    {
        m_nBrutality = nValue;

        g_pGameOptions->WriteBrutality(nValue);
    }
}

// FUNCTION: HYPERX 0x00488160
void GameEngine::SetPeriodLengthMs(int nValue)
{
    if (Clock::Milliseconds{ static_cast<std::uint32_t>(nValue) } != m_periodLength)
    {
        m_periodLength = Clock::Milliseconds{ static_cast<std::uint32_t>(nValue) };

        g_pGameOptions->WritePeriodLength(nValue);
    }
}

// FUNCTION: HYPERX 0x00488190
void GameEngine::SetAward(int nSlot, char cValue)
{
    m_abAward[nSlot] = cValue;
}

// FUNCTION: HYPERX 0x004881b0
void GameEngine::ClearAwards()
{
    int nSlot;

    for (nSlot = 0; nSlot < 6; nSlot++)
    {
        SetAward(nSlot, 0);
    }
}

// FUNCTION: HYPERX 0x004881d0
char GameEngine::HasAward(int nSlot) const
{
    return m_abAward[nSlot];
}

// FUNCTION: HYPERX 0x004881e0
char GameEngine::EvalArcherAward()
{
    int nRow;

    auto flTotal = 0.0f;

    char bEligible = 1;
    auto nCounted  = 0;

    for (nRow = 0; nRow < kTeamRowCount; nRow++)
    {
        const TeamStatRow* const pRow = &g_pGameEngine->m_pTeamA->aStatRows[nRow];

        if (pRow->aStat[kMatchStatShotsOnGoal] > 0.0f)
        {
            flTotal += pRow->aStat[kMatchStatShootingPct];
            nCounted++;
        }
    }

    if (flTotal / nCounted < STAT_MINIMUM)
    {
        bEligible = 0;
    }

    if (HasAward(kAwardArcher) == 0)
    {
        SetAward(kAwardArcher, bEligible);
    }

    return bEligible;
}

// FUNCTION: HYPERX 0x00488270
char GameEngine::EvalAnvilAward()
{
    int nRemaining;

    char bEligible = 1;

    for (nRemaining = 0; nRemaining < kTeamRowCount; nRemaining++)
    {
        if (g_pGameEngine->m_pTeamB->aRows[nRemaining].aStat[kRatingDamage] != 0.0f)
        {
            bEligible = 0;
        }
    }

    if (HasAward(kAwardAnvil) == 0)
    {
        SetAward(kAwardAnvil, bEligible);
    }

    return bEligible;
}

// The particle systems start sixty slots in; a count of one or less finds nothing.
// FUNCTION: HYPERX 0x004882d0
ParticleSystem* GameEngine::FindFreeParticleSystem()
{
    int iSlot;

    const auto nCount = GetEntityCountOfType(kGOBExplosion);

    for (iSlot = 1; iSlot < nCount; iSlot++)
    {
        auto* const pEntity = GetParticleSystem(kSlotExplosion + iSlot);

        if (pEntity != nullptr && pEntity->m_bAnyAlive == 0)
        {
            return pEntity;
        }
    }

    return nullptr;
}

// FUNCTION: HYPERX 0x00488310
void GameEngine::DeactivateParticleSystems()
{
    const auto nCount = GetEntityCountOfType(kGOBExplosion);
    int        nIndex;

    for (nIndex = 0; nIndex < nCount; nIndex++)
    {
        auto* const pSystem = GetParticleSystem(kSlotExplosion + nIndex);

        if (pSystem != nullptr && pSystem->m_bAnyAlive != 0)
        {
            pSystem->Deactivate();
        }
    }
}

// FUNCTION: HYPERX 0x00488350
void GameEngine::SetAttackMode(int nOn)
{
    const int nFlag = nOn != 0;

    m_nAttackMode = nFlag;

    g_pGameOptions->WriteInt(g_pszOptionsSection, g_pszAttackModeKey, nFlag);
}

// FUNCTION: HYPERX 0x00488380
void GameEngine::SetTeammateMode(int nOn)
{
    const int nFlag = nOn != 0;

    m_nTeammateMode = nFlag;

    g_pGameOptions->WriteInt(g_pszOptionsSection, g_pszTeammateModeKey, nFlag);
}

// FUNCTION: HYPERX 0x004883b0
ReserveRows* GameEngine::GetReserveRows(const TeamBlock* pTeam)
{
    return &g_aFormationTable[pTeam->aSeat[1] + pTeam->aSeat[0] * 4];
}

unsigned char g_abObjectTypeBaseline[32] = {
    102, 0, 6, 10, 12, 7, 8, 9, 25, 26, 33, 27, 46, 72, 80, 23, 24, 40, 66, 56, 60, 52, 86, 87, 89, 91, 58, 54, 55, 92, 94, 102
};

unsigned char g_abObjectTypeLimit[32] = {
    102, 5, 6, 10, 18, 7, 8, 9, 25, 26, 39, 32, 51, 79, 85, 23, 24, 45, 71, 57, 65, 53, 86, 88, 90, 91, 59, 54, 55, 93, 101, 102
};

// The body-part model ids, id_MOD_BodyPart0 to 7: home four, then away four.
int g_anBodyPartModelIds[8] = { 94, 95, 96, 97, 98, 99, 100, 101 };

int g_anTeamSlotOrder[4] = { 0, 1, 3, 4 };
