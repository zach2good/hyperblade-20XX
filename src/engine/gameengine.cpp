#include "common/compat.h"
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
#include "net/netglobals.h"
#include "planvm/planopcodetable.h"
#include "saveblock.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <windows.h>
#include <mmsystem.h>
#include <new.h>
#include <stdio.h>
#include <mbstring.h>
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

#define TEAM_KEYWORD_COUNT 9

// FUNCTION: HYPERX 0x0040b970
void __cdecl GameEngine::InitAgendaItemPools(void)
{
    int nSlot;

    PooledOwnerRoot** ppEntry = g_apAgendaItemPools;

    for (nSlot = 0; nSlot < Agenda_Count; nSlot++)
    {
        if (*ppEntry != 0)
        {
            (*ppEntry)->Init();
        }

        ppEntry++;
    }
}

// The statement order is the original's: the first pool built goes into slot eleven.
// FUNCTION: HYPERX 0x0040bbf0
void __cdecl GameEngine::CreateAgendaItemPools(void)
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
    g_apAgendaItemPools[Agenda_Count]                 = 0;
}

// Runs each pool's Destroy slot and stores its answer back, stopping at the first null.

// FUNCTION: HYPERX 0x0040c070
void __cdecl GameEngine::DestroyAgendaItemPools(void)
{
    short iSlot = 0;

    PooledOwnerRoot* pNode = g_apAgendaItemPools[0];

    while (pNode != 0)
    {
        g_apAgendaItemPools[iSlot] = pNode->Destroy();

        iSlot++;

        pNode = g_apAgendaItemPools[iSlot];
    }
}

// Colours the multiplier's two materials by the rok's value; the first call saves their rest colours.

// FUNCTION: HYPERX 0x00463220
void __cdecl GameEngine::ShowRokValue(short nRokValue)
{
    EngineObject* const pObject = g_pGameEngine->m_ppSlots[SLOT_MULTIPLIER];

    if (pObject == 0)
    {
        return;
    }

    MeshMaterial* const pMaterials = pObject->m_pModelBlock->m_pMaterials;

    if (g_bMultiplierColoursUnsaved != 0)
    {
        g_wMultiplierRestColour1    = (short)pMaterials[1].nColour;
        g_wMultiplierRestColour2    = (short)pMaterials[2].nColour;
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
int __cdecl GameEngine::CountEntitiesInRange(short nWhich)
{
    int           iEntity;
    EngineObject* pSelf;
    EngineObject* pOther;
    const Vec3*   pSelfPos;
    const Vec3*   pOtherPos;
    float         flDx;
    float         flDy;
    float         flDz;

    const int nFirst  = g_pGameEngine->GetMineCount();
    const int nSecond = g_pGameEngine->GetSwarfCount();

    int nInRange = 0;

    for (iEntity = 0; iEntity < nFirst; iEntity++)
    {
        pSelf  = g_pGameEngine->m_ppSlots[nWhich];
        pOther = g_pGameEngine->m_ppSlots[(short)(iEntity + SLOT_MINE)];

        pSelfPos  = pSelf->GetPositionPtr();
        pOtherPos = pOther->GetPositionPtr();

        flDx = pOtherPos->x - pSelfPos->x;
        flDy = pOtherPos->y - pSelfPos->y;
        flDz = pOtherPos->z - pSelfPos->z;

        if ((float)sqrt(flDz * flDz + flDy * flDy + flDx * flDx) <= 10.0f)
        {
            nInRange++;
        }
    }

    for (iEntity = 0; iEntity < nSecond; iEntity++)
    {
        pSelf  = g_pGameEngine->m_ppSlots[nWhich];
        pOther = g_pGameEngine->m_ppSlots[(short)(iEntity + SLOT_SWARF)];

        pSelfPos  = pSelf->GetPositionPtr();
        pOtherPos = pOther->GetPositionPtr();

        flDx = pOtherPos->x - pSelfPos->x;
        flDy = pOtherPos->y - pSelfPos->y;
        flDz = pOtherPos->z - pSelfPos->z;

        if ((float)sqrt(flDz * flDz + flDy * flDy + flDx * flDx) <= 10.0f)
        {
            nInRange++;
        }
    }

    return nInRange;
}

// The resting byte of four of the five queue tables, repeated over each table.
#define QUEUE_TARGET_BYTE 0x8e
#define QUEUE_IDLE_BYTE   0x66
#define QUEUE_FLAG_BYTE   0x02

#define HIDDEN_TEAM_A 8
#define HIDDEN_TEAM_B 2

// FUNCTION: HYPERX 0x00469380
int __cdecl GameEngine::IsTeamWinning(unsigned char nTeam)
{
    if ((nTeam & 1) != 0)
    {
        return g_pGameEngine->GetAwayScore() > g_pGameEngine->GetHomeScore();
    }

    return g_pGameEngine->GetHomeScore() > g_pGameEngine->GetAwayScore();
}

// FUNCTION: HYPERX 0x0047acf0
unsigned short __fastcall GameEngine::TogglePause(HWND hWnd, int bRestyle)
{
    if (g_pGameEngine == 0)
    {
        return 1;
    }

    if (g_pGameEngine->m_wRunState == ENGINE_RUN_PLAYING)
    {
        g_pGameEngine->SetRunState(0, 0);

        g_nSavedCursorState = GameApp::SetCursorVisible(1);

        if (bRestyle != 0)
        {
            Window::ApplyFullscreenStyle(hWnd, 1);
        }
    }
    else
    {
        g_pGameEngine->SetRunState(1, 0);

        GameApp::SetCursorVisible(g_nSavedCursorState);

        if (bRestyle != 0)
        {
            Window::ApplyFullscreenStyle(hWnd, 0);
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
    m_ppSlots          = 0;
    m_ppObjects        = 0;
    memset(m_apCameras, 0, sizeof(m_apCameras));

    m_wCameraCount = 0;

    memset(m_abTypeHighWater, 0, sizeof(m_abTypeHighWater));

    m_nViewedSlot = 0;

    memset(m_ppColObjects, 0, sizeof(m_ppColObjects));

    m_wRunState     = ENGINE_RUN_STOPPED;
    m_nPendingCount = 0;

    memset(m_aQueueMessage, QUEUE_TARGET_BYTE, sizeof(m_aQueueMessage));
    memset(m_aQueueSlot, QUEUE_IDLE_BYTE, sizeof(m_aQueueSlot));
    memset(m_aQueueArg, QUEUE_IDLE_BYTE, sizeof(m_aQueueArg));
    memset(m_aQueueFromInput, QUEUE_FLAG_BYTE, sizeof(m_aQueueFromInput));

    memset(&m_clock, 0, sizeof(m_clock));

    m_flGravityDt     = g_flZeroGameEngine;
    m_flPrevGravityDt = g_flZeroGameEngine;

    m_wUnusedB     = 0;
    m_wUnusedC     = 0;
    m_wUnusedD     = 0;
    m_wDeviceCount = 0;

    m_apTracked[0] = 0;
    m_apTracked[1] = 0;
    m_pTeamA       = 0;
    m_pTeamB       = 0;
    m_pArena       = 0;

    m_nTeamHomeIndex = 5;
    m_nTeamAwayIndex = 10;
    m_nArenaIndex    = 1;

    m_nPending = 0;

    TeamsBlock* const pTeams = new TeamsBlock;

    m_pTeamTable = pTeams;

    SaveObject* const pSave = new SaveObject(HIDDEN_TEAM_A, HIDDEN_TEAM_B);

    m_pSaveObject = pSave;

    // STRING: HYPERX 0x004c7268
    pSave->ParseGauntletConfig("gauntlet.cfg");

    m_nScoreHome = -1;
    m_nScoreAway = -1;

    m_nDifficulty     = g_pGameOptions->ReadDifficulty();
    m_nBrutality      = g_pGameOptions->ReadBrutality();
    m_nPeriodLengthMs = g_pGameOptions->ReadPeriodLength();

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

#define AWARD_AGGREGATE 0
#define AWARD_ANVIL     1
#define AWARD_CRUSH     2
#define AWARD_ARCHER    3
#define AWARD_SUPER     4
#define AWARD_RAZOR     5

#define AWARD_JUDGED_FIRST 1
#define AWARD_JUDGED       4

// The home score that wins the Super award.
#define SUPER_AWARD_SCORE 0x1e

#define BRACKET_END (-1)

// A drawn score with any away row still damaged restarts the bracket; awards nought and five
// are written and never read.
// FUNCTION: HYPERX 0x00480fc0
void GameEngine::AdvanceTournamentBracket()
{
    int aAwardReady[AWARD_COUNT];
    int nIndex;
    int nRemaining;
    int nEntry;

    memset(aAwardReady, 0, sizeof(aAwardReady));

    SyncPlayerDataToTeam();

    if (m_nScoreHome == m_nScoreAway && m_nScoreHome != 0)
    {
        for (nIndex = 0; nIndex < TEAM_ROW_COUNT; nIndex++)
        {
            if (m_pTeamB->aRows[nIndex].aStat[RATING_DAMAGE] > 0.0f)
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

    if (nEntry == BRACKET_END)
    {
        g_pGameEngine->SetAward(AWARD_RAZOR, 1);
    }

    if (m_nScoreHome >= SUPER_AWARD_SCORE)
    {
        SetAward(AWARD_SUPER, 1);

        aAwardReady[AWARD_SUPER] = 1;
    }

    if (m_nScoreAway == 0)
    {
        SetAward(AWARD_CRUSH, 1);

        aAwardReady[AWARD_CRUSH] = 1;
    }

    aAwardReady[AWARD_ARCHER] = (unsigned char)EvalArcherAward();
    aAwardReady[AWARD_ANVIL]  = (unsigned char)EvalAnvilAward();

    char bAllReady = 1;

    int* pReady = &aAwardReady[AWARD_JUDGED_FIRST];

    for (nRemaining = 0; nRemaining < AWARD_JUDGED; nRemaining++)
    {
        if (*pReady == 0)
        {
            bAllReady = 0;
        }

        pReady++;
    }

    if (HasAward(AWARD_AGGREGATE) == 0)
    {
        SetAward(AWARD_AGGREGATE, bAllReady);
    }
}

#define MAX_FRAME_TIME_MS 200.0f
#define MS_TO_SEC         0.001f

#define COUNTDOWN_START_MS 10000
#define COUNTDOWN_SECS     9

#define FINAL_PERIOD_INDEX 3
#define PERIOD_COUNT       4

#define OVERTIME_EARLY_MS 10000
#define OVERTIME_FINAL_MS 30000

// The frame delta, in milliseconds before the scaling and in seconds after it.
extern float g_flFrameDelta;

// Tells the second countdown test the first one has already armed.
extern unsigned char g_bPeriodCountdownActive;

// The five-slot ring holds the mean.
// FUNCTION: HYPERX 0x00481130
void GameEngine::TickClock()
{
    int          nOvertimeMs;
    unsigned int nElapsedMs;

    m_clock.flPrevDelta           = m_clock.flDelta;
    m_clock.flPrevDeltaSquared    = m_clock.flDeltaSquared;
    m_clock.flPrevInvDelta        = m_clock.flInvDelta;
    m_clock.flPrevInvDeltaSquared = m_clock.flInvDeltaSquared;
    m_clock.nPrevTickMs           = m_clock.nTickMs;

    const DWORD dwNow = (DWORD)SDL_GetTicks();
    m_clock.nTickMs   = dwNow;

    // The simulation steps a fixed tick, however long the frame took to draw.
    g_flFrameDelta = FramePacing::TICK_MS;

    g_flFrameDelta = g_flFifthGameEngine * g_flFrameDelta * MS_TO_SEC;

    const short nSlot      = m_clock.nRingIndex;
    m_clock.flDelta        = m_clock.flDelta - m_clock.aflDeltaRing[nSlot] + g_flFrameDelta;
    m_clock.flDeltaSquared = m_clock.flDelta * m_clock.flDelta;

    m_clock.flInvDelta        = g_flOneGameEngine / m_clock.flDelta;
    m_clock.flInvDeltaSquared = m_clock.flInvDelta * m_clock.flInvDelta;

    m_clock.aflDeltaRing[nSlot] = g_flFrameDelta;
    m_clock.nRingIndex          = (short)((short)(m_clock.nRingIndex + 1) % CLOCK_RING_LENGTH);

    if (m_clock.nPeriod >= FINAL_PERIOD_INDEX)
    {
        nOvertimeMs = OVERTIME_FINAL_MS;
    }
    else
    {
        nOvertimeMs = OVERTIME_EARLY_MS;
    }

    if (m_clock.nPauseMark != 1)
    {
        if (m_clock.nTickMs - m_clock.nPeriodStartMs < (unsigned int)m_nPeriodLengthMs && m_bEndPeriodRequested == 0 && AreAllPlayersInjured() == 0 && IsHomeTeamInjured() == 0 && IsAwayTeamInjured() == 0)
        {
            return;
        }
    }

    if (m_clock.nPauseMark == 1)
    {
        m_clock.nPauseStartMs = m_clock.nTickMs;
        nElapsedMs            = m_clock.nTickMs - m_clock.nPeriodStartMs;
    }
    else
    {
        nElapsedMs = m_clock.nPauseStartMs - m_clock.nPeriodStartMs;
    }

    if (g_bPeriodCountdownActive == 0)
    {
        if (m_nPeriodLengthMs - nElapsedMs <= COUNTDOWN_START_MS)
        {
            g_bPeriodCountdownActive = 1;
            g_nCountdownLastSecs     = COUNTDOWN_SECS;
            g_pAudioMgr->PlaySpecific(SOUND_ENDSIR);
        }
    }

    if (g_bPeriodCountdownActive != 0)
    {
        const int nSecs = (int)(m_nPeriodLengthMs - nElapsedMs) / 1000;

        if (nSecs < g_nCountdownLastSecs)
        {
            g_nCountdownLastSecs = nSecs;
            g_pAudioMgr->PlaySpecific(SOUND_ENDSIR);
        }
    }

    if (nElapsedMs < (unsigned int)m_nPeriodLengthMs || g_bDragOff != 0)
    {
        if (m_clock.nPauseMark == 0 && (IsHomeTeamInjured() != 0 || IsAwayTeamInjured() != 0))
        {
            m_bEndPeriodRequested = 1;
        }

        if (AreAllPlayersInjured() != 0 || m_bEndPeriodRequested != 0)
        {
            m_clock.nPeriodStartMs = m_clock.nPauseStartMs - m_nPeriodLengthMs;

            if (IsHomeTeamDead() != 0 || IsAwayTeamDead() != 0)
            {
                m_clock.nPeriod = FINAL_PERIOD_INDEX;
            }

            m_clock.nPauseMark = 1;
        }
    }
    else if (m_bPeriodEnded == 0)
    {
        if (m_clock.nPeriod == FINAL_PERIOD_INDEX)
        {
            BroadcastEvent(MSG_END_OF_GAME, 0, SLOT_GOD);
        }
        else
        {
            BroadcastEvent(MSG_END_OF_PERIOD, 0, SLOT_GOD);
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
            g_wActiveScoreLine = (short)(GetHomeScore() <= GetAwayScore());
        }

        m_clock.nPauseMark = 0;
        m_bPeriodEnded     = 1;
    }

    if (g_nPlayerMode == PLAYER_MODE_EXHIBITION && (unsigned int)(m_clock.nTickMs - m_clock.nPeriodStartMs) >= (unsigned int)(m_nPeriodLengthMs + nOvertimeMs) && nElapsedMs >= (unsigned int)m_nPeriodLengthMs && g_bDragOff == 0)
    {
        m_clock.nPeriod++;

        if (m_clock.nPeriod == PERIOD_COUNT)
        {
            m_bMatchOver = 1;

            if (g_bSessionActive != 0 && g_pGameSession != 0)
            {
                GameSession::Disconnect();
            }
        }

        g_gameApp.m_nEngineState = ESTATE_JUMBOTRON;
        g_nPrevEngineState       = ESTATE_MATCH_LIVE;
    }

    if (IsTournamentMode() != 0 && (unsigned int)(m_clock.nTickMs - m_clock.nPeriodStartMs) >= (unsigned int)(m_nPeriodLengthMs + nOvertimeMs) && nElapsedMs >= (unsigned int)m_nPeriodLengthMs && g_bDragOff == 0)
    {
        m_clock.nPeriod++;

        if (m_clock.nPeriod == PERIOD_COUNT)
        {
            AdvanceTournamentBracket();
        }

        g_gameApp.m_nEngineState = ESTATE_JUMBOTRON;
        g_nPrevEngineState       = ESTATE_MATCH_LIVE;
        GameEngine::TogglePause(g_hWndGame, 0);
    }
}

// FUNCTION: HYPERX 0x00481590
char GameEngine::AreAllPlayersInjured()
{
    float flLimit;
    int   i;

    char bAll = 1;

    for (i = 0; i < PLAYER_SLOT_COUNT; i++)
    {
        flLimit = DamageGate();

        PlayerObject* const pPlayer = GetPlayer((short)i);

        if (pPlayer->m_stats.GetScalar(RATING_DAMAGE) > flLimit)
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

    for (i = 0; i < TEAM_ROW_COUNT; i++)
    {
        if (m_pTeamA->aSeat[0] == i)
        {
            float         flSeat0Gate;
            PlayerObject* pSeat0;

            flSeat0Gate = DamageGate();

            pSeat0 = GetPlayer(0);

            if (pSeat0->m_stats.GetScalar(RATING_DAMAGE) > flSeat0Gate)
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

            if (pSeat1->m_stats.GetScalar(RATING_DAMAGE) > flSeat1Gate)
            {
                bBelow = 0;
                break;
            }
        }
        else
        {
            if (DamageGate() < m_pTeamA->aRows[i].aStat[RATING_DAMAGE])
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

    for (i = 0; i < TEAM_ROW_COUNT; i++)
    {
        if (m_pTeamA->aSeat[0] == i)
        {
            PlayerObject* pSeat0;

            pSeat0 = GetPlayer(0);

            if (pSeat0->m_stats.GetScalar(RATING_DAMAGE) > g_flZeroGameEngine)
            {
                bBelow = 0;
                break;
            }
        }
        else if (m_pTeamA->aSeat[1] == i)
        {
            PlayerObject* pSeat1;

            pSeat1 = GetPlayer(2);

            if (pSeat1->m_stats.GetScalar(RATING_DAMAGE) > g_flZeroGameEngine)
            {
                bBelow = 0;
                break;
            }
        }
        else
        {
            if (m_pTeamA->aRows[i].aStat[RATING_DAMAGE] > g_flZeroGameEngine)
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

    for (i = 0; i < TEAM_ROW_COUNT; i++)
    {
        if (m_pTeamB->aSeat[0] == i)
        {
            float         flSeat0Gate;
            PlayerObject* pSeat0;

            flSeat0Gate = DamageGate();

            pSeat0 = GetPlayer(1);

            if (pSeat0->m_stats.GetScalar(RATING_DAMAGE) > flSeat0Gate)
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

            if (pSeat1->m_stats.GetScalar(RATING_DAMAGE) > flSeat1Gate)
            {
                bBelow = 0;
                break;
            }
        }
        else
        {
            if (DamageGate() < m_pTeamB->aRows[i].aStat[RATING_DAMAGE])
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

#define BEHAVIOR_IDLE 0
// The Rok value that leaves the multiplier its rest colours.
#define ROK_VALUE_PLAIN 0

#define FIRST_PERIOD 1

// Rows of the message queue's text table, the lines they go on, and how long they stay.
#define TEXT_EXHIBITION_MODE 0x58
#define TEXT_GAUNTLET_MODE   0x59
#define TEXT_DIFFICULTY_BASE 0x5a
#define TEXT_LINE_MAIN       1
#define TEXT_LINE_NAMES      2
#define TEXT_DURATION        0x640

// FUNCTION: HYPERX 0x004819a0
unsigned char GameEngine::IsAwayTeamDead()
{
    char bBelow = 1;
    int  i;

    for (i = 0; i < TEAM_ROW_COUNT; i++)
    {
        if (m_pTeamB->aSeat[0] == i)
        {
            PlayerObject* pSeat0;

            pSeat0 = GetPlayer(1);

            if (pSeat0->m_stats.GetScalar(RATING_DAMAGE) > g_flZeroGameEngine)
            {
                bBelow = 0;
                break;
            }
        }
        else if (m_pTeamB->aSeat[1] == i)
        {
            PlayerObject* pSeat1;

            pSeat1 = GetPlayer(3);

            if (pSeat1->m_stats.GetScalar(RATING_DAMAGE) > g_flZeroGameEngine)
            {
                bBelow = 0;
                break;
            }
        }
        else
        {
            if (m_pTeamB->aRows[i].aStat[RATING_DAMAGE] > g_flZeroGameEngine)
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
    int              nOffset;

    m_bEndPeriodRequested    = 0;
    m_bPeriodEnded           = 0;
    m_bFollowingOther        = 0;
    g_bPeriodCountdownActive = 0;

    RokObject* const pRok = (RokObject*)m_ppSlots[SLOT_ROK];

    if (pRok != 0)
    {
        pRok->m_wRokValue = 0;
        pRok->SetRenderSlotFlags();
        pRok->ActivateAtSpawnSlot(SPAWN_SLOT_ANY);
        pRok->GetPhysicsBody()->pAttach = 0;
    }

    if (m_ppSlots[SLOT_RED_RANDOMIZER] != 0)
    {
        m_ppSlots[SLOT_RED_RANDOMIZER]->GetPlanState()->ActivateBehavior(BEHAVIOR_IDLE);
    }

    if (m_ppSlots[SLOT_MINE] != 0)
    {
        m_ppSlots[SLOT_MINE]->GetPhysicsBody()->pAttach = 0;
        m_ppSlots[SLOT_MINE]->GetPlanState()->ActivateBehavior(BEHAVIOR_IDLE);
    }

    if (m_ppSlots[SLOT_SWARF] != 0)
    {
        m_ppSlots[SLOT_SWARF]->GetPhysicsBody()->pAttach = 0;
        m_ppSlots[SLOT_SWARF]->GetPlanState()->ActivateBehavior(BEHAVIOR_IDLE);
    }

    GameEngine::ShowRokValue(ROK_VALUE_PLAIN);

    m_clock.nPeriodStartMs = m_clock.nTickMs;

    for (nIndex = 0; nIndex < m_nObjectCount; nIndex++)
    {
        if (m_ppObjects[nIndex]->m_nKind == KIND_DROMER)
        {
            ObjectStats* const pStats = &((PlayerObject*)m_ppObjects[nIndex])->m_stats;
            pStats->m_aStats[0]       = FULL_WEIGHT;

            if (pStats->GetScalar(RATING_DAMAGE) > g_flZeroGameEngine)
            {
                pStats->m_aStats[RATING_SLOT(RATING_DAMAGE)] = FULL_WEIGHT;
            }
        }
    }

    // Nothing reads nOffset.
    nOffset = 0;

    for (nRow = 0; nRow < TEAM_ROW_COUNT; nRow++)
    {
        if (m_pTeamA->aRows[nRow].aStat[RATING_DAMAGE] > g_flZeroGameEngine)
        {
            m_pTeamA->aRows[nRow].aStat[RATING_DAMAGE] = FULL_WEIGHT;
        }

        if (m_pTeamB->aRows[nRow].aStat[RATING_DAMAGE] > g_flZeroGameEngine)
        {
            m_pTeamB->aRows[nRow].aStat[RATING_DAMAGE] = FULL_WEIGHT;
        }

        nOffset += sizeof(TeamPlayerRow);

        m_pTeamA->aReserveSince[nRow] = 0;
        m_pTeamB->aReserveSince[nRow] = 0;
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

        if (pTeam->aRows[0].aStat[RATING_DAMAGE] > g_flZeroGameEngine && pTeam->aSeat[0] != 0 && pTeam->aSeat[1] != 0)
        {
            nSlot = nTeam;
            SubstitutePlayer(nSlot, 0);
        }
        else
        {
            nSlot   = nTeam;
            pObject = GetPlayer((short)nSlot);
            pObject->m_stats.CopyBaseStats();
            pObject->m_stats.ScaleTo(nSlot);
        }

        if (pTeam->aRows[1].aStat[RATING_DAMAGE] > g_flZeroGameEngine && pTeam->aSeat[0] != 1 && pTeam->aSeat[1] != 1)
        {
            nSlot += 2;
            SubstitutePlayer(nSlot, 1);
        }
        else
        {
            nSlot += 2;
            pObject = GetPlayer((short)nSlot);
            pObject->m_stats.CopyBaseStats();
            pObject->m_stats.ScaleTo(nSlot);
        }
    }

    DeactivateParticleSystems();

    g_pMessageQueue->ProcessAndClear();

    if (m_clock.nPeriod == FIRST_PERIOD)
    {
        if (IsExhibitionMode() != 0)
        {
            g_pMessageQueue->PushFromTable(TEXT_EXHIBITION_MODE, TEXT_LINE_MAIN, TEXT_DURATION);
        }
        else
        {
            g_pMessageQueue->PushFromTable(TEXT_GAUNTLET_MODE, TEXT_LINE_MAIN, TEXT_DURATION);
        }

        g_pMessageQueue->Push(m_pTeamA->GetName(), TEXT_LINE_NAMES, TEXT_DURATION);
        char* const pszVs = g_gameApp.m_stringPool.LoadText(STRING_VS);
        g_pMessageQueue->Push(pszVs, TEXT_LINE_NAMES, TEXT_DURATION);
        g_pMessageQueue->Push(m_pTeamB->GetName(), TEXT_LINE_NAMES, TEXT_DURATION);

        g_pMessageQueue->PushFromTable(GetDifficultyLevel() + TEXT_DIFFICULTY_BASE,
                                       TEXT_LINE_MAIN,
                                       TEXT_DURATION);
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

    m_clock.nTickMs     = 0;
    m_clock.nPrevTickMs = 0;

    m_clock.flPrevInvDeltaSquared = MATCH_RATE_SQUARED;
    m_clock.flInvDeltaSquared     = MATCH_RATE_SQUARED;

    const float flStep = g_flOneGameEngine * MATCH_STEP;

    m_clock.flPrevDelta        = flStep;
    m_clock.flDelta            = flStep;
    m_clock.flPrevDeltaSquared = flStep * flStep;
    m_clock.flDeltaSquared     = flStep * flStep;

    for (nSlot = 0; nSlot < CLOCK_RING_LENGTH; nSlot++)
    {
        m_clock.aflDeltaRing[nSlot] = g_flFifthGameEngine * m_clock.flDelta;
    }

    m_clock.nRingIndex = 0;
    m_clock.nPeriod    = 1;

    const DWORD dwNow = (DWORD)SDL_GetTicks();

    m_clock.nPeriodStartMs = dwNow;
    m_clock.nPauseStartMs  = dwNow;
    m_clock.nPauseMark     = -1;
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

#define CONFIG_KEYWORD_COUNT 3

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
    if (_mbsicmp((const unsigned char*)pszFileName, (const unsigned char*)"game.cfg") == 0)
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
    PathString strPath(String::Concat(g_gameApp.m_strExeDir.m_str, pszFileName).m_pchData, MAX_PATH);
    char       szToken[PARSE_TOKEN_MAX];
    char*      pLine;
    short      nKeyword;

    if (file.m_file.Open(strPath.m_str.m_pchData, FILE_MODE_READ) != 0)
    {
        while (file.m_file.GetNextLine(&pLine) > 0)
        {
            char* pRest = pLine;

            if (pRest != 0
                // STRING: HYPERX 0x004c732c
                && ParseUtil::ParseToken(pRest, szToken, 0, " \t", PARSE_TOKEN_MAX, &pRest) != 0 && (nKeyword = ParseUtil::FindKeyword(szToken, s_apszKeywords, CONFIG_KEYWORD_COUNT)) >= 0)
            {
                switch (nKeyword)
                {
                    case KEYWORD_ARENA:
                        pRest = ParseUtil::SkipPastChar(pRest, g_pszEqualsSign);

                        if (pRest == 0
                            // STRING: HYPERX 0x004c7324
                            || ParseUtil::ParseToken(pRest, szToken, 0, " \t\n\r", PARSE_TOKEN_MAX, &pRest) == 0)
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

                        if (pRest == 0 || ParseUtil::ParseToken(pRest, szToken, 0, " \t\n\r", PARSE_TOKEN_MAX, &pRest) == 0)
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

                        if (pRest == 0 || ParseUtil::ParseToken(pRest, szToken, 0, " \t\n\r", PARSE_TOKEN_MAX, &pRest) == 0)
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

    g_gameApp.ReportLastError(0);
    g_gameApp.OnShutdown();

    return 0;
}

// GLOBAL: HYPERX 0x004c7078
static const char* s_apszCommands[] = {
    "DefineObject", "EndObjectDefinition", "SetMotionParams", "SetRatings", "SetRatings1", "SetRatings2", "SetRatings3", "SetGoalieRatings", "SetGoalieRatings1", "SetGoalieRatings2", "SetGoalieRatings3", "SetSpecialGoalieRatings", "SetCollisionObject",
    // STRING: HYPERX 0x004c7120
    "SetPlans"
};

#define COMMAND_COUNT 14

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
    char  szCommand[PARSE_TOKEN_MAX];
    short nCommand;

    ObjectDef* pDef      = 0;
    char       bComplete = 0;

    while (pFile->GetNextLine(&pLine) > 0)
    {
        // STRING: HYPERX 0x004c732c
        if (pLine != 0 && ParseUtil::ParseToken(pLine, szCommand, 0, " \t", PARSE_TOKEN_MAX, &pLine) != 0 && (nCommand = ParseUtil::FindKeyword(szCommand, s_apszCommands, COMMAND_COUNT)) >= 0)
        {
            switch (nCommand)
            {
                case CMD_DEFINE_OBJECT:
                    pDef = Cmd_DefineObject(pLine, pnModelId);
                    break;

                case CMD_MOTION_PARAMS:
                    if (pDef != 0)
                    {
                        Cmd_SetMotionParams(pLine, pDef);
                    }
                    break;

                case CMD_RATINGS:
                case CMD_RATINGS_1:
                    Cmd_SetRatings(pLine, pDef, RATINGS_FIELD, 0);
                    break;

                case CMD_RATINGS_2:
                    Cmd_SetRatings(pLine, pDef, RATINGS_FIELD, 1);
                    break;

                case CMD_RATINGS_3:
                    Cmd_SetRatings(pLine, pDef, RATINGS_FIELD, 2);
                    break;

                case CMD_GOALIE_RATINGS:
                case CMD_GOALIE_RATINGS_1:
                    Cmd_SetRatings(pLine, pDef, RATINGS_GOALIE, 0);
                    break;

                case CMD_GOALIE_RATINGS_2:
                    Cmd_SetRatings(pLine, pDef, RATINGS_GOALIE, 1);
                    break;

                case CMD_GOALIE_RATINGS_3:
                    Cmd_SetRatings(pLine, pDef, RATINGS_GOALIE, 2);
                    break;

                case CMD_GOALIE_RATINGS_SPECIAL:
                    Cmd_SetRatings(pLine, pDef, RATINGS_GOALIE, RATINGS_SPECIAL_GOALIE);
                    break;

                case CMD_COLLISION_OBJECT:
                    if (pDef != 0)
                    {
                        Cmd_SetCollisionObject(pLine, pDef, *pnModelId);
                    }
                    break;

                case CMD_PLANS:
                    if (pDef != 0)
                    {
                        Cmd_SetPlans(pLine, pDef);
                    }
                    break;

                case CMD_END_DEFINITION:
                    if (pDef != 0)
                    {
                        bComplete = 1;
                    }
                    break;
            }

            if (pDef == 0)
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

    if (pDef != 0)
    {
        // STRING: HYPERX 0x004c7330
        g_gameApp.Fatal("Game Config file error: missing of EndOfDefinition for %s!",
                        pDef->m_pszName);
    }

    return 0;
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

#define ATTRIBUTE_COUNT 6
#define TYPE_COUNT      32

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
    const char* apszAttributes[ATTRIBUTE_COUNT] = {
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
    char  szToken[PARSE_TOKEN_MAX];
    short nType;

    ObjectDef* const pDef = new ObjectDef(0);

    int nSeen = 0;

    const char* apszTypes[TYPE_COUNT] = {
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

    const int anTypeIds[TYPE_COUNT] = {
        KIND_NULL, KIND_DROMER, KIND_ROK, KIND_DROME, KIND_RAMP, KIND_GOAL, KIND_GOAL, KIND_CAMERA, KIND_CAMERA, KIND_START_GUN, KIND_TURNSTILE, KIND_MULTIPLIER, KIND_LASER_HURDLE, KIND_GREEN_RANDOMIZER, KIND_RED_RANDOMIZER, KIND_GOD, KIND_GENERIC, KIND_LIMBO_POLE, KIND_HURDLE, KIND_FAST_LANE, KIND_EXPLOSION, KIND_ARMORY, KIND_KEYBOARD, KIND_GAMEPAD, KIND_JOYSTICK, KIND_MOUSE, KIND_SLOW_LANE, KIND_MINE, KIND_SWARF, KIND_COACH, KIND_BODY_PART, KIND_GENERIC
    };

    char* pRest = pszLine;

    memset(pDef, 0, sizeof(ObjectDef));

    while (pRest != 0)
    {
        // STRING: HYPERX 0x004c7504
        if (ParseUtil::ParseToken(pRest, szToken, 0, " =\t", PARSE_TOKEN_MAX, &pRest) == 0)
        {
            break;
        }

        const short nAttribute = ParseUtil::FindKeyword(szToken, apszAttributes, ATTRIBUTE_COUNT);

        if (nAttribute <= 0)
        {
            break;
        }

        switch (nAttribute)
        {
            case ATTR_TYPE:
                pRest = ParseUtil::SkipPastChar(pRest, g_pszEqualsSign);

                if (pRest == 0
                    // STRING: HYPERX 0x004c7324
                    || ParseUtil::ParseToken(pRest, szToken, 0, " \t\n\r", PARSE_TOKEN_MAX, &pRest) == 0 || (nType = ParseUtil::FindKeyword(szToken, apszTypes, id_GOB_NUM_OF_TYPES)) < 0)
                {
                    g_gameApp.Fatal(
                        // STRING: HYPERX 0x004c7464
                        "Game Config file error: bad type keyword %s!",
                        szToken);
                    break;
                }

                pDef->m_nTypeId = anTypeIds[nType];

                if (nType == id_GOB_Dromer)
                {
                    if (pnModelId == 0)
                    {
                        g_gameApp.Fatal(
                            // STRING: HYPERX 0x004c74d0
                            "ERROR: ParseModelObjDef: dromer model ID is NULL!");
                    }

                    pDef->m_nType = id_GOB_Dromer;
                }
                else
                {
                    pDef->m_nType = nType;

                    if (pDef->m_nType == id_GOB_BodyPart)
                    {
                        if ((int)*pnModelId < SLOT_BODY_PART || (int)*pnModelId > SLOT_BODY_PART_LAST)
                        {
                            // STRING: HYPERX 0x004c74b0
                            g_gameApp.Fatal("Invalid model ID for bodypart\n");
                        }
                    }
                    else if (m_abTypeHighWater[nType] > g_abObjectTypeLimit[nType])
                    {
                        if (nType != id_GOB_Rok || g_bSuppressRokId == 0)
                        {
                            g_gameApp.Fatal(
                                // STRING: HYPERX 0x004c7494
                                "Error: Too many %s objects!",
                                apszTypes[nType]);
                        }
                    }
                    else if (nType != id_GOB_Rok || g_bSuppressRokId == 0)
                    {
                        *pnModelId = m_abTypeHighWater[nType];

                        m_abTypeHighWater[nType]++;
                    }
                }

                nSeen |= SEEN_TYPE;
                break;

            case ATTR_MODEL:
                pRest = ParseUtil::SkipPastChar(pRest, g_pszEqualsSign);

                if (pRest == 0 || ParseUtil::ParseToken(pRest, szToken, 0, " \t\n\r", PARSE_TOKEN_MAX, &pRest) == 0)
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

                if (pRest == 0 || ParseUtil::ParseToken(pRest, szToken, 0, " \t\n\r", PARSE_TOKEN_MAX, &pRest) == 0)
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

                if (pRest == 0 || ParseUtil::ParseToken(pRest, szToken, 0, " \t\n\r", PARSE_TOKEN_MAX, &pRest) == 0)
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

    if ((unsigned char)(nSeen & SEEN_REQUIRED) != SEEN_REQUIRED)
    {
        g_gameApp.Fatal(
            // STRING: HYPERX 0x004c73b8
            "Incomplete Game Object def: %s!",
            pszLine);
    }

    pDef->m_nRatingsMode = 0;

    return pDef;
}

#define MOTION_KEYWORD_COUNT 9

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
    const char* apszKeywords[MOTION_KEYWORD_COUNT] = {
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

    char szToken[PARSE_TOKEN_MAX];

    char* pszCursor = pszLine;

    while (pszCursor != 0)
    {
        if (ParseUtil::ParseToken(pszCursor, szToken, 0, " =\t", PARSE_TOKEN_MAX, &pszCursor) == 0)
        {
            break;
        }

        pszCursor = ParseUtil::SkipToChar(pszCursor, g_pszEqualsSign);

        if (pszCursor == 0)
        {
            break;
        }

        const short nKeyword = ParseUtil::FindKeyword(szToken, apszKeywords, MOTION_KEYWORD_COUNT);

        if (nKeyword <= 0)
        {
            break;
        }

        switch (nKeyword)
        {
            case KEY_POS:
                pszCursor++;

                if (
                    ParseUtil::ParseToken(pszCursor, szToken, 0, " \t\n\r", PARSE_TOKEN_MAX, &pszCursor) == 0 || sscanf(szToken, g_pszPercentF, &pDef->m_vPos.x) != 1 || ParseUtil::ParseToken(pszCursor, szToken, 0, " \t\n\r", PARSE_TOKEN_MAX, &pszCursor) == 0 || sscanf(szToken, g_pszPercentF, &pDef->m_vPos.y) != 1 || ParseUtil::ParseToken(pszCursor, szToken, 0, " \t\n\r", PARSE_TOKEN_MAX, &pszCursor) == 0 || sscanf(szToken, g_pszPercentF, &pDef->m_vPos.z) != 1)
                {
                    g_gameApp.Fatal("Game Config file error: bad type keyword %s!", szToken);
                }

                break;

            case KEY_ACCEL:
                pszCursor++;

                if (ParseUtil::ParseToken(pszCursor, szToken, 0, " \t\n\r", PARSE_TOKEN_MAX, &pszCursor) == 0 || sscanf(szToken, g_pszPercentF, &pDef->m_flAccel) != 1)
                {
                    g_gameApp.Fatal(
                        // STRING: HYPERX 0x004c7658
                        "Game Config file error: accel param %s!",
                        szToken);
                }

                break;

            case KEY_SPEED:
                pszCursor++;

                if (ParseUtil::ParseToken(pszCursor, szToken, 0, " \t\n\r", PARSE_TOKEN_MAX, &pszCursor) == 0 || sscanf(szToken, g_pszPercentF, &pDef->m_flSpeed) != 1)
                {
                    g_gameApp.Fatal("Game Config file error: accel param %s!", szToken);
                }

                break;

            case KEY_AZIM:
                pszCursor++;

                if (ParseUtil::ParseToken(pszCursor, szToken, 0, " \t\n\r", PARSE_TOKEN_MAX, &pszCursor) == 0 || sscanf(szToken, g_pszPercentF, &pDef->m_flAzimuth) != 1)
                {
                    g_gameApp.Fatal("Game Config file error: accel param %s!", szToken);
                }

                break;

            case KEY_ELEV:
                pszCursor++;

                if (ParseUtil::ParseToken(pszCursor, szToken, 0, " \t\n\r", PARSE_TOKEN_MAX, &pszCursor) == 0 || sscanf(szToken, g_pszPercentF, &pDef->m_flElevation) != 1)
                {
                    g_gameApp.Fatal("Game Config file error: accel param %s!", szToken);
                }

                break;

            case KEY_FRIC:
                pszCursor++;

                if (ParseUtil::ParseToken(pszCursor, szToken, 0, " \t\n\r", PARSE_TOKEN_MAX, &pszCursor) == 0 || sscanf(szToken, g_pszPercentF, &pDef->m_flFriction) != 1)
                {
                    g_gameApp.Fatal("Game Config file error: accel param %s!", szToken);
                }

                break;

            case KEY_ELAS:
                pszCursor++;

                if (ParseUtil::ParseToken(pszCursor, szToken, 0, " \t\n\r", PARSE_TOKEN_MAX, &pszCursor) == 0 || sscanf(szToken, g_pszPercentF, &pDef->m_flElasticity) != 1)
                {
                    g_gameApp.Fatal("Game Config file error: accel param %s!", szToken);
                }

                break;
        }
    }

    return 1;
}

#define ATTR_COUNT   6
#define BUBBLE_COUNT 0x1b

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
#define BUBBLE_DROMER_AWARENESS 0x00
#define BUBBLE_DROMER_MODEL     0x01
#define BUBBLE_DROMER_BODY      0x02
#define BUBBLE_DROMER_SCOOP     0x03
#define BUBBLE_RAMP             0x04
#define BUBBLE_ROK              0x05
#define BUBBLE_GOAL_SIDE        0x06
#define BUBBLE_GOAL_ZONE        0x07
#define BUBBLE_GOAL_CASE        0x08
#define BUBBLE_GOAL_PORT        0x09
#define BUBBLE_CAMERA           0x0a
#define BUBBLE_HEAD_CAM         0x0b
#define BUBBLE_MULTIPLIER       0x0c
#define BUBBLE_TURNSTILE        0x0d
#define BUBBLE_STARTGUN         0x0e
#define BUBBLE_LASER_HURDLE     0x0f
#define BUBBLE_ARMORY           0x10
#define BUBBLE_LIMBO_POLE       0x11
#define BUBBLE_FAST_LANE        0x12
#define BUBBLE_EXPLOSION        0x13
#define BUBBLE_HURDLE           0x14
#define BUBBLE_RANDOMIZER       0x15
#define BUBBLE_KLUDGE           0x16
#define BUBBLE_SLOW_LANE        0x17
#define BUBBLE_MINE             0x18
#define BUBBLE_SWARF            0x19
#define BUBBLE_BODY_PART        0x1a

#define COLLISION_MISSING   -9999999.0f
#define COLLISION_NO_RADIUS -1.0f

inline int GameEngine::FindTeamDefinition(FileObj* pFile, char** ppLine, char* pszToken)
{
    do
    {
        // STRING: HYPERX 0x004c732c
        if (ParseUtil::ParseToken(*ppLine, pszToken, 0, " \t", PARSE_TOKEN_MAX, ppLine) != 0 && _mbsicmp((const unsigned char*)pszToken, (const unsigned char*)"DefineTeam") == 0)
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
    const char* apszKeyword[TEAM_KEYWORD_COUNT] = {
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

    char       szToken[PARSE_TOKEN_MAX];
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
        return 0;
    }

    memset(pTeam, 0, sizeof(TeamBlock));

    while (pLine != 0)
    {
        // STRING: HYPERX 0x004c7504
        if (ParseUtil::ParseToken(pLine, szToken, 0, " =\t", PARSE_TOKEN_MAX, &pLine) == 0)
        {
            break;
        }

        pLine = ParseUtil::SkipToChar(pLine, g_pszEqualsSign);

        if (pLine == 0)
        {
            break;
        }

        nKeyword = ParseUtil::FindKeyword(szToken, apszKeyword, TEAM_KEYWORD_COUNT);

        if (nKeyword < 0)
        {
            break;
        }

        switch (nKeyword)
        {
            case 0:
                pLine++;

                if (ParseUtil::ParseToken(pLine, szToken, 0, " \t\n\r", PARSE_TOKEN_MAX, &pLine) == 0)
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

                if (ParseUtil::ParseToken(pLine, szToken, 0, " \t\n\r", PARSE_TOKEN_MAX, &pLine) == 0)
                {
                    // STRING: HYPERX 0x004c77d8
                    g_gameApp.Error("Team asgn error: Team LogoBMPname: %s!", szToken);

                    goto failed;
                }

                strcpy(pTeam->szIdLogo, szToken);

                break;

            case 2:
                pLine++;

                if (ParseUtil::ParseToken(pLine, szToken, 0, " \t\n\r", PARSE_TOKEN_MAX, &pLine) == 0)
                {
                    // STRING: HYPERX 0x004c77b0
                    g_gameApp.Error("Team asgn error: Team select logo: %s!", szToken);

                    goto failed;
                }

                strcpy(pTeam->szTsLogo, szToken);

                break;

            case 3:
                pLine++;

                if (ParseUtil::ParseToken(pLine, szToken, 0, " \t\n\r", PARSE_TOKEN_MAX, &pLine) == 0)
                {
                    // STRING: HYPERX 0x004c7788
                    g_gameApp.Error("Team asgn error: Team select text: %s!", szToken);

                    goto failed;
                }

                strcpy(pTeam->szTsText, szToken);

                break;

            case 4:
                pLine++;

                if (ParseUtil::ParseToken(pLine, szToken, 0, " \t\n\r", PARSE_TOKEN_MAX, &pLine) == 0)
                {
                    // STRING: HYPERX 0x004c7760
                    g_gameApp.Error("Team asgn error: Team select title: %s!", szToken);

                    goto failed;
                }

                strcpy(pTeam->szTsTitle, szToken);

                break;

            case 5:
                pLine++;

                if (ParseUtil::ParseToken(pLine, szToken, 0, " \t\n\r", PARSE_TOKEN_MAX, &pLine) == 0)
                {
                    // STRING: HYPERX 0x004c772c
                    g_gameApp.Error("Team asgn error: Team select animation name: %s!", szToken);

                    goto failed;
                }

                strcpy(pTeam->szTsAnim, szToken);

                break;

            case 6:
                pLine++;

                if (ParseUtil::ParseToken(pLine, szToken, 0, " \t\n\r", PARSE_TOKEN_MAX, &pLine) == 0)
                {
                    // STRING: HYPERX 0x004c76f4
                    g_gameApp.Error("Team asgn error: Team select Drome exterior name: %s!", szToken);

                    goto failed;
                }

                strcpy(pTeam->szEnvironment, szToken);

                break;

            case 8:
                pLine++;

                if (ParseUtil::ParseToken(pLine, szToken, 0, " \t\n\r", PARSE_TOKEN_MAX, &pLine) == 0)
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

        return 0;
    }

    return pTeam;

failed:
    delete pTeam;

quit:
    return 0;
}

// A RandomizerBubble on a RedRandomizer becomes the next kind ("MAJOR KLUDGE HERE").

// FUNCTION: HYPERX 0x00483300
char GameEngine::Cmd_SetCollisionObject(char* pszLine, ObjectDef* pDef, unsigned int nModelId)
{
    const char* apszAttributes[ATTR_COUNT] = {
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

    const char* apszBubbles[BUBBLE_COUNT] = {
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

    int anBubbleShapes[BUBBLE_COUNT] = {
        BUBBLE_SHAPE_SPHERE, BUBBLE_SHAPE_SPHERE, BUBBLE_SHAPE_SPHERE, BUBBLE_SHAPE_ATTACHED, BUBBLE_SHAPE_CAGED, BUBBLE_SHAPE_SPHERE, BUBBLE_SHAPE_SPHERE, BUBBLE_SHAPE_SPHERE, BUBBLE_SHAPE_SPHERE, BUBBLE_SHAPE_SPHERE, BUBBLE_SHAPE_SPHERE, BUBBLE_SHAPE_SPHERE, BUBBLE_SHAPE_SPHERE, BUBBLE_SHAPE_SPHERE, BUBBLE_SHAPE_SPHERE, BUBBLE_SHAPE_SPHERE, BUBBLE_SHAPE_SPHERE, BUBBLE_SHAPE_SPHERE, BUBBLE_SHAPE_SPHERE, BUBBLE_SHAPE_SPHERE, BUBBLE_SHAPE_SPHERE, BUBBLE_SHAPE_SPHERE, BUBBLE_SHAPE_SPHERE, BUBBLE_SHAPE_SPHERE, BUBBLE_SHAPE_SPHERE, BUBBLE_SHAPE_SPHERE, BUBBLE_SHAPE_SPHERE
    };

    char szToken[PARSE_TOKEN_MAX];
    Vec3 vCentroid;
    Vec3 vAttach;

    short nBubbleType = -1;
    short nComponent  = -1;

    float flRadius = COLLISION_NO_RADIUS;
    vCentroid.x    = COLLISION_MISSING;
    vAttach.x      = COLLISION_MISSING;

    char bUseDetail = 0;

    char* pszCursor = pszLine;

    while (pszCursor != 0)
    {
        if (ParseUtil::ParseToken(pszCursor, szToken, 0, " =\t", PARSE_TOKEN_MAX, &pszCursor) == 0)
        {
            break;
        }

        const short nAttribute = ParseUtil::FindKeyword(szToken, apszAttributes, ATTR_COUNT);

        if (nAttribute < 0)
        {
            break;
        }

        switch (nAttribute)
        {
            case ATTR_TYPE_KEY:
                pszCursor = ParseUtil::SkipPastChar(pszCursor, g_pszEqualsSign);

                if (pszCursor != 0 && ParseUtil::ParseToken(pszCursor, szToken, 0, " \t\n\r", PARSE_TOKEN_MAX, &pszCursor) != 0)
                {
                    nBubbleType = ParseUtil::FindKeyword(szToken, apszBubbles, BUBBLE_COUNT);

                    if (nBubbleType >= 0)
                    {
                        if (nBubbleType == BUBBLE_RANDOMIZER && pDef->m_nType == id_GOB_RedRandomizer)
                        {
                            nBubbleType = BUBBLE_KLUDGE;
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

                if (pszCursor == 0 || ParseUtil::ParseToken(pszCursor, szToken, 0, " \t\n\r", PARSE_TOKEN_MAX, &pszCursor) == 0 || sscanf(szToken, g_pszPercentHd, &nComponent) != 1)
                {
                    g_gameApp.Fatal(
                        // STRING: HYPERX 0x004c796c
                        "Game Config file error: collision radius %s!",
                        pszLine);
                }

                break;

            case ATTR_RADIUS:
                pszCursor = ParseUtil::SkipPastChar(pszCursor, g_pszEqualsSign);

                if (pszCursor == 0 || ParseUtil::ParseToken(pszCursor, szToken, 0, " \t\n\r", PARSE_TOKEN_MAX, &pszCursor) == 0 || sscanf(szToken, g_pszPercentF, &flRadius) != 1)
                {
                    g_gameApp.Fatal("Game Config file error: collision radius %s!", pszLine);
                }

                break;

            case ATTR_CENTROID:
                pszCursor = ParseUtil::SkipPastChar(pszCursor, g_pszEqualsSign);

                if (pszCursor == 0 || ParseUtil::ParseToken(pszCursor, szToken, 0, " \t\n\r", PARSE_TOKEN_MAX, &pszCursor) == 0 || sscanf(szToken, g_pszPercentF, &vCentroid.x) != 1 || ParseUtil::ParseToken(pszCursor, szToken, 0, " \t\n\r", PARSE_TOKEN_MAX, &pszCursor) == 0 || sscanf(szToken, g_pszPercentF, &vCentroid.y) != 1 || ParseUtil::ParseToken(pszCursor, szToken, 0, " \t\n\r", PARSE_TOKEN_MAX, &pszCursor) == 0 || sscanf(szToken, g_pszPercentF, &vCentroid.z) != 1)
                {
                    g_gameApp.Fatal(
                        // STRING: HYPERX 0x004c793c
                        "Game Config file error: collision centroid %s!",
                        szToken);
                }

                break;

            case ATTR_ATTACH:
                pszCursor = ParseUtil::SkipPastChar(pszCursor, g_pszEqualsSign);

                if (pszCursor == 0 || ParseUtil::ParseToken(pszCursor, szToken, 0, " \t\n\r", PARSE_TOKEN_MAX, &pszCursor) == 0 || sscanf(szToken, g_pszPercentF, &vAttach.x) != 1 || ParseUtil::ParseToken(pszCursor, szToken, 0, " \t\n\r", PARSE_TOKEN_MAX, &pszCursor) == 0 || sscanf(szToken, g_pszPercentF, &vAttach.y) != 1 || ParseUtil::ParseToken(pszCursor, szToken, 0, " \t\n\r", PARSE_TOKEN_MAX, &pszCursor) == 0 || sscanf(szToken, g_pszPercentF, &vAttach.z) != 1)
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

    if (nBubbleType == -1 || nBubbleType >= BUBBLE_COUNT)
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
    BubbleCmd* pTail = 0;

    for (BubbleCmd* pNode = *ppSlot; pNode != 0; pNode = *ppSlot)
    {
        ppSlot = &pNode->m_pChildren;
        pTail  = pNode;
    }

    if (pTail != 0)
    {
        pTail->m_nChildCount++;
    }

    BubbleCmd* const pNew = new BubbleCmd;

    *ppSlot = pNew;

    memset(pNew, 0, sizeof(BubbleCmd));

    pNew->m_wShape    = (short)anBubbleShapes[nBubbleType];
    pNew->m_nSlot     = nBubbleType;
    pNew->m_pChildren = 0;

    if (vAttach.x != COLLISION_MISSING)
    {
        AttachPak* const pExtra = new AttachPak;

        memset(pExtra, 0, sizeof(AttachPak));

        pExtra->nLinkId = nComponent;

        pExtra->vOffset = vAttach;

        pNew->m_pAttach = pExtra;
    }
    else if (bUseDetail != 0)
    {
        pNew->m_pAttach = (AttachPak*)(INT_PTR)EXTRA_USE_DETAIL;
    }

    MapModifierToId((short)nModelId, pNew);

    BubbleShapeNode* const pPos = new BubbleShapeNode;

    pNew->m_pPos = pPos;

    pPos->m_nComponent = nComponent;

    pNew->m_pPos->m_flRadius = flRadius;

    pNew->m_pPos->m_vCentre = vCentroid;

    return 1;
}

#define RATINGS_PLAYER_COUNT 0x40
#define RATINGS_GOALIE_COUNT 0x11

#define GOALIE_RATING_DEFAULT 5.0f

// Values are whole; EndRatings or anything past the end stores one past the array.
// FUNCTION: HYPERX 0x00483b50
int GameEngine::Cmd_SetRatings(char* pszLine, ObjectDef* pDef, int nMode, int nLevel)
{
    const char* apszPlayer[RATINGS_PLAYER_COUNT] = {
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

    const char* apszGoalie[RATINGS_GOALIE_COUNT] = {
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
    char          szToken[PARSE_TOKEN_MAX];
    char          szObject[PARSE_TOKEN_MAX];
    int           nCount;
    int           nLast;
    int           nValue;
    unsigned char nEntry;

    if (pDef == 0)
    {
        return -1;
    }

    if (nMode == RATINGS_FIELD)
    {
        ppszTable = apszPlayer;
        nCount    = RATINGS_PLAYER_COUNT;
        nLast     = RATING_COUNT;
    }
    else
    {
        ppszTable = apszGoalie;
        nCount    = RATINGS_GOALIE_COUNT;
        nLast     = GOALIE_RATING_COUNT;
    }

    if (pDef->m_apRatings[nLevel] == 0)
    {
        pDef->m_nRatingsMode = nMode;

        if (nMode == RATINGS_FIELD)
        {
            pDef->m_apRatings[nLevel] = new float[RATING_COUNT];

            for (nEntry = 0; nEntry < RATING_COUNT; nEntry++)
            {
                pDef->m_apRatings[nLevel][nEntry] = 0.0f;
            }
        }
        else
        {
            pDef->m_apRatings[nLevel] = new float[GOALIE_RATING_COUNT];

            for (nEntry = 0; nEntry < GOALIE_RATING_COUNT; nEntry++)
            {
                pDef->m_apRatings[nLevel][nEntry] = GOALIE_RATING_DEFAULT;
            }
        }
    }

    float* const pflRatings = pDef->m_apRatings[nLevel];

    char* pszCursor = pszLine;

    while (pszCursor != 0)
    {
        if (ParseUtil::ParseToken(pszCursor, szToken, 0, " =\t", PARSE_TOKEN_MAX, &pszCursor) == 0)
        {
            break;
        }

        pszCursor = ParseUtil::SkipToChar(pszCursor, g_pszEqualsSign);

        if (pszCursor == 0)
        {
            break;
        }

        const short nKeyword = ParseUtil::FindKeyword(szToken, ppszTable, (short)nCount);

        if (nKeyword < 0)
        {
            break;
        }

        pszCursor++;

        if (ParseUtil::ParseToken(pszCursor, szToken, 0, " \t\n\r", PARSE_TOKEN_MAX, &pszCursor) != 0 && sscanf(szToken, g_pszPercentD, &nValue) == 1)
        {
            if (nKeyword >= nLast)
            {
                g_gameApp.Fatal(
                    // STRING: HYPERX 0x004c7bd8
                    "Game Config file error: ratings for object %s!",
                    szObject);
            }

            pflRatings[nKeyword] = (float)nValue;
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
        { COB_DR0_AWARENESS, COB_DR1_AWARENESS, COB_DR2_AWARENESS, COB_DR3_AWARENESS, COB_DR4_AWARENESS, COB_DR5_AWARENESS },
        { COB_DROMER_0, COB_DROMER_1, COB_DROMER_2, COB_DROMER_3, COB_DROMER_4, COB_DROMER_5 },
        { COB_DROMER_BODY_0, COB_DROMER_BODY_1, COB_DROMER_BODY_2, COB_DROMER_BODY_3, COB_DROMER_BODY_4, COB_DROMER_BODY_5 },
        { COB_BATON_EZONE_0, COB_BATON_EZONE_1, COB_BATON_EZONE_2, COB_BATON_EZONE_3, COB_BATON_EZONE_4, COB_BATON_EZONE_5 }
    };
    short aPair[4][2] = {
        { COB_NGOAL_SIDE, COB_SGOAL_SIDE },
        { COB_NGOAL_ZONE, COB_SGOAL_ZONE },
        { COB_NGOAL_CASE, COB_SGOAL_CASE },
        { COB_NGOAL_PORT, COB_SGOAL_PORT }
    };

    switch ((unsigned short)pCmd->m_nSlot)
    {
        case BUBBLE_BODY_PART:
            pCmd->m_nSlot = nModelId - g_abObjectTypeBaseline[id_GOB_BodyPart] + COB_BODY_PART0;
            return;
        case BUBBLE_DROMER_AWARENESS:
            pCmd->m_nSlot = aRow[0][nModelId];
            return;
        case BUBBLE_DROMER_MODEL:
            pCmd->m_nSlot = aRow[1][nModelId];
            return;
        case BUBBLE_DROMER_BODY:
            pCmd->m_nSlot = aRow[2][nModelId];
            return;
        case BUBBLE_DROMER_SCOOP:
            pCmd->m_nSlot = aRow[3][nModelId];
            if (pCmd->m_wShape == BUBBLE_SHAPE_ATTACHED)
            {
                pCmd->m_pAttach->nSlot = nModelId;
            }
            break;
        case BUBBLE_ROK:
            pCmd->m_nSlot = COB_ROK;
            return;
        case BUBBLE_RAMP:
            pCmd->m_nSlot = nModelId - g_abObjectTypeBaseline[id_GOB_Ramp] + COB_RAMP;
            return;
        case BUBBLE_GOAL_SIDE:
            pCmd->m_nSlot = aPair[0][nModelId - SLOT_NGOAL];
            return;
        case BUBBLE_GOAL_ZONE:
            pCmd->m_nSlot = aPair[1][nModelId - SLOT_NGOAL];
            return;
        case BUBBLE_GOAL_CASE:
            pCmd->m_nSlot = aPair[2][nModelId - SLOT_NGOAL];
            return;
        case BUBBLE_GOAL_PORT:
            pCmd->m_nSlot = aPair[3][nModelId - SLOT_NGOAL];
            return;
        case BUBBLE_CAMERA:
            pCmd->m_nSlot = COB_CAMERA;
            return;
        case BUBBLE_HEAD_CAM:
            pCmd->m_nSlot = COB_HEAD_CAM;
            return;
        case BUBBLE_STARTGUN:
            pCmd->m_nSlot = COB_START_GUN;
            return;
        case BUBBLE_TURNSTILE:
            pCmd->m_nSlot = nModelId - g_abObjectTypeBaseline[id_GOB_Turnstile] + COB_TURNSTILE;
            return;
        case BUBBLE_MULTIPLIER:
            pCmd->m_nSlot = nModelId - g_abObjectTypeBaseline[id_GOB_Multiplier] + COB_MULTIPLIER;
            return;
        case BUBBLE_LASER_HURDLE:
            pCmd->m_nSlot = nModelId - g_abObjectTypeBaseline[id_GOB_LaserHurdle] + COB_LASER_HURDLE;
            return;
        case BUBBLE_KLUDGE:
            pCmd->m_nSlot = nModelId - g_abObjectTypeBaseline[id_GOB_RedRandomizer] + COB_RED_RANDOMIZER;
            return;
        case BUBBLE_RANDOMIZER:
            pCmd->m_nSlot = nModelId - g_abObjectTypeBaseline[id_GOB_GreenRandomizer] + COB_GREEN_RANDOMIZER;
            return;
        case BUBBLE_LIMBO_POLE:
            pCmd->m_nSlot = nModelId - g_abObjectTypeBaseline[id_GOB_LimboPole] + COB_LIMBO_POLE;
            return;
        case BUBBLE_HURDLE:
            pCmd->m_nSlot = nModelId - g_abObjectTypeBaseline[id_GOB_Hurdle] + COB_HURDLE;
            return;
        case BUBBLE_FAST_LANE:
            pCmd->m_nSlot = nModelId - g_abObjectTypeBaseline[id_GOB_FastLane] + COB_FAST_LANE;
            return;
        case BUBBLE_EXPLOSION:
            pCmd->m_nSlot = nModelId - g_abObjectTypeBaseline[id_GOB_Explosion] + COB_EXPLOSION;
            return;
        case BUBBLE_ARMORY:
            pCmd->m_nSlot = nModelId - g_abObjectTypeBaseline[id_GOB_Armory] + COB_ARMORY;
            return;
        case BUBBLE_SLOW_LANE:
            pCmd->m_nSlot = nModelId - g_abObjectTypeBaseline[id_GOB_SlowLane] + COB_SLOW_LANE;
            return;
        case BUBBLE_MINE:
            pCmd->m_nSlot = nModelId - g_abObjectTypeBaseline[id_GOB_Mine] + COB_MINE;
            return;
        case BUBBLE_SWARF:
            pCmd->m_nSlot = nModelId - g_abObjectTypeBaseline[id_GOB_Swarf] + COB_SWARF;
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
#define PLAN_TOKEN_MAX 0x50
#define TOKENS_MAX     30

// A token longer than the buffer overruns it; nothing bounds the token count.
// FUNCTION: HYPERX 0x00484460
char GameEngine::Cmd_SetPlans(char* pszText, ObjectDef* pDef)
{
    char* apTokens[TOKENS_MAX];
    char  szToken[PLAN_TOKEN_MAX];

    short nIndex;

    short nParsed = 0;

    memset(szToken, 0, sizeof(szToken));

    char* pszCursor = pszText;

    while (pszCursor != 0)
    {
        // STRING: HYPERX 0x004c7324
        if (ParseUtil::ParseToken(pszCursor, szToken, 0, " \t\n\r", (short)strlen(pszCursor), &pszCursor) == 0)
        {
            break;
        }

        char** ppSlot = &apTokens[nParsed];

        nParsed++;

        *ppSlot = strcpy(new char[strlen(szToken) + 1], szToken);
    }

    const short  nOld  = pDef->m_nPlanCount;
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
        int nId = m_pArena->m_pnSlotIds[m_pArena->m_wCount];

        EngineObject* const pObject = m_ppSlots[(short)nId];

        RemoveFromSortedList((short)m_pArena->m_pnSlotIds[m_pArena->m_wCount]);

        UnregisterObject(m_pArena->m_pnSlotIds[m_pArena->m_wCount]);

        for (nType = 0; nType < id_GOB_NUM_OF_TYPES; nType++)
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

        if (pObject != 0)
        {
            delete pObject;
        }
    }

    ArenaState* const pArena = m_pArena;

    if (pArena != 0)
    {
        if (pArena->m_pnSlotIds != 0)
        {
            delete[] pArena->m_pnSlotIds;
        }

        delete pArena->m_pRokDef;

        delete pArena;
    }

    m_pArena = 0;
}

// The type id starts at 102, so a file that names no type still produces something.
// FUNCTION: HYPERX 0x00484cf0
char GameEngine::LoadObjectsFromConfig(const char* pszFileName)
{
    OwnedFile  file;
    PathString strPath(String::Concat(g_gameApp.m_strExeDir.m_str, pszFileName).m_pchData, MAX_PATH);

    if (file.m_file.Open(strPath.m_str.m_pchData, FILE_MODE_READ) != 0)
    {
        unsigned int nModelId = MODEL_ID_NONE;
        ObjectDef*   pDef     = ParseObjectDefinition(&file.m_file, &nModelId);

        while (pDef != 0)
        {
            CreateObject(pDef, nModelId);

            delete pDef;

            pDef = ParseObjectDefinition(&file.m_file, &nModelId);
        }

        file.m_file.CloseAndTest();

        return 1;
    }

    g_gameApp.ReportLastError(0);
    g_gameApp.OnShutdown();

    return 0;
}

#define ARENA_OBJECT_MAX 100

// Fatal returns, so an object past the hundredth is still built and overruns anModelIds.
// FUNCTION: HYPERX 0x00484e80
char GameEngine::LoadArena(const char* pszArenaPath)
{
    OwnedFile    file;
    unsigned int anModelIds[ARENA_OBJECT_MAX];

    if (m_pArena != 0)
    {
        UnloadArena();
    }

    m_pArena = 0;

    PathString strPath(String::Concat(g_gameApp.m_strExeDir.m_str, pszArenaPath).m_pchData, MAX_PATH);

    if (file.m_file.Open(strPath.m_str.m_pchData, FILE_MODE_READ) != 0)
    {
        m_pArena = new ArenaState;

        Renderer::SetLoadingMode();

        unsigned int nModelId = MODEL_ID_NONE;
        ObjectDef*   pDef     = ParseObjectDefinition(&file.m_file, &nModelId);

        while (pDef != 0)
        {
            if (m_pArena->m_wCount >= ARENA_OBJECT_MAX)
            {
                GameApp* const pApp = &g_gameApp;

                // STRING: HYPERX 0x004c8384
                pApp->Fatal("Too many objects in .ENV file");
            }

            CreateObject(pDef, nModelId);

            anModelIds[m_pArena->m_wCount++] = nModelId;

            if (nModelId == SLOT_ROK)
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

    g_gameApp.ReportLastError(0);

    return 0;
}

// Selecting the arena already selected answers true at once.
// FUNCTION: HYPERX 0x004850f0
char GameEngine::SetArena(short nArena)
{
    const char* pszPath;
    int         nIndex;

    if (nArena == (short)m_nArenaIndex)
    {
        return 1;
    }

    m_nArenaIndex = nArena;

    TeamsBlock* const pTable = g_pGameEngine->m_pTeamTable;

    if (nArena < pTable->GetTeamCount())
    {
        pszPath = pTable->m_ppTeams[nArena]->szEnvironment;
    }
    else
    {
        pszPath = 0;
    }

    if (LoadArena(pszPath) != 0)
    {
        const int nCount = m_pArena->m_wCount;

        for (nIndex = 0; nIndex < nCount; nIndex++)
        {
            EngineObject* pObject = m_ppSlots[(short)m_pArena->m_pnSlotIds[nIndex]];

            if (pObject != 0 && pObject->GetSlot() != SLOT_DROME)
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

    if (pSet == 0)
    {
        return;
    }

    for (nIndex = 0; nIndex < TEAM_BLOCK_SLOTS; nIndex++)
    {
        EngineObject* const pEntity = pSet->apOnIce[nIndex];

        if (pEntity != 0)
        {
            int nType = pEntity->GetSlot();

            for (iRange = 0; iRange < id_GOB_NUM_OF_TYPES; iRange++)
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

            RemoveFromSortedList((short)nType);

            UnregisterObject(nType);

            delete pEntity;
        }
    }

    delete pSet;
}

// FUNCTION: HYPERX 0x00485250
char GameEngine::UnloadHomeTeam()
{
    TeamBlock* const pSet = m_pTeamA;

    if (pSet != 0)
    {
        UnloadEntitySet(pSet);

        m_pTeamA = 0;

        return 1;
    }

    return 0;
}

// FUNCTION: HYPERX 0x00485280
char GameEngine::UnloadAwayTeam()
{
    TeamBlock* const pSet = m_pTeamB;

    if (pSet != 0)
    {
        UnloadEntitySet(pSet);

        m_pTeamB = 0;

        return 1;
    }

    return 0;
}

#define SECOND_RUN_FIRST 5
#define SECOND_RUN_LAST  8
#define FIRST_RUN_LAST   2

// A rematch reloads the same teams; a failed load names the home team whichever half failed.
// FUNCTION: HYPERX 0x004856f0
char GameEngine::SetTeams(short nHome, short nAway)
{
    TeamsBlock* pTable;
    const char* pszTeam;
    int         nSlot;

    Window::SetBusyCursor(true);

    if (nHome != (short)m_nTeamHomeIndex || m_bMatchOver != 0)
    {
        UnloadHomeTeam();
    }

    if (nAway != (short)m_nTeamAwayIndex || m_bMatchOver != 0)
    {
        UnloadAwayTeam();
    }

    if (nHome != (short)m_nTeamHomeIndex || m_bMatchOver != 0)
    {
        m_nTeamHomeIndex = nHome;

        pTable = g_pGameEngine->m_pTeamTable;

        if (nHome < pTable->GetTeamCount())
        {
            pszTeam = pTable->m_ppTeams[nHome]->szPath;
        }
        else
        {
            pszTeam = 0;
        }

        if (LoadHomeTeam(pszTeam) == 0)
        {
            // STRING: HYPERX 0x004c83f0
            g_gameApp.Fatal("Error initializing home team\n");
        }

        for (nSlot = 0; nSlot < TEAM_BLOCK_SLOTS; nSlot++)
        {
            if ((nSlot >= SECOND_RUN_FIRST && nSlot <= SECOND_RUN_LAST) || (nSlot >= 0 && nSlot <= FIRST_RUN_LAST))
            {
                m_pTeamA->apOnIce[nSlot]->CompileAllPlans();
            }
        }
    }

    if (nAway != (short)m_nTeamAwayIndex || m_bMatchOver != 0)
    {
        m_nTeamAwayIndex = nAway;

        pTable = g_pGameEngine->m_pTeamTable;

        if (nAway < pTable->GetTeamCount())
        {
            pszTeam = pTable->m_ppTeams[nAway]->szPath;
        }
        else
        {
            pszTeam = 0;
        }

        if (LoadAwayTeam(pszTeam) == 0)
        {
            g_gameApp.Fatal("Error initializing home team\n");
        }

        for (nSlot = 0; nSlot < TEAM_BLOCK_SLOTS; nSlot++)
        {
            if ((nSlot >= SECOND_RUN_FIRST && nSlot <= SECOND_RUN_LAST) || (nSlot >= 0 && nSlot <= FIRST_RUN_LAST))
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

#define MODEL_ID_UNASSIGNED (-1)

#define TEAM_SIDE_AWAY 1

// The ten slots, and which of them are the body parts.
#define TEAM_SLOT_BODY_PART_FIRST 5
#define TEAM_SLOT_BODY_PART_LAST  8
#define TEAM_SLOT_ROK             9

// Three players carry the running id; the fourth and fifth do not.
#define TEAM_PLAYERS_ON_DROME 3

// Ten slots: five players, four body parts and the team's Rok; only four players get ratings.
// FUNCTION: HYPERX 0x004858b0
TeamBlock* GameEngine::LoadTeamFromFile(const char* pszTeamPath, int nTeam)
{
    OwnedFile  file;
    ObjectDef* pObjDef;
    int        nRow;

    PathString strPath(String::Concat(g_gameApp.m_strExeDir.m_str, pszTeamPath).m_pchData,
                       MAX_PATH);

    if (file.m_file.Open(strPath.m_str.m_pchData, FILE_MODE_READ) != 0)
    {
        int nSlot = 0;

        TeamBlock* pTeamDef = ParseTeamHTMFile(&file.m_file);

        if (pTeamDef == 0)
        {
            // STRING: HYPERX 0x004c8478
            g_gameApp.Fatal("Away team definition error in file %s!", pszTeamPath);

            return 0;
        }

        strcpy(pTeamDef->szPath, pszTeamPath);

        int nPlayers   = 0;
        int nRunningId = nTeam;

        for (; nSlot < TEAM_BLOCK_SLOTS; nSlot++)
        {
            int          nBodyPartRow;
            int          nKind;
            int          nLevel;
            const float* pRatings;
            float*       pRow;
            int          i;

            unsigned int nObjectId = MODEL_ID_NONE;

            if (nPlayers < TEAM_PLAYERS_ON_DROME)
            {
                g_bSuppressRokId = 0;
                nObjectId        = nRunningId;
            }
            else if (nSlot == TEAM_SLOT_ROK)
            {
                g_bSuppressRokId = 1;
                nObjectId        = (unsigned int)MODEL_ID_UNASSIGNED;
            }
            else if (nSlot >= TEAM_SLOT_BODY_PART_FIRST && nSlot <= TEAM_SLOT_BODY_PART_LAST)
            {
                nBodyPartRow = nSlot - TEAM_SLOT_BODY_PART_FIRST;

                if (nTeam == TEAM_SIDE_AWAY)
                {
                    nBodyPartRow += 4;
                }

                nObjectId = g_anBodyPartModelIds[nBodyPartRow];
            }

            pObjDef = ParseObjectDefinition(&file.m_file, &nObjectId);

            g_bSuppressRokId = 0;

            if (pObjDef == 0)
            {
                // STRING: HYPERX 0x004c8454
                g_gameApp.Fatal("Team definition file incomplete %s!", pszTeamPath);
            }

            nKind = pObjDef->m_nType;

            if (nKind == id_GOB_Dromer)
            {
                nPlayers++;
                nRunningId += 2;
            }

            if (nTeam == TEAM_SIDE_AWAY)
            {
                pObjDef->m_bForceRemap = 1;
            }

            if ((pObjDef->m_nType == id_GOB_Dromer && nPlayers <= TEAM_PLAYERS_ON_DROME) || pObjDef->m_nType == id_GOB_BodyPart)
            {
                pTeamDef->apOnIce[nSlot] = CreateObject(pObjDef, nObjectId);
            }
            else
            {
                pTeamDef->apOnIce[nSlot] = 0;
            }

            switch (nSlot)
            {
                case 0:
                case 1:
                case 2:
                case 3:
                case 4:
                    if (pObjDef->m_nType != id_GOB_Dromer)
                    {
                        // STRING: HYPERX 0x004c8430
                        g_gameApp.Fatal("Team definition file invalid %s!", pszTeamPath);
                    }

                    break;

                case 5:
                case 6:
                case 7:
                case 8:
                    if (pObjDef->m_nType != id_GOB_BodyPart)
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
                nLevel = m_nDifficulty;

                if (g_bSessionActive != 0)
                {
                    nLevel = 0;
                }

                pRatings = pObjDef->m_apRatings[nLevel];

                if (pRatings == 0)
                {
                    pRatings = pObjDef->m_apRatings[0];
                }

                pRow = pTeamDef->aRows[nRow].aStat;

                for (i = 0; i < RATING_COUNT; i++)
                {
                    *pRow = *pRatings;

                    pRatings++;
                    pRow++;
                }

                memset(&pTeamDef->aStatRows[nRow], 0, sizeof(TeamStatRow));
            }

            if (nSlot >= TEAM_SLOT_BODY_PART_FIRST && nSlot <= TEAM_SLOT_BODY_PART_LAST)
            {
                delete pObjDef;

                pObjDef = 0;
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

    return 0;
}

// FUNCTION: HYPERX 0x00485c70
char GameEngine::LoadHomeTeam(const char* pszTeamPath)
{
    TeamBlock* const pTeam = LoadTeamFromFile(pszTeamPath, 0);

    m_pTeamA = pTeam;

    if (pTeam != 0)
    {
        if (g_pRenderer != 0)
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
    TeamBlock* const pTeam = LoadTeamFromFile(pszTeamPath, 1);

    m_pTeamB = pTeam;

    if (pTeam != 0)
    {
        if (g_pRenderer != 0)
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
    if (m_pArena != 0)
    {
        ObjectDef* const pDef = m_pArena->m_pRokDef;

        if (pDef != 0)
        {
            return pDef;
        }
    }

    return 0;
}

// FUNCTION: HYPERX 0x00485d10
ObjectDef* GameEngine::GetTeamRokDef(int nSlot)
{
    const TeamBlock* pCatalog;

    if (nSlot < SLOT_DROMER0 || nSlot > SLOT_DROMER3)
    {
        return 0;
    }

    if ((nSlot & 1) != 0)
    {
        pCatalog = m_pTeamB;
    }
    else
    {
        pCatalog = m_pTeamA;
    }

    if (pCatalog != 0)
    {
        ObjectDef* const pEntry = pCatalog->apDefs[TEAM_SLOT_ROK];

        if (pEntry != 0)
        {
            return pEntry;
        }
    }

    return 0;
}

// A reserve's recovery since leaving the ice is added once, clamped at a hundred.
// FUNCTION: HYPERX 0x00485d50
char GameEngine::SubstitutePlayer(int nSlot, int nWith)
{
    TeamBlock* pTeam;
    int        nDefIndex;
    int        nSide;

    switch ((unsigned int)nWith)
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

    switch ((unsigned int)nSlot)
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

    PlayerObject* const pPlayer    = (PlayerObject*)pTeam->apOnIce[nSide];
    int                 nOnIceSlot = pPlayer->GetSlot();
    ObjectStats* const  pBody      = &pPlayer->m_stats;

    pBody->CopyBaseStats();

    const int nWasOn = pTeam->aSeat[nSide];

    memmove(&pTeam->aRows[nWasOn], &pPlayer->m_stats.m_aStats[1], sizeof(TeamPlayerRow));
    memmove(&pTeam->aStatRows[nWasOn], &pPlayer->m_matchStats, sizeof(TeamStatRow));

    pTeam->aReserveSince[nWasOn] = m_clock.nTickMs;

    const int nStamp = pTeam->aReserveSince[nWith];

    if (nStamp != 0)
    {
        const int nElapsed = m_clock.nTickMs - nStamp;
        float     flGate;

        flGate = DamageGate();

        const float  flRested  = (float)(unsigned int)nElapsed;
        float* const pflDamage = &pTeam->aRows[nWith].aStat[RATING_DAMAGE];
        float        flDamage;

        if (100.0f < flRested / (13500.0f / flGate) + *pflDamage)
        {
            flDamage = 100.0f;
        }
        else
        {
            flDamage = flRested / (13500.0f / DamageGate()) + *pflDamage;
        }

        *pflDamage = flDamage;

        pTeam->aReserveSince[nWith] = 0;
    }

    ReplaceEntity(nOnIceSlot,
                  pTeam->apDefs[nDefIndex]);

    pPlayer->ApplyRenderSlotPose();

    memmove(&pPlayer->m_stats.m_aStats[1], &pTeam->aRows[nWith], sizeof(TeamPlayerRow));
    memmove(&pPlayer->m_matchStats, &pTeam->aStatRows[nWith], sizeof(TeamStatRow));

    nOnIceSlot = pPlayer->GetSlot();

    pBody->ScaleTo(nOnIceSlot);

    pBody->m_aStats[0] = 100.0f;

    pTeam->aSeat[nSide] = nWith;

    return 1;
}

#define SLOT_TEAM_ONE_FIRST  0
#define SLOT_TEAM_TWO_FIRST  1
#define SLOT_TEAM_ONE_SECOND 2
#define SLOT_TEAM_TWO_SECOND 3

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
        case SLOT_TEAM_ONE_FIRST:
            pTeam  = m_pTeamA;
            nSelf  = 0;
            nOther = 1;
            break;

        case SLOT_TEAM_ONE_SECOND:
            pTeam  = m_pTeamA;
            nSelf  = 1;
            nOther = 0;
            break;

        case SLOT_TEAM_TWO_FIRST:
            nSelf  = 0;
            nOther = 1;
            pTeam  = m_pTeamB;
            break;

        case SLOT_TEAM_TWO_SECOND:
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

    PlayerObject* const pEntity = (PlayerObject*)pTeam->apOnIce[nOther];

    if (nWhich == pEntity->m_wDesiredSubstitute)
    {
        return 0;
    }

    if (DamageGate() < pTeam->aRows[nWhich].aStat[RATING_DAMAGE])
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

    int nSide = 0;

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

        if (pTeam != 0)
        {
            for (nSeat = 0; nSeat < TEAM_BLOCK_SEATS; nSeat++)
            {
                const int     nSlot   = pTeam->aSeat[nSeat];
                PlayerObject* pPlayer = (PlayerObject*)pTeam->apOnIce[nSeat];

                if (pPlayer != 0)
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

    const int nStart = GameApp::GetInstallLevel();

    switch (pDef->m_nTypeId)
    {
        case KIND_DROMER:
            pNew = new PlayerObject(nEntry, pDef);
            break;

        case KIND_ROK:
            pNew = new RokObject(nEntry, pDef);
            break;

        case KIND_MINE:
        case KIND_SWARF:
            pNew = new WeaponObject(nEntry, pDef);
            break;

        case KIND_GOAL:
            pNew = new GoalObject(nEntry, pDef);
            break;

        case KIND_CAMERA:
        {
            if (m_wCameraCount == CAMERA_LIMIT)
            {
                // STRING: HYPERX 0x004c84b8
                g_gameApp.Fatal("Too many cameras!");
            }

            CameraObject* const pCamera = new CameraObject(nEntry, pDef);

            m_apCameras[m_wCameraCount] = pCamera;

            pNew = pCamera;

            m_wCameraCount++;
            break;
        }

        case KIND_KEYBOARD:
            if (m_wDeviceCount == DEVICE_LIMIT)
            {
                // STRING: HYPERX 0x004c84a0
                g_gameApp.Fatal("Too many input devices!");
            }

            pNew = new KeyboardDeviceObj(nEntry, pDef);

            m_apDevices[m_wDeviceCount] = pNew;

            m_wDeviceCount++;

            g_pInputDevice = (class KeyboardDeviceObj*)pNew;
            break;

        case KIND_GAMEPAD:
            if (m_wDeviceCount == DEVICE_LIMIT)
            {
                g_gameApp.Fatal("Too many input devices!");
            }

            pNew = new GamepadDeviceObj(nEntry, pDef);

            m_apDevices[m_wDeviceCount] = pNew;

            m_wDeviceCount++;
            break;

        case KIND_JOYSTICK:
            if (m_wDeviceCount == DEVICE_LIMIT)
            {
                g_gameApp.Fatal("Too many input devices!");
            }

            pNew = new JoystickDeviceObj(nEntry, pDef);

            m_apDevices[m_wDeviceCount] = pNew;

            m_wDeviceCount++;
            break;

        case KIND_MOUSE:
            if (m_wDeviceCount == DEVICE_LIMIT)
            {
                g_gameApp.Fatal("Too many input devices!");
            }

            pNew = new MouseDeviceObj(nEntry, pDef);

            m_apDevices[m_wDeviceCount] = pNew;

            m_wDeviceCount++;
            break;

        case KIND_DROME:
            pNew = new DromeObject(nEntry, pDef);
            break;

        case KIND_RAMP:
            pNew = new RampObject(nEntry, pDef);
            break;

        case KIND_EXPLOSION:
            pNew = new ParticleSystem(nEntry, pDef);
            break;

        case KIND_COACH:
            pNew = new CoachObject(nEntry, pDef);
            break;

        default:
            pNew = new EngineObject(nEntry, pDef);
            break;
    }

    RegisterObject(nEntry, pNew);

    pNew->m_installCost.nLevels += GameApp::GetInstallLevel() - nStart;

    m_nLoadCost -= pNew->m_installCost.nLevels;

    if (nEntry != SLOT_DROME)
    {
        InsertObjectSorted((short)nEntry);
    }

    return pNew;
}

// The length of both slot tables, m_ppSlots and m_ppObjects.
#define ENGINE_SLOT_COUNT 0x66

// The first slot array is the camera holder; stubbing it faults the graphics set-up.
// FUNCTION: HYPERX 0x00486680
void GameEngine::Init(const char* pszConfig)
{
    int nIndex;

    ArenaState* const pState = new ArenaState;

    m_pArena = pState;

    m_nViewedSlot = 0;

    for (nIndex = 0; nIndex < id_GOB_NUM_OF_TYPES; nIndex++)
    {
        m_abTypeHighWater[nIndex] = g_abObjectTypeBaseline[nIndex];
    }

    InitMatchTimers();

    m_flGravityDt = g_flGravityGameEngine * m_clock.flDeltaSquared;

    InitPlanSystems();

    // The DirectPlay layer's last-error word; the original's session set-up writes it.
    g_nDPlayLastError = -1;
    m_nPending        = -1;

    EngineObject** const ppSlots = new EngineObject*[ENGINE_SLOT_COUNT];

    m_ppSlots = ppSlots;

    memset(ppSlots, 0, ENGINE_SLOT_COUNT * sizeof(EngineObject*));

    EngineObject** const ppObjects = new EngineObject*[ENGINE_SLOT_COUNT];

    m_ppObjects = ppObjects;

    memset(ppObjects, 0, ENGINE_SLOT_COUNT * sizeof(EngineObject*));

    m_nLoadedCount = 0;
    m_nObjectCount = 0;

    LoadConfigFile(pszConfig);

    // STRING: HYPERX 0x004b3f38
    LoadObjectsFromConfig("common.cfg");

    SyncRenderQueueSize();

    m_apTracked[0] = m_ppSlots[0];
    m_apTracked[1] = m_ppSlots[0];

    if (g_bNetworkGame != 0)
    {
        g_anNetworkSlotState[2] = 0;
        g_anNetworkSlotState[1] = 0;
        g_anNetworkSlotState[0] = 0;
        g_anNetworkSlotState[6] = 0;
        g_anNetworkSlotState[5] = 0;
        g_anNetworkSlotState[4] = 0;
        g_anNetworkSlotState[3] = 0;
    }
}

// FUNCTION: HYPERX 0x004867b0
char GameEngine::SyncRenderQueueSize()
{
    if (m_nQueuedSlotCount == m_nSlotCount)
    {
        return 0;
    }

    CameraObject** ppCamera   = m_apCameras;
    int            nRemaining = CAMERA_LIMIT;

    do
    {
        if (*ppCamera != 0)
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

    m_wRunState = ENGINE_RUN_PLAYING;

    if (m_pTeamA == 0 || m_pTeamB == 0)
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
    EngineObject**      ppSlot  = &m_ppSlots[nSlot];
    EngineObject* const pObject = *ppSlot;

    m_nLoadedCount--;

    *ppSlot = 0;

    m_nSlotCount -= pObject->GetComponentCount();
}

// The first overlap found is processed and ends the walk.
// FUNCTION: HYPERX 0x004868f0
void GameEngine::DispatchCollisionToRule(CollInteractor* pChain)
{
    CollInteractor* pNode = pChain;

    while (pNode != 0)
    {
        if (pNode->GetActiveState() == 1 && pNode->GetActor()->GetPhase() == OBJECT_PHASE_LOCAL)
        {
            const short nOtherSlot = pNode->GetOtherSlot();
            short       nModelSlot = pNode->GetModelSlot();

            if (nOtherSlot >= 0 && nModelSlot >= 0)
            {
                BoundBubble* const pModelBubble = GetColObjPtr(nModelSlot);
                BoundBubble* const pOtherBubble = GetColObjPtr(nOtherSlot);

                if (pOtherBubble->SphereSphereOverlap(pModelBubble) != 0)
                {
                    short nOutcome = DECISION_LOG_FRESH;

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
    short               nIndex;
    static unsigned int s_dwEventBase = (DWORD)SDL_GetTicks();

    if (g_bDragOff == 0 || nEvent != MSG_START_DRAG)
    {
        for (nIndex = 0; nIndex < m_nObjectCount; nIndex++)
        {
            if (m_ppObjects[nIndex]->GetSlot() != nSender)
            {
                m_ppObjects[nIndex]->PostEvent(nEvent, nParam, nSender);
            }
        }
    }

    if (nEvent == MSG_GOAL_SCORED)
    {
        s_dwEventBase = (DWORD)SDL_GetTicks();

        m_clock.nPauseMark = 0;
    }
    else if (nEvent == MSG_START_DRAG)
    {
        s_dwEventBase = (DWORD)SDL_GetTicks();

        m_clock.nPeriodStartMs = s_dwEventBase;

        m_clock.nPauseStartMs = s_dwEventBase;

        m_clock.nPauseMark = 0;
    }
    else if (nEvent == MSG_SHOOT)
    {
        if (m_clock.nPauseMark == 0)
        {
            m_clock.nPauseStartMs = (DWORD)SDL_GetTicks();

            m_clock.nPeriodStartMs += m_clock.nPauseStartMs - s_dwEventBase;
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
        EngineObject* const pObject = *ppObject;

        ppObject++;

        pObject->m_nFlags &= ~ENGINEOBJECT_POSED;
    }

    switch (m_wRunState)
    {
        case ENGINE_RUN_RESTART:
            GameEngine::RestartMatch();

            GameEngine::TogglePause(g_gameApp.m_hWnd, 1);

            return;

        case ENGINE_RUN_PAUSED:
            GetCamera(CAMERA_MAIN_SLOT)->ViewIt();

            break;

        case ENGINE_RUN_PLAYING:
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

            if (g_pGameSession != 0)
            {
                if (g_pGameSession->PumpNetworkCommands() == 0)
                {
                    GameSession::Disconnect();
                }

                if (g_pGameSession != 0)
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
        const short wFromSub = (*ppObject)->GetPlanState()->m_pCurrent->m_wSyncValue;

        PlanState* const pDetail = (*ppObject)->GetPlanState();

        EngineObject* const pObject = *ppObject;

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

    short               nCount  = m_nObjectCount;
    EngineObject* const pObject = m_ppSlots[nId];

    for (i = 0; i < nCount; i++)
    {
        if (m_ppObjects[i]->GetSlotWord() == nId)
        {
            return 0;
        }
    }

    const int nClass = pObject->m_nKind;

    if (nClass == KIND_KEYBOARD || nClass == KIND_GAMEPAD || nClass == KIND_JOYSTICK || nClass == KIND_MOUSE)
    {
        for (j = nCount; j > 0; j--)
        {
            m_ppObjects[j] = m_ppObjects[j - 1];
        }

        m_ppObjects[0] = pObject;
    }
    else if (nClass == KIND_DROMER && nCount > 5)
    {
        // The count is read again here: the search above ran against the old one.
        short nSlot = pObject->GetSlot();
        nCount      = m_nObjectCount;
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
        EngineObject* const pEntry = m_ppObjects[i];

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
    static unsigned int s_dwPauseBase = (DWORD)SDL_GetTicks();

    switch (bRunning)
    {
        case 1:
            if (m_clock.nPauseMark == -1)
            {
                m_clock.nPauseMark = 1;

                m_clock.nPauseStartMs = (DWORD)SDL_GetTicks();

                m_clock.nPeriodStartMs += m_clock.nPauseStartMs - s_dwPauseBase;
            }

            m_wRunState = ENGINE_RUN_PLAYING;

            SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
            SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

            SDL_DisableScreenSaver();
            break;

        case 0:
            s_dwPauseBase = (DWORD)SDL_GetTicks();

            m_wRunState = ENGINE_RUN_PAUSED;

            SetPriorityClass(GetCurrentProcess(), NORMAL_PRIORITY_CLASS);
            SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);

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
    for (short nSlot = 0; nSlot < ENGINE_SLOT_COUNT; nSlot++)
    {
        EngineObject* const pObject = m_ppSlots[nSlot];

        if (pObject != 0)
        {
            delete pObject;

            m_nLoadedCount--;
        }
    }

    delete[] m_ppSlots;
    delete[] m_ppObjects;

    TeamBlock* pTeam = m_pTeamA;

    delete pTeam;

    pTeam = m_pTeamB;

    delete pTeam;

    ArenaState* const pArena = m_pArena;

    if (pArena != 0)
    {
        if (pArena->m_pnSlotIds != 0)
        {
            delete[] pArena->m_pnSlotIds;
        }

        delete pArena->m_pRokDef;

        delete pArena;
    }

    BehaviorRegistry::DestroyAll();
    GameEngine::DestroyAgendaItemPools();
    PlanInteractorFactories::DestroyAll();

    SaveObject* const pSave = m_pSaveObject;

    if (pSave != 0)
    {
        delete pSave;
    }
}

// FUNCTION: HYPERX 0x00486fb0
void GameEngine::ReplaceEntity(int nKind, ObjectDef* pDef)
{
    EngineObject* const pObject = m_ppSlots[(short)nKind];

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

    if (m_clock.nPeriod == FIRST_PERIOD)
    {
        Reset();

        // No active score line.
        g_wActiveScoreLine = -1;

        if (IsTournamentMode() != 0)
        {
            for (nRow = 0; nRow < TEAM_ROW_COUNT; nRow++)
            {
                for (nSlot = 0; nSlot < MATCH_STAT_COUNT; nSlot++)
                {
                    m_pTeamA->aStatRows[nRow].aStat[nSlot] = 0.0f;
                }
            }

            PlayerObject* const pStatsB = GetPlayer(2);
            PlayerObject* const pStatsA = GetPlayer(0);

            for (nSlot = 0; nSlot < MATCH_STAT_COUNT; nSlot++)
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

    BroadcastEvent(MSG_START_DRAG, 0, SLOT_GOD);
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

    if (m_nPendingCount < PENDING_QUEUE_LENGTH)
    {
        m_nPendingCount++;
        m_aQueueMessage[m_nPendingCount] = MSG_NOMSG;

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
        EngineObject* const pObject = m_ppObjects[nIndex];
        CPlayer*            pLock   = pObject->GetPhysicsBody()->pPlayer;

        if (pLock != 0)
        {
            std::recursive_mutex* pSection = &pLock->m_lock;

            pSection->lock();
            char bQueued = pObject->DequeueInputCommand(&nCommand, &nPayload);
            pSection->unlock();

            while (bQueued != 0)
            {
                if (nCommand == COMMAND_SET_PLAN_AND_BEHAVIOR)
                {
                    pObject->InstantiateAIPlan(PlanBehaviourWord::GetPlan(nPayload));
                    pObject->GetPlanState()->ActivateBehavior(PlanBehaviourWord::GetBehaviour(nPayload));
                }
                else if (nCommand == COMMAND_SET_BEHAVIOR)
                {
                    pObject->GetPlanState()->ActivateBehavior((short)nPayload);
                }
                else if ((nCommand & MSG_EVENT_COLLISION_MARK) != 0)
                {
                    MsgEvent::Unpack(nCommand, &nKind, &nWantedOther, &nWantedOwn, &nLow);

                    CollInteractor* pNode = pObject->GetPlanState()->WalkLinkedList(nKind);

                    if (pNode != 0 && pNode->GetOtherSlot() == nWantedOther && pNode->GetModelSlot() == nWantedOwn)
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
        m_ppSlots[(short)m_aQueueSlot[nPending]]
            ->DispatchSceneMessage(m_aQueueMessage[nPending], m_aQueueArg[nPending], DECISION_LOG_FRESH, m_aQueueSender[nPending], m_aQueueFromInput[nPending]);
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
    m_aQueueMessage[0] = MSG_NOMSG;
}

// FUNCTION: HYPERX 0x00487480
int GameEngine::GetArmoryCount() const
{
    return m_abTypeHighWater[id_GOB_Armory] - g_abObjectTypeBaseline[id_GOB_Armory];
}

// FUNCTION: HYPERX 0x00487490
int GameEngine::GetSwarfCount() const
{
    return m_abTypeHighWater[id_GOB_Swarf] - g_abObjectTypeBaseline[id_GOB_Swarf];
}

// FUNCTION: HYPERX 0x004874b0
int GameEngine::GetMineCount() const
{
    return m_abTypeHighWater[id_GOB_Mine] - g_abObjectTypeBaseline[id_GOB_Mine];
}

// FUNCTION: HYPERX 0x004874c0
int GameEngine::GetEntityCountOfType(int nType)
{
    if (nType >= 0 && nType < id_GOB_NUM_OF_TYPES)
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

#define TEAM_COUNT 2

// Writes only in tournament player mode.
// FUNCTION: HYPERX 0x00487590
char GameEngine::WriteSaveFile(int nSlot, const char* pszLabel)
{
    char                 szName[MAX_PATH];
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

    PathString strPath(String::Concat(g_gameApp.m_strExeDir.m_str, szName).m_pchData, MAX_PATH);

    if (file.m_file.Open(strPath.m_str.m_pchData, FILE_MODE_WRITE) == 0)
    {
        return 0;
    }

    memset(&block, 0, sizeof(block));

    const int nLength = (int)strlen(pszLabel);

    block.nTag = SAVE_BLOCK_TAG_VALUE;

    if (nLength > 0)
    {
        memcpy(block.szLabel, pszLabel, nLength);
    }

    block.nBracketPos = m_pSaveObject->GetBracketPos();
    block.nDifficulty = m_nDifficulty;

    for (nTeam = 0; nTeam < TEAM_COUNT; nTeam++)
    {
        if (nTeam == 0)
        {
            pTeam = m_pTeamA;
            pDest = &block.teamOne;

            pDest->nTeamIndex = (short)m_nTeamHomeIndex;
        }
        else
        {
            pTeam = m_pTeamB;
            pDest = &block.teamTwo;

            pDest->nTeamIndex = (short)m_nTeamAwayIndex;
        }

        for (nSeat = 0; nSeat < TEAM_BLOCK_SEATS; nSeat++)
        {
            memmove(&pTeam->aStatRows[pTeam->aSeat[nSeat]],
                    &((PlayerObject*)pTeam->apOnIce[nSeat])->m_matchStats,
                    sizeof(TeamStatRow));
        }

        TeamStatRow* pSource = pTeam->aStatRows;
        TeamStatRow* pRow    = pDest->aStatRows;

        for (nRow = 0; nRow < TEAM_ROW_COUNT; nRow++)
        {
            const float* pWalk = pSource->aStat;
            float*       pTo   = pRow->aStat;

            for (nIndex = 0; nIndex < MATCH_STAT_COUNT; nIndex++)
            {
                *pTo = *pWalk;
                pTo++;
                pWalk++;
            }

            pSource++;
            pRow++;
        }

        for (nIndex = 0, pReserve = pTeam->aRows, pStat = pDest->aDamage;
             nIndex < TEAM_ROW_COUNT;
             nIndex++, pStat++, pReserve++)
        {
            if (pTeam->aSeat[0] == nIndex)
            {
                pObject = g_pGameEngine->GetPlayer(0);

                *pStat = pObject->m_stats.GetScalar(RATING_DAMAGE);
            }
            else if (pTeam->aSeat[1] == nIndex)
            {
                pObject = g_pGameEngine->GetPlayer(2);

                *pStat = pObject->m_stats.GetScalar(RATING_DAMAGE);
            }
            else
            {
                *pStat = pReserve->aStat[RATING_DAMAGE];
            }
        }

        pDest->aSeat[0] = pTeam->aSeat[0];
        pDest->aSeat[1] = pTeam->aSeat[1];
    }

    memcpy(block.abAward, m_abAward, sizeof(m_abAward));

    if (file.m_file.Write((char*)&block, sizeof(block)) == sizeof(block))
    {
        return 1;
    }

    return 0;
}

#define ARENA_NONE ((unsigned short)0xffff)

// A team's first seat is its slot; every other seat shares the slot two above it.
inline int GameEngine::SubstituteSlot(int nTeam, int nSeat)
{
    int nSlot = nTeam;

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
    char           szName[MAX_PATH];
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

    PathString strPath(String::Concat(g_gameApp.m_strExeDir.m_str, szName).m_pchData, MAX_PATH);

    if (file.m_file.Open(strPath.m_str.m_pchData, FILE_MODE_READ) == 0)
    {
        return 0;
    }

    if (file.m_file.Read(&block, sizeof(block)) != sizeof(block))
    {
        return 0;
    }

    if (block.nTag == SAVE_BLOCK_TAG_VALUE)
    {
        SetDifficultyLevel(block.nDifficulty);

        m_pSaveObject->SetBracketPos((short)block.nBracketPos);

        ClearPendingEvents();

        m_bMatchOver = 1;

        SetTeams((short)block.teamOne.nTeamIndex, (short)block.teamTwo.nTeamIndex);

        m_nArenaIndex = ARENA_NONE;

        SetArena((short)block.teamTwo.nTeamIndex);

        for (short nObject = 0; nObject < m_nObjectCount; nObject++)
        {
            m_ppObjects[nObject]->SelectAndActivatePlan();
        }

        for (nIndex = 0; nIndex < TEAM_BLOCK_SEATS; nIndex++)
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

        for (nTeam = 0; nTeam < TEAM_COUNT; nTeam++)
        {
            if (nTeam == 0)
            {
                nWord   = block.teamOne.nTeamIndex;
                pTeam   = m_pTeamA;
                pRecord = &block.teamOne;

                g_nDromeTeamHome = nWord;

                m_pSaveObject->ValidateSetHomeTeam((short)nWord, 1);
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

            for (nRow = 0; nRow < TEAM_ROW_COUNT; nRow++)
            {
                const float* pWalk = pRow->aStat;
                float*       pTo   = pDest->aStat;

                for (nIndex = 0; nIndex < MATCH_STAT_COUNT; nIndex++)
                {
                    *pTo = *pWalk;
                    pTo++;
                    pWalk++;
                }

                pDest++;
                pRow++;
            }

            for (nSeat = 0; nSeat < TEAM_BLOCK_SEATS; nSeat++)
            {
                memmove(&((PlayerObject*)pTeam->apOnIce[nSeat])->m_matchStats,
                        &pTeam->aStatRows[pTeam->aSeat[nSeat]],
                        sizeof(TeamStatRow));
            }

            for (nStat = 0, pReserve = pTeam->aRows, pStat = pRecord->aDamage;
                 nStat < TEAM_ROW_COUNT;
                 nStat++, pStat++, pReserve++)
            {
                if ((unsigned int)pRecord->aSeat[0] == nStat)
                {
                    ((PlayerObject*)pTeam->apOnIce[0])->m_stats.m_aStats[RATING_SLOT(RATING_DAMAGE)] = *pStat;
                }
                else if ((unsigned int)pRecord->aSeat[1] == nStat)
                {
                    ((PlayerObject*)pTeam->apOnIce[1])->m_stats.m_aStats[RATING_SLOT(RATING_DAMAGE)] = *pStat;
                }
                else
                {
                    pReserve->aStat[RATING_DAMAGE] = *pStat;
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
    char      szName[MAX_PATH];
    SaveBlock block;
    int       nCopied;

    *pszLabel = 0;

    OwnedFile file;

    // STRING: HYPERX 0x004c84f4
    sprintf(szName, "SAVE%02d.BIN", nSlot);

    PathString strPath(String::Concat(g_gameApp.m_strExeDir.m_str, szName).m_pchData,
                       MAX_PATH);

    if (file.m_file.Open(strPath.m_str.m_pchData, FILE_MODE_READ) != 0 && file.m_file.Read(&block, sizeof(block)) == sizeof(block) && block.nTag == SAVE_BLOCK_TAG_VALUE)
    {
        nMax--;

        for (nCopied = 0; nCopied < nMax; nCopied++)
        {
            const char ch = block.szLabel[nCopied];

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

#define SLOT_ORDER_COUNT 4

// FUNCTION: HYPERX 0x00487e50
int GameEngine::GetDifficultyLevel() const
{
    return m_nDifficulty;
}

// RATING_DAMAGE is never copied; a level above two answers no.
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

    if (nLevel > DIFFICULTY_MAX)
    {
        return 0;
    }

    if (m_nDifficulty == (int)nLevel)
    {
        return 1;
    }

    g_pGameOptions->WriteDifficulty(nLevel);

    for (nTeam = 0; nTeam < TEAM_COUNT; nTeam++)
    {
        if (nTeam == 0)
        {
            pTeam = m_pTeamA;
        }
        else
        {
            pTeam = m_pTeamB;
        }

        for (nOrder = 0; nOrder < SLOT_ORDER_COUNT; nOrder++)
        {
            pRecord = pTeam->apDefs[g_anTeamSlotOrder[nOrder]];

            if (pRecord != 0)
            {
                pStats = pRecord->m_apRatings[nLevel];

                if (pStats != 0)
                {
                    if (nOrder < TEAM_BLOCK_SEATS && pTeam->aSeat[nOrder] == g_anTeamSlotOrder[nOrder])
                    {
                        pObject = (PlayerObject*)pTeam->apOnIce[nOrder];

                        pObject->m_stats.CopyBaseStats();

                        for (nIndex = 0; nIndex < RATING_COUNT; nIndex++)
                        {
                            if (nIndex != RATING_DAMAGE)
                            {
                                pObject->m_stats.m_aStats[RATING_SLOT(nIndex)] = pStats[nIndex];
                            }
                        }

                        pObject->m_stats.ScaleTo(pObject->GetSlot());
                    }
                    else
                    {
                        for (nIndex = 0; nIndex < RATING_COUNT; nIndex++)
                        {
                            if (nIndex != RATING_DAMAGE)
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

        if (pRecord != 0)
        {
            pStats = pRecord->m_apRatings[RATINGS_SPECIAL_GOALIE];

            if (pStats != 0)
            {
                pObject = (PlayerObject*)pTeam->apOnIce[2];

                for (nIndex = 0; nIndex < GOALIE_RATING_COUNT; nIndex++)
                {
                    pObject->m_stats.m_aStats[RATING_SLOT(nIndex)] = pStats[nIndex];
                }
            }
        }
    }

    m_nDifficulty = nLevel;

    return 1;
}

// FUNCTION: HYPERX 0x00488010
unsigned char GameEngine::IsTournamentMode()
{
    return g_nPlayerMode == PLAYER_MODE_TOURNAMENT;
}

// FUNCTION: HYPERX 0x00488020
unsigned char GameEngine::IsExhibitionMode()
{
    return g_nPlayerMode == PLAYER_MODE_EXHIBITION;
}

// The smallest drawn area is half the buffer, and each step adds a twelfth.
#define SCREEN_SIZE_MAX     6
#define SCREEN_TWELFTHS_MIN 6

// The drawn area is (step + 6) twelfths of the buffer, centred.
// FUNCTION: HYPERX 0x00488030
void GameEngine::SetScreenSize(int nStep)
{
    LayoutState state;

    if (nStep < 0)
    {
        nStep = 0;
    }
    else if (SCREEN_SIZE_MAX < nStep)
    {
        nStep = SCREEN_SIZE_MAX;
    }

    if (nStep != m_nScreenSize)
    {
        m_nScreenSize = nStep;

        g_pGameOptions->WriteScreenSize(nStep);

        g_pScene = 0;

        const int nBufferWidth = g_nBackBufferWidth;

        const int nTwelfths = nStep + SCREEN_TWELFTHS_MIN;

        const int nWidth  = nBufferWidth * nTwelfths / 12;
        const int nHeight = g_nBackBufferHeight * nTwelfths / 12;

        CameraObject* const pCamera = GetCamera(CAMERA_MAIN_SLOT);

        if (pCamera != 0)
        {
            const int nMarginX = (nBufferWidth - nWidth) / 2;
            const int nMarginY = (g_nBackBufferHeight - nHeight) / 2;

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
    if (nValue != m_nPeriodLengthMs)
    {
        m_nPeriodLengthMs = nValue;

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

    float flTotal = 0.0f;

    char bEligible = 1;
    int  nCounted  = 0;

    for (nRow = 0; nRow < TEAM_ROW_COUNT; nRow++)
    {
        const TeamStatRow* const pRow = &g_pGameEngine->m_pTeamA->aStatRows[nRow];

        if (pRow->aStat[MATCH_STAT_SHOTS_ON_GOAL] > 0.0f)
        {
            flTotal += pRow->aStat[MATCH_STAT_SHOOTING_PCT];
            nCounted++;
        }
    }

    if (flTotal / nCounted < STAT_MINIMUM)
    {
        bEligible = 0;
    }

    if (HasAward(AWARD_ARCHER) == 0)
    {
        SetAward(AWARD_ARCHER, bEligible);
    }

    return bEligible;
}

// FUNCTION: HYPERX 0x00488270
char GameEngine::EvalAnvilAward()
{
    int nRemaining;

    char bEligible = 1;

    for (nRemaining = 0; nRemaining < TEAM_ROW_COUNT; nRemaining++)
    {
        if (g_pGameEngine->m_pTeamB->aRows[nRemaining].aStat[RATING_DAMAGE] != 0.0f)
        {
            bEligible = 0;
        }
    }

    if (HasAward(AWARD_ANVIL) == 0)
    {
        SetAward(AWARD_ANVIL, bEligible);
    }

    return bEligible;
}

// The particle systems start sixty slots in; a count of one or less finds nothing.
// FUNCTION: HYPERX 0x004882d0
ParticleSystem* GameEngine::FindFreeParticleSystem()
{
    int iSlot;

    const int nCount = GetEntityCountOfType(id_GOB_Explosion);

    for (iSlot = 1; iSlot < nCount; iSlot++)
    {
        ParticleSystem* pEntity = GetParticleSystem(SLOT_EXPLOSION + iSlot);

        if (pEntity != 0 && pEntity->m_bAnyAlive == 0)
        {
            return pEntity;
        }
    }

    return 0;
}

// FUNCTION: HYPERX 0x00488310
void GameEngine::DeactivateParticleSystems()
{
    const int nCount = GetEntityCountOfType(id_GOB_Explosion);
    int       nIndex;

    for (nIndex = 0; nIndex < nCount; nIndex++)
    {
        ParticleSystem* const pSystem = GetParticleSystem(SLOT_EXPLOSION + nIndex);

        if (pSystem != 0 && pSystem->m_bAnyAlive != 0)
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

// Empty; folded with every empty two-argument body, so its call names Screen::OnWidgetMessage.
void GameEngine::SetInstance(HINSTANCE, int)
{
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
