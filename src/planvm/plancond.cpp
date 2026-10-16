#include "common/compat.h"
#include "anim/animdb.h"
#include "planvm/behaviorids.h"
#include "physics/physicsbody.h"
#include "physics/boundbubble.h"
#include "engine/devicector.h"
#include "engine/gameengine.h"
#include "globals.h"
#include "ui/hudglobals.h"
#include "planvm.h"
#include "engine/objectclasses.h"
#include "plannode.h"
#include "anim/animbinding.h"
#include "damagegate.h"
#include "input/keyboarddeviceobj.h"
#include "engine/cameraobject.h"
#include "planvmglobals.h"
#include "common/staticinit.h"
#include "math/vector.h"
#include "graphics/renderer.h"
#include "graphics/infobarradar.h"
#include "planvm/agendaitem.h"
#include "planvm/cbungeetoward.h"
#include "engine/csetpositionrelative.h"
#include "engine/objectstats.h"
#include "common/pooledclasses.h"
#include "axisframe.h"
#include "engine/statids.h"
#include "engine/messageids.h"
#include "net/session.h"
#include "engine/possessionsyncword.h"
#include "engine/systemsyncword.h"
#include "engine/teamplayerrow.h"
#include "engine/objectdef.h"
#include "anim/animregistry.h"
#include "app/gameapp.h"
#include "engine/playerobject.h"
#include "engine/rokobject.h"
#include "anim/animplayer.h"
#include "net/cplayer.h"
#include "physics/collisionobj.h"
#include <math.h>
#include <string.h>
#include <mbstring.h>
#include <stdlib.h>
#include "engine/engineobject.h"
#include "anim/animset.h"
#include "planvm/planaction.h"
#include "planvm/plancond.h"
#include "planvm/planutil.h"
#include "engine/player.h"
#include "math/floatbits.h"
#include "planvm/ctracktraj.h"
#include "planvm/csteertotarget.h"
#include "planvm/csteertoward.h"
#include "engine/weaponobject.h"
#include "engine/objectkind.h"
#include "engine/particle.h"
#include "engine/crelaxyaw.h"
#include "engine/cspeedburst.h"
#include "planvm/cpaletteeffect.h"
#include "graphics/meshbone.h"
#include "math/transformobject.h"
#include "physics/upaxis.h"
#include "graphics/meshboundingbox.h"
#include "engine/teamblock.h"
#include "common/stub.h"
#include "planarg.h"
#include "engine/messagequeue.h"
#include "physics/collisionellipsoid.h"
#include "graphics/colorpalette.h"
#include "graphics/meshmaterial.h"
#include "graphics/modeldatabase.h"
#include "engine/rampobject.h"
#include "ui/infobarbase.h"
#include "ui/infobar.h"
#include "app/bringup.h"
#include "engine/particlesystem.h"
#include "physics/collisionplane.h"
#include "physics/collisioncheck.h"
#include "engine/object.h"
#include "engine/objectphase.h"
#include "planvm/decisionlog.h"

#define ENTITY_SLOT_LIMIT 0x66

#define JUDGE_STATE_LOW 0

#define JUDGE_STATE_HIGH 8

#define ARG_COUNT_WITH_XY 3

#define ARG_COUNT_WITH_Z 4

#define ARG_OBJECT 1
#define ARG_FIRST  1

// One, the plan files' own copy. src/globals.h's g_flOne is at a different address.
// GLOBAL: HYPERX 0x004b36ec
extern const float g_flOnePlan;

// The velocity is fifty plus a roll of fifty, scaled by the frame delta, at a fixed climb.
#define SCATTER_ROLL  50
#define SCATTER_FLOOR 50.0f
#define SCATTER_CLIMB 0.75f

// Slots alternate teams, so the other side's two players sit at the flipped parity and two past it.
#define OPPOSING_FIRST(nKind)  ((unsigned char)~(nKind) & 1)
#define OPPOSING_SECOND(nKind) (OPPOSING_FIRST(nKind) | 2)

#define APPROACH_ACCURACY_WEIGHT 0.1f
#define APPROACH_RANGE_GATE      15.0f
#define APPROACH_TIME_GATE       2.0f

// Inside this the two are close enough to matter.
#define INTERACT_RANGE      7.5f
#define INTERACT_SCALE_HALF 0.5f

// FUNCTION: HYPERX 0x004015e0
void __cdecl PlanCond::Decide_Attachment(Interactor* pContext, PlanNode*, short* pResult, int)
{
    EngineObject* pRange;
    EngineObject* pOther;
    float         flDot;
    Vec3          vDelta;
    Vec3          vPoint;

    const short         nSlot  = pContext->GetOtherSlot();
    EngineObject* const pActor = pContext->GetActor();

    pRange = g_pGameEngine->m_ppSlots[(short)CollisionObj::GetModelId(nSlot)];

    if (pContext->GetActor()->GetPhysicsBody()->pAttach != 0)
    {
        pOther = g_pGameEngine->m_ppSlots[pContext->GetActor()->GetPhysicsBody()->pAttach->nSlot];

        if (*pResult < 0)
        {
            const Vec3* pForwardOther = &pOther->GetPhysicsBody()->velDir;
            const Vec3* pForwardRange = &pRange->GetPhysicsBody()->velDir;

            // Not GetColObjPtr: the client stores the dot between the load and its test (0x0040168a to 0x0040169c).
            BoundBubble* const pColObj = g_pGameEngine->m_ppColObjects[nSlot];

            flDot = pForwardRange->x * pForwardOther->x + pForwardRange->y * pForwardOther->y + pForwardRange->z * pForwardOther->z;

            if (pColObj == 0)
            {
                // STRING: HYPERX 0x004bb2b0
                g_gameApp.Fatal("ERROR: GetColObjPtr: NULL Pointer\n");
            }

            pColObj->GetWorldPosition(&vPoint);

            vDelta.x = pActor->GetPositionPtr()->x - vPoint.x;
            vDelta.y = pActor->GetPositionPtr()->y - vPoint.y;
            vDelta.z = pActor->GetPositionPtr()->z - vPoint.z;

            if ((float)sqrt(vDelta.x * vDelta.x + vDelta.y * vDelta.y + vDelta.z * vDelta.z) < INTERACT_RANGE)
            {
                if (((pRange->GetSlot() ^ pOther->GetSlot()) & 1) != 0)
                {
                    PhysicsBody* const pBodyOther = pOther->GetPhysicsBody();
                    PhysicsBody* const pBodyRange = pRange->GetPhysicsBody();

                    if (pBodyOther->GetSpeed() < (g_flChaseSpeedBias - flDot) * pBodyRange->flSpeed * INTERACT_SCALE_HALF)
                    {
                        *pResult = 0;
                        return;
                    }
                }
            }

            *pResult = 1;
        }
    }
    else
    {
        *pResult = 0;
    }
}

// Answers through the third argument; with no object in the slot it leaves it untouched.
// FUNCTION: HYPERX 0x00402130
void PlanCond::ForceRearView(Interactor*, PlanNode*, short* pResult, int)
{
    CameraObject* const pSub = g_pGameEngine->GetCamera(CAMERA_HEAD_SLOT);

    if (pSub != 0)
    {
        // The gap word at the end of CameraObject; this is its only reader.
        *pResult = pSub->m_bRearView != 0;
    }
}

// FUNCTION: HYPERX 0x00403b70
void __cdecl PlanCond::DecideGoalieLaunch(Interactor* pContext, PlanNode*, short* pResult, int)
{
    const Vec3* pGoalCentre;
    float       flTime;

    EngineObject* const pActor = pContext->GetActor();

    EngineObject* const pRok = g_pGameEngine->m_ppSlots[SLOT_ROK];

    pActor->GetSlot();
    const Vec3* pPosition = pActor->GetPositionPtr();
    const short nTeam     = pActor->GetSlot();

    if (nTeam % 2 == 0)
    {
        pGoalCentre = g_apPositionMarkers[POS_N_GOAL_CENTER];
    }
    else
    {
        pGoalCentre = g_apPositionMarkers[POS_S_GOAL_CENTER];
    }

    Vec3        vTriple  = pActor->GetPhysicsBody()->vel;
    const Vec3* pGoalPos = pRok->GetPositionPtr();

    const float flDx = pGoalCentre->x - pGoalPos->x;
    const float flDy = pGoalCentre->y - pGoalPos->y;
    const float flDz = pGoalCentre->z - pGoalPos->z;

    flTime = (float)sqrt(flDx * flDx + flDy * flDy + flDz * flDz) / (float)sqrt(vTriple.x * vTriple.x + vTriple.y * vTriple.y + vTriple.z * vTriple.z) * g_pGameEngine->m_clock.flDelta;

    vTriple.x = pGoalCentre->x - pPosition->x;
    vTriple.y = pGoalCentre->y - pPosition->y;
    vTriple.z = pGoalCentre->z - pPosition->z;

    PlayerObject* const pOwner = pContext->GetPlayerActor();

    const float flAccuracy = pOwner->m_stats.GetScalar(RATING_SHOT_ACCURACY);

    if (flAccuracy * APPROACH_ACCURACY_WEIGHT + flTime > APPROACH_TIME_GATE)
    {
        *pResult = 0;
        return;
    }

    if ((float)sqrt(vTriple.x * vTriple.x + vTriple.y * vTriple.y + vTriple.z * vTriple.z) > APPROACH_RANGE_GATE)
    {
        *pResult = 1;
        return;
    }

    *pResult = 2;
}

// Starts at one and is only cleared; OffWing, Defense and Sideln are rejected by name.

// FUNCTION: HYPERX 0x00404590
void __cdecl PlanCond::OkToLaunchRok(Interactor* pContext, PlanNode*, short* pOutValid, int)
{
    *pOutValid = 1;

    if (pContext->GetNamedSlot() >= 0)
    {
        pContext->GetActor();
    }
    else
    {
        CollisionObj::GetModelId(pContext->GetModelSlot());
    }

    short nTarget = pContext->GetTargetSlot();

    char* const pszName = g_pGameEngine->m_ppSlots[nTarget]->GetPlanState()->m_pszName;

    if (pszName == 0
        // STRING: HYPERX 0x004bb38c
        || _strcmpi(pszName, "OffWing") == 0
        // STRING: HYPERX 0x004bb384
        || _strcmpi(pszName, "Defense") == 0
        // STRING: HYPERX 0x004bb37c
        || _strcmpi(pszName, "Sideln") == 0)
    {
        *pOutValid = 0;
    }

    AttachPak* const pHeld = pContext->GetActor()->GetPhysicsBody()->pAttach;

    if (pHeld != 0 && nTarget != pHeld->nSlot)
    {
        if (nTarget != pContext->GetActor()->GetSlot())
        {
            *pOutValid = 0;
        }
    }
}

// FUNCTION: HYPERX 0x0045e2f0
void PlanCond::SetInteractorName(EngineObject* pActor, PlanNode* pNode, GameBehaviorObject*, Interactor* pInteractor)
{
    const short nIndex = (short)pNode->m_pArgs[1].n;

    pActor->GetPlanState()->m_apNamedInteractors[nIndex] = pInteractor;
}

#define ARG_COUNT_DIRECT  2
#define ARG_COUNT_INDEXED 3

// A count of three indexes the named interactors only under a PLAN_ARG_INTERACTOR_NAME high half.
// FUNCTION: HYPERX 0x0045e320
void __cdecl PlanCond::SetActiveState(EngineObject* pActor, PlanNode* pArgs, GameBehaviorObject*, Interactor* pInteractor)
{
    const short nArgCount = pArgs->m_nArgCount;

    if (nArgCount == ARG_COUNT_DIRECT)
    {
        pInteractor->SetActiveState(pArgs->m_pArgs[1].n);
        return;
    }

    if (nArgCount == ARG_COUNT_INDEXED)
    {
        if (!pArgs->m_pArgs[1].IsInteractorName())
        {
            return;
        }

        const short nIndex = (short)pArgs->m_pArgs[1].n;

        PlanState* const pList = pActor->GetPlanState();

        if (nIndex < pList->m_nNamedInteractors)
        {
            Interactor* const pEntry = pList->m_apNamedInteractors[nIndex];

            if (pEntry != 0)
            {
                pEntry->SetActiveState(pArgs->m_pArgs[2].n);
            }
        }
    }
}

// FUNCTION: HYPERX 0x0045e3f0
void PlanCond::Decide_Celebration(Interactor* pContext, PlanNode*, short* pResult, int)
{
    if (g_wActiveScoreLine == -1)
    {
        *pResult = 0;

        return;
    }

    EngineObject* const pActor = pContext->GetActor();

    *pResult = (short)(pActor->GetSlot() & 1) != g_wActiveScoreLine;
}

// FUNCTION: HYPERX 0x0045e430
void __cdecl PlanAction::LaunchGunDirection(Interactor* pContext, PlanNode*, short*, int)
{
    float flSign;
    float flDelta;

    Vec3* const pForward = &pContext->GetActor()->GetPhysicsBody()->velDir;

    if ((rand() & 1) == 0)
    {
        flSign = -g_flOnePlan;
    }
    else
    {
        flSign = g_flOnePlan;
    }

    pForward->x = flSign;
    pForward->y = g_flZeroPlan;
    pForward->z = SCATTER_CLIMB;

    Math::Vec3Normalize(pForward);

    flDelta = g_pGameEngine->m_clock.flDelta;

    const float flSpeed = ((float)(rand() % SCATTER_ROLL) + SCATTER_FLOOR) * flDelta;

    PhysicsBody* pBody = pContext->GetActor()->GetPhysicsBody();

    Vec3::ScaleTo(pForward, flSpeed, &pBody->vel);
}

// A failed build returns the agenda item to its pool, so it leaves nothing behind.

#define ARG_KIND     1
#define ARG_ANIM     2
#define ARG_START    3
#define ARG_DURATION 4

// FUNCTION: HYPERX 0x0045e4d0
void PlanAction::SetPaletteEffect(Interactor* pContext, PlanNode* pNode, short*, int)
{
    AgendaItem* const pAgendaItem = AllocAgendaItem(Agenda_CPaletteEffect);

    pAgendaItem->Bind(pContext->GetActor(), 0);

    PlanWord* const pArgs = pNode->m_pArgs;

    if (((CPaletteEffect*)pAgendaItem)
            ->CreateVisualEffect(pArgs[ARG_KIND].n, pArgs[ARG_ANIM].n, pArgs[ARG_START].pe, pArgs[ARG_DURATION].n) != 0)
    {
        pAgendaItem->Activate();

        return;
    }

    pAgendaItem->OnListChanged();
}

// FUNCTION: HYPERX 0x0045e530
void PlanAction::SetDeviceOwner(Interactor* pContext, PlanNode* pArgs, short*, int)
{
    EngineObject* pTarget;
    DeviceObject* pActor;

    const short nArgCount = pArgs->m_nArgCount;

    if (nArgCount == 1)
    {
        pTarget = g_pGameEngine->m_ppSlots[pContext->GetTargetSlot()];

        pActor = (DeviceObject*)pContext->GetActor();

        pActor->m_pOwner = pTarget;
    }
    else if (nArgCount == 2)
    {
        pTarget = g_pGameEngine->m_ppSlots[(short)pArgs->m_pArgs[1].n];

        pActor = (DeviceObject*)pContext->GetActor();

        pActor->m_pOwner = pTarget;
    }
}

// FUNCTION: HYPERX 0x0045e5a0
void PlanAction::SignalKeystate(Interactor*, PlanNode* pArgs, short*, int)
{
    const PlanWord* pBlock = pArgs->m_pArgs;

    g_pGameEngine->GetInputDevice((short)pBlock[1].n)->OnButtonDown((short)pBlock[2].n);
}

// A target of _MESSAGE_DATA_ uses the interpreter's value, and _BROADCAST_ reaches everyone.
// FUNCTION: HYPERX 0x0045e5d0
void __cdecl PlanAction::ReallySendMsg(Interactor* pContext, PlanNode* pArgs, short*, int nArg)
{
    unsigned int  nTarget;
    EngineObject* pActor;

    const PlanWord* pBlock = pArgs->m_pArgs;
    const int       nEvent = pBlock[2].n;

    if (pBlock[1].IsPronoun())
    {
        nTarget = g_apPlanStatFns[pBlock[1].wPronoun](pContext->GetActor(), pArgs);
    }
    else
    {
        nTarget = pBlock[1].n;
    }

    if (nTarget == PLAN_TOKEN_MESSAGE_DATA)
    {
        nTarget = nArg;
    }

    if (pArgs->m_nArgCount == 4 && (unsigned int)pArgs->m_pArgs[3].n != PLAN_TOKEN_MESSAGE_DATA)
    {
        nArg = pArgs->m_pArgs[3].n;
    }

#ifdef HYPERX_BRINGUP
    // tools/check_match.py reads the counts of every message a plan sends.
    BringUp::CountPlanMessage(nEvent);
#endif

    if (nTarget == PLAN_TOKEN_BROADCAST)
    {
        pActor = pContext->GetActor();

        g_pGameEngine->BroadcastEvent(nEvent, nArg, pActor->GetSlot());

        return;
    }

    EngineObject* const pTarget = g_pGameEngine->m_ppSlots[(short)nTarget];
    pActor                      = pContext->GetActor();

    pTarget->DispatchSceneMessage(nEvent, nArg, DECISION_LOG_FRESH, pActor->GetSlot(), 0);
}

// FUNCTION: HYPERX 0x0045e6a0
void __cdecl PlanAction::SendMsg(Interactor* pContext, PlanNode* pArgs, short*, int nParam)
{
    unsigned int  nTarget;
    EngineObject* pActor;

    const PlanWord* pBlock = pArgs->m_pArgs;
    const int       nEvent = pBlock[2].n;

    if (pBlock[1].IsPronoun())
    {
        nTarget = g_apPlanStatFns[pBlock[1].wPronoun](pContext->GetActor(), pArgs);
    }
    else
    {
        nTarget = pBlock[1].n;
    }

    if (nTarget == PLAN_TOKEN_MESSAGE_DATA)
    {
        nTarget = nParam;
    }

    if (pArgs->m_nArgCount == 4 && (unsigned int)pArgs->m_pArgs[3].n != PLAN_TOKEN_MESSAGE_DATA)
    {
        nParam = pArgs->m_pArgs[3].n;
    }

#ifdef HYPERX_BRINGUP
    BringUp::CountPlanMessage(nEvent);
#endif

    if (nTarget == PLAN_TOKEN_BROADCAST)
    {
        pActor = pContext->GetActor();

        g_pGameEngine->BroadcastEvent(nEvent, nParam, pActor->GetSlot());

        return;
    }

    EngineObject* const pTarget = g_pGameEngine->m_ppSlots[(short)nTarget];

    if (pTarget != 0)
    {
        pActor = pContext->GetActor();

        pTarget->PostSceneMessage(nEvent, nParam, pActor->GetSlot(), 0);
    }
}

#define POSSESSION_TIME_SCALE 100.0f

// FUNCTION: HYPERX 0x0045e770
void __cdecl PlanAction::SyncPossession(Interactor* pActor, PlanNode*, short*, int nArg)
{
    const PossessionSyncWord packed = { nArg };

    TeamBlock*    pTeam;
    PlayerObject* pObject;

    if (pActor->GetActor()->GetPhase() == OBJECT_PHASE_LOCAL)
    {
        return;
    }

    if (packed.GetAwayTeam() == 0)
    {
        pTeam = g_pGameEngine->m_pTeamA;
    }
    else
    {
        pTeam = g_pGameEngine->m_pTeamB;
    }

    const int          nRecord         = packed.GetRow();
    const int          nSide           = packed.GetTeam();
    const int          nShotsOnGoal    = packed.GetShotsOnGoal();
    const unsigned int nPossessionTime = packed.GetPossessionTime();

    if (nRecord == pTeam->aSeat[0])
    {
        pObject = g_pGameEngine->GetPlayer((short)nSide);

        pObject->m_matchStats.SetAt(MATCH_STAT_TIME_OF_POSS, (float)nPossessionTime * POSSESSION_TIME_SCALE);
        pObject->m_matchStats.SetAt(MATCH_STAT_SHOTS_ON_GOAL, (float)nShotsOnGoal);
    }
    else if (nRecord == pTeam->aSeat[1])
    {
        pObject = g_pGameEngine->GetPlayer((short)(nSide + 2));

        pObject->m_matchStats.SetAt(MATCH_STAT_TIME_OF_POSS, (float)nPossessionTime * POSSESSION_TIME_SCALE);
        pObject->m_matchStats.SetAt(MATCH_STAT_SHOTS_ON_GOAL, (float)nShotsOnGoal);
    }
    else
    {
        const float flValue = (float)nPossessionTime;

        pTeam->aStatRows[nRecord].aStat[MATCH_STAT_TIME_OF_POSS]  = flValue * POSSESSION_TIME_SCALE;
        pTeam->aStatRows[nRecord].aStat[MATCH_STAT_SHOTS_ON_GOAL] = (float)nShotsOnGoal;
    }
}

// The pause must be asked for three frames running before the mark moves and is broadcast.

// How many frames running have asked for a pause state the engine is not in.
// GLOBAL: HYPERX 0x004cfa50
extern int g_nPauseRequestStreak;

#define PAUSE_MARK_NONE (-1)

#define PAUSE_STREAK_NEEDED 3

// The host packs the period's elapsed time in tenths of a second.
#define SYNC_ELAPSED_UNIT_MS 100

// Matches the host's clock by replaying the goal plans' broadcasts: a shot starts it, a goal stops it.
// FUNCTION: HYPERX 0x0045e890
void PlanAction::SyncSystem(Interactor* pContext, PlanNode*, short*, int nArg)
{
    const SystemSyncWord packed = { nArg };

    int nTotal;

    EngineObject* const pActor = pContext->GetActor();

    if (pActor->GetPhase() == OBJECT_PHASE_LOCAL)
    {
        return;
    }

    EngineClock* const pClock = &g_pGameEngine->m_clock;
    const int          nMark  = pClock->nPauseMark;

    if (nMark != PAUSE_MARK_NONE)
    {
        if (packed.GetPauseMark() != 0)
        {
            if (nMark != 1)
            {
                ++g_nPauseRequestStreak;

                if (g_nPauseRequestStreak >= PAUSE_STREAK_NEEDED)
                {
                    pClock->nPauseMark = 1;

                    g_pGameEngine->BroadcastEvent(MSG_SHOOT, 0, pActor->GetSlot());

                    g_nPauseRequestStreak = 0;
                }
            }
            else
            {
                g_nPauseRequestStreak = 0;
            }
        }
        else
        {
            if (nMark != 0)
            {
                ++g_nPauseRequestStreak;

                if (g_nPauseRequestStreak >= PAUSE_STREAK_NEEDED)
                {
                    pClock->nPauseMark = 0;

                    g_pGameEngine->BroadcastEvent(MSG_GOAL_SCORED, 0, pActor->GetSlot());

                    g_nPauseRequestStreak = 0;
                }
            }
            else
            {
                g_nPauseRequestStreak = 0;
            }
        }
    }

    nTotal                 = pClock->nPauseStartMs - packed.GetElapsed() * SYNC_ELAPSED_UNIT_MS;
    pClock->nPeriodStartMs = nTotal;

    g_pGameEngine->SetHomeScore(packed.GetHomeScore());
    g_pGameEngine->SetAwayScore(packed.GetAwayScore());
}

#define ARG_COUNT_WITH_SLOT 2

// FUNCTION: HYPERX 0x0045e980
void PlanAction::SyncDamage(Interactor* pContext, PlanNode*, short*, int nValue)
{
    PlayerObject* const pActor = pContext->GetPlayerActor();

    if (pActor->GetPhase() != OBJECT_PHASE_LOCAL)
    {
        pActor->m_stats.m_aStats[RATING_SLOT(RATING_DAMAGE)] = (float)(nValue & 0xff);
    }
}

// In a network session this machine must drive both the actor and the target.
// FUNCTION: HYPERX 0x0045e9c0
void __cdecl PlanAction::TransferViewOwnership(Interactor* pContext, PlanNode* pArgs, short*, int nSlot)
{
    int nIndex;

    EngineObject* pActor   = pContext->GetActor();
    const int     nOwnSlot = pActor->GetSlot();

    if (pArgs->m_nArgCount == ARG_COUNT_WITH_SLOT)
    {
        nIndex = pArgs->m_pArgs[1].n;

        if (nIndex == PLAN_TOKEN_MESSAGE_DATA)
        {
            nIndex = nSlot;
        }
    }
    else
    {
        nIndex = nOwnSlot;
    }

    pActor                = pContext->GetActor();
    EngineObject* pTarget = g_pGameEngine->m_ppSlots[(short)nIndex];

    if (pActor != 0 && pTarget != 0 && (g_bSessionActive == 0 || (pActor->GetPhase() == OBJECT_PHASE_LOCAL && pTarget->GetPhase() == OBJECT_PHASE_LOCAL)))
    {
        g_pGameEngine->m_nViewedSlot = (short)nIndex;

        g_pGameEngine->m_apTracked[0] = g_pGameEngine->m_ppSlots[(short)nIndex];
        g_pGameEngine->m_apTracked[1] = g_pGameEngine->m_ppSlots[(short)nIndex];

        if (nIndex == nOwnSlot)
        {
            g_pGameEngine->m_bFollowingOther = 0;

            return;
        }

        g_pGameEngine->m_bFollowingOther = 1;
    }
}

#define ARG_COUNT_WITH_VALUE 3

// In a network session the actor must be the viewed slot or driven by this machine.
// FUNCTION: HYPERX 0x0045ea90
void __cdecl PlanAction::SendMessageToMyCamera(Interactor* pContext, PlanNode* pArgs, short*, int nParam)
{
    EngineObject* pActor;
    int           nValue;

    const int nEvent = pArgs->m_pArgs[1].n;

    if (pArgs->m_nArgCount == ARG_COUNT_WITH_VALUE && pArgs->m_pArgs[2].n != PLAN_TOKEN_MESSAGE_DATA)
    {
        nValue = pArgs->m_pArgs[2].n;
    }
    else
    {
        nValue = nParam;
    }

    EngineObject* const pCamera = g_pGameEngine->m_ppSlots[CAMERA_MAIN_SLOT];

    if (pCamera == 0)
    {
        return;
    }

    if (g_bSessionActive != 0)
    {
        pActor              = pContext->GetActor();
        const short nViewed = g_pGameEngine->m_nViewedSlot;

        if (pActor->GetSlot() != nViewed)
        {
            pActor = pContext->GetActor();

            if (pActor->GetPhase() != OBJECT_PHASE_LOCAL)
            {
                return;
            }
        }
    }

    pActor = pContext->GetActor();

    pCamera->PostEvent(nEvent, nValue, pActor->GetSlot());
}

// The same word is both the gate and the divisor.
#define ARG_PERIOD 1

// A nought period removes the speed burst; any other starts one.
// FUNCTION: HYPERX 0x0045eb30
void __cdecl PlanAction::Set_SpeedBurst(Interactor* pContext, PlanNode* pNode, short*, int)
{
    float flScale;

    if (pNode->m_pArgs[ARG_PERIOD].n != 0)
    {
        CSpeedBurst* const pBurst = (CSpeedBurst*)AllocAgendaItem(Agenda_CSpeedBurst);

        if (pContext->GetPlayerActor()->m_stats.m_flSpeedRatio > 0.3)
        {
            flScale = g_flOnePlan;
        }
        else
        {
            flScale = g_flZeroPlan;
        }

        GameBehaviorObject* const pCurrentBehaviour = pContext->GetActor()->GetPlanState()->m_pCurrent;

        pBurst->Bind(pContext->GetActor(), pCurrentBehaviour);
        pBurst->Activate();

        pBurst->m_flTarget =
            (float)(80.0 / (unsigned int)pNode->m_pArgs[ARG_PERIOD].n * flScale + g_flOnePlan);

        *pBurst->m_pBoost = 1.05f;

        PhysicsBody* pFirst = pContext->GetActor()->GetPhysicsBody();

        Vec3* const pSpin  = &pFirst->axis;
        const float flSpin = pSpin->x + 0.25f;

        pSpin->x = flSpin;

        pFirst = pContext->GetActor()->GetPhysicsBody();

        pFirst->flFriction = 0.1f;

        pFirst = pContext->GetActor()->GetPhysicsBody();

        Vec3* const pVel = &pFirst->vel;

        const float flLen = (float)sqrt(pVel->x * pVel->x + pVel->y * pVel->y + pVel->z * pVel->z);

        if (flLen < 0.1)
        {
            pFirst                     = pContext->GetActor()->GetPhysicsBody();
            PhysicsBody* const pSecond = pContext->GetActor()->GetPhysicsBody();

            pSecond->vel = pFirst->velDir;
        }
    }
    else
    {
        PlanState* const pBlock = pContext->GetActor()->GetPlanState();

        pBlock->m_pCurrent->RemoveAgendaItem(Agenda_CSpeedBurst);
    }
}

#define ARG_COL_OBJECT 1
#define ARG_STAT       2

#define ARG_COUNT_ACTOR 2
#define ARG_COUNT_SLOT  3

// Tells whichever of the mine, the swarf and the Rok the actor carries to deploy.
// FUNCTION: HYPERX 0x0045ecc0
void __cdecl PlanAction::DeployWeapon(Interactor* pContext, PlanNode*, short*, int)
{
    EngineObject* pActor;
    AttachPak*    pSlot;
    short         nSlot;

    if (pContext->GetNamedSlot() == -1)
    {
        BoundBubble* const pCollision = g_pGameEngine->GetColObjPtr(pContext->GetModelSlot());

        pActor = pCollision->GetOwner();
    }
    else
    {
        pActor = pContext->GetActor();
    }

    EngineObject* pTracked = g_pGameEngine->m_ppSlots[SLOT_MINE];

    if (pTracked != 0)
    {
        pSlot = pTracked->GetPhysicsBody()->pAttach;

        if (pSlot != 0)
        {
            nSlot = pSlot->nSlot;

            if (nSlot < PLAYER_OBJECT_COUNT && nSlot == pActor->GetSlot())
            {
                pTracked->PostSceneEvent(MSG_DEPLOY_WEAPON, 0);
            }
        }
    }

    pTracked = g_pGameEngine->m_ppSlots[SLOT_SWARF];

    if (pTracked != 0)
    {
        pSlot = pTracked->GetPhysicsBody()->pAttach;

        if (pSlot != 0)
        {
            nSlot = pSlot->nSlot;

            if (nSlot < PLAYER_OBJECT_COUNT && nSlot == pActor->GetSlot())
            {
                pTracked->PostSceneEvent(MSG_DEPLOY_WEAPON, 0);
            }
        }
    }

    pTracked = g_pGameEngine->m_ppSlots[SLOT_ROK];

    if (pTracked != 0)
    {
        pSlot = pTracked->GetPhysicsBody()->pAttach;

        if (pSlot != 0)
        {
            nSlot = pSlot->nSlot;

            if (nSlot < PLAYER_OBJECT_COUNT && nSlot == pActor->GetSlot())
            {
                pTracked->PostSceneEvent(MSG_DEPLOY_WEAPON, 0);
            }
        }
    }
}

// Refuses a slot outside the players or the actor's own, writing minus one.
// FUNCTION: HYPERX 0x0045edd0
void __cdecl PlanAction::SetAttackee(Interactor* pContext, PlanNode* pArgs, short*, int)
{
    short nSlot = (short)pArgs->m_pArgs[1].n;

    if (nSlot < 0 || nSlot > GOALIE_SLOT_B)
    {
        nSlot = SLOT_NONE;
    }

    PlayerObject* const pActor = pContext->GetPlayerActor();

    if (pActor->GetSlot() == nSlot)
    {
        nSlot = SLOT_NONE;
    }

    pActor->m_wAttackeeSlot = nSlot;
}

// FUNCTION: HYPERX 0x0045ee20
void __cdecl PlanAction::SetKnockedDownState(Interactor* pContext, PlanNode* pArgs, short*, int nArg)
{
    const short nSlot = (short)nArg;

    PlayerObject* const pActor = pContext->GetPlayerActor();

    if (pArgs->m_nArgCount == ARG_COUNT_SLOT)
    {
        int nIndex = nSlot;

        if (pArgs->m_pArgs[2].n != PLAN_TOKEN_MESSAGE_DATA)
        {
            nIndex = (short)pArgs->m_pArgs[2].n;
        }

        PlayerObject* const pTarget = g_pGameEngine->GetPlayer(nIndex);

        if (pTarget != 0)
        {
            pTarget->m_nKnockedDownState = pArgs->m_pArgs[1].n;

            return;
        }
    }
    else if (pArgs->m_nArgCount == ARG_COUNT_ACTOR)
    {
        pActor->m_nKnockedDownState = pArgs->m_pArgs[1].n;
    }
}

// FUNCTION: HYPERX 0x0045ee80
void PlanAction::HoverAboveArmory(Interactor* pContext, PlanNode* pNode, short*, int)
{
    AxisFrame     f;
    EngineObject* pSource;
    PlanWord*     pArgs;

    short nSlot = SLOT_ARMORY;

    short nDistance = pContext->GetNamedSlot();

    if (nDistance == -1)
    {
        pArgs     = pNode->m_pArgs;
        pSource   = g_pGameEngine->m_ppSlots[(short)pArgs[1].n];
        nDistance = (short)pArgs[2].n;

        if (pNode->m_nArgCount == 4)
        {
            nSlot = (short)pArgs[3].n;
        }
    }
    else
    {
        pSource   = pContext->GetActor();
        pArgs     = pNode->m_pArgs;
        nDistance = (short)pArgs[1].n;

        if (pNode->m_nArgCount == 3)
        {
            nSlot = (short)pArgs[2].n;
        }
    }

    f.vResult.y = (float)nDistance;

    if (pSource != 0)
    {
        EngineObject* const pTarget = g_pGameEngine->m_ppSlots[nSlot];

        if (pTarget != 0)
        {
            f.vOffset.x = (pSource->m_transformC.m_a[0][2] + pSource->m_transformC.m_a[0][0]) * g_flZeroPlan + f.vResult.y * pSource->m_transformC.m_a[0][1];
            f.vOffset.y = (pSource->m_transformC.m_a[1][2] + pSource->m_transformC.m_a[1][0]) * g_flZeroPlan + f.vResult.y * pSource->m_transformC.m_a[1][1];
            f.vOffset.z = (pSource->m_transformC.m_a[2][2] + pSource->m_transformC.m_a[2][0]) * g_flZeroPlan + f.vResult.y * pSource->m_transformC.m_a[2][1];

            Vec3* const pBasePos = pTarget->GetPositionPtr();

            f.vResult.x = f.vOffset.x + pBasePos->x;
            f.vResult.y = f.vOffset.y + pBasePos->y;
            f.vResult.z = f.vOffset.z + pBasePos->z;

            pSource->m_pBody->pos = f.vResult;
        }
    }
}

// The ellipsoid's out-of-bounds height answer of one is not checked, in the original too.

// FUNCTION: HYPERX 0x0045efc0
void __cdecl PlanAction::SetLocation(Interactor* pContext, PlanNode* pNode, short*, int nMarkerIndex)
{
    Vec3* pPos;

    if (pNode->m_pArgs[ARG_FIRST].n == PLAN_TOKEN_MESSAGE_DATA)
    {
        Vec3* pMarker;
        Vec3* pPrev;
        Vec3* pWrite;

        pPos    = pContext->GetActor()->GetPositionPtr();
        pMarker = g_apPositionMarkers[nMarkerIndex];
        *pPos   = *pMarker;

        pPrev  = &pContext->GetActor()->GetPhysicsBody()->prevPos;
        *pPrev = *pPos;

        pWrite  = &pContext->GetActor()->GetPhysicsBody()->pos;
        *pWrite = *pPrev;
    }
    else if (pNode->m_nArgCount >= ARG_COUNT_WITH_XY)
    {
        pContext->GetActor()->GetPhysicsBody()->pos.x = pContext->GetActor()->GetPhysicsBody()->prevPos.x = pContext->GetActor()->GetPositionPtr()->x = PlanArg::ResolveFloat(pContext, pNode, 1);

        pContext->GetActor()->GetPhysicsBody()->pos.y = pContext->GetActor()->GetPhysicsBody()->prevPos.y = pContext->GetActor()->GetPositionPtr()->y = PlanArg::ResolveFloat(pContext, pNode, 2);

        if (pNode->m_nArgCount == ARG_COUNT_WITH_Z)
        {
            pContext->GetActor()->GetPhysicsBody()->pos.z = pContext->GetActor()->GetPhysicsBody()->prevPos.z = pContext->GetActor()->GetPositionPtr()->z = PlanArg::ResolveFloat(pContext, pNode, 3);
        }
        else
        {
            pContext->GetActor()->GetPhysicsBody()->pos.z = pContext->GetActor()->GetPhysicsBody()->prevPos.z = pContext->GetActor()->GetPositionPtr()->z = g_arenaEllipsoid.GetZ(PlanArg::ResolveFloat(pContext, pNode, 1),
                                                                                                                                                                                  PlanArg::ResolveFloat(pContext, pNode, 2));
        }
    }
    else
    {
        Vec3* pMarker;
        Vec3* pPrev;
        Vec3* pWrite;

        pPos    = pContext->GetActor()->GetPositionPtr();
        pMarker = g_apPositionMarkers[pNode->m_pArgs[ARG_FIRST].n];
        *pPos   = *pMarker;

        pPrev  = &pContext->GetActor()->GetPhysicsBody()->prevPos;
        *pPrev = *pPos;

        pWrite  = &pContext->GetActor()->GetPhysicsBody()->pos;
        *pWrite = *pPrev;
    }

    PhysicsBody* pBody;
    Vec3*        pNormal;
    char         cSavedVisible;

    pBody = pContext->GetActor()->GetPhysicsBody();

    pNormal = g_arenaEllipsoid.NormalAtPoint(&pContext->GetActor()->GetPhysicsBody()->pos);

#ifdef HYPERX_BRINGUP
    // tools/check_match.py flags a null normal, which the copy below reads unchecked.
    BringUp::Trace("setPosNormal", (int)pNormal);
#endif

    pBody->surfNormal = *pNormal;

    pContext->GetActor()->GetPhysicsBody()->vel.z = g_flZeroPlan;
    pContext->GetActor()->GetPhysicsBody()->vel.y = g_flZeroPlan;
    pContext->GetActor()->GetPhysicsBody()->vel.x = g_flZeroPlan;

    if (pContext->GetActor()->GetSlot() < PLAYER_OBJECT_COUNT)
    {
        cSavedVisible = pContext->GetActor()->IsVisible();
        pContext->GetActor()->SetVisible(1);
        pContext->GetActor()->ApplyRenderSlotPose();
        pContext->GetActor()->SetVisible(cSavedVisible);
    }
}

// The copy through the owner's position accessor writes the position onto itself.

// A node of four arguments carries three literal integers.
#define ARG_COUNT_WITH_LITERALS 4

// Below this length the heading is left alone.
#define MIN_VELOCITY_LENGTH 0.001f

// FUNCTION: HYPERX 0x0045f2a0
void __cdecl PlanAction::SetVelocity(Interactor* pContext, PlanNode* pNode, short*, int)
{
    PlanWord* pnArgs;
    Vec3*     pOwnPosition;
    Vec3      vVelocity;

    if (pNode->m_nArgCount == ARG_COUNT_WITH_LITERALS)
    {
        pnArgs = pNode->m_pArgs;

        vVelocity.x = (float)pnArgs[1].n;
        vVelocity.y = (float)pnArgs[2].n;
        vVelocity.z = (float)pnArgs[3].n;
    }
    else
    {
        pnArgs = pNode->m_pArgs;

        vVelocity.x = g_apTrackOffsets[pnArgs[1].n]->x;
        vVelocity.y = g_apTrackOffsets[pnArgs[1].n]->y;
        vVelocity.z = g_apTrackOffsets[pnArgs[1].n]->z;
    }

    Vec3* const pPosition = &pContext->GetActor()->GetPhysicsBody()->pos;

    pContext->GetActor()->GetPhysicsBody()->TakeSurfaceNormal(pPosition);

    Vec3* const pVelocity = &pContext->GetActor()->GetPhysicsBody()->vel;

    pVelocity->x = vVelocity.x;
    pVelocity->y = vVelocity.y;
    pVelocity->z = vVelocity.z;

    Vec3* const pPrevious = &pContext->GetActor()->GetPhysicsBody()->prevPos;

    pPrevious->x = pPosition->x - vVelocity.x;
    pPrevious->y = pPosition->y - vVelocity.y;
    pPrevious->z = pPosition->z - vVelocity.z;

    pOwnPosition = pContext->GetActor()->GetPositionPtr();

    *pOwnPosition = *pPosition;

    pContext->GetActor()->GetPhysicsBody()->smoothVel.x = vVelocity.x;

    pContext->GetActor()->GetPhysicsBody()->smoothVel.y = vVelocity.y;

    pContext->GetActor()->GetPhysicsBody()->smoothVel.z = vVelocity.z;

    if ((float)sqrt(vVelocity.x * vVelocity.x + vVelocity.y * vVelocity.y + vVelocity.z * vVelocity.z) > MIN_VELOCITY_LENGTH)
    {
        PhysicsBody* pBody = pContext->GetActor()->GetPhysicsBody();

        Vec3::ScaleTo(&vVelocity, g_flOnePlan / (float)sqrt(vVelocity.x * vVelocity.x + vVelocity.y * vVelocity.y + vVelocity.z * vVelocity.z), &pBody->velDir);
    }
}

#define FACING_MIN_DOT 0.1f
#define TACKLE_RANGE   25.0
#define TACKLE_MIN_DOT 0.2f

#define TACKLE_MESSAGE_ARG 2

// Three; its marker is in src/globals.cpp.
extern const float g_flThreePlan;

namespace PlanAction
{

// Facing within range and heading the same way.
inline bool IsInReach(Interactor* pContext, EngineObject* pMate, float flFacing, float flRange, float flDotGate)
{
    float flDistance;

    if ((flDistance = pContext->GetActor()->GetDistIfFacing(pMate, flFacing)) == 0.0f || flDistance >= flRange)
    {
        return false;
    }

    PhysicsBody* const pMateBody  = pMate->GetPhysicsBody();
    PhysicsBody* const pOwnerBody = pContext->GetActor()->GetPhysicsBody();

    return Vec3::DotInline(&pOwnerBody->velDir, &pMateBody->velDir) > flDotGate;
}

// A loose rok is chased for a second, a carried one for two.
inline short TackleCountdown(bool bLoose)
{
    if (bLoose)
    {
        return 100;
    }

    return 200;
}

} // namespace PlanAction

// A tackle with the rok loose is shorter and has a stricter dot gate.
// FUNCTION: HYPERX 0x0045f450
void __cdecl PlanAction::Engage_Opponent(Interactor* pContext, PlanNode*, short*, int)
{
    AttachPak*            pnCarrier;
    short                 nCarrier;
    bool                  bCarrier;
    float                 flDotGate;
    Vec3                  vOffset;
    CSetPositionRelative* pSetPosition;

    const short nFirst = (short)((pContext->GetActor()->GetSlot() + 1) % PLAYER_SLOT_COUNT);

    EngineObject* const pFirst = g_pGameEngine->GetSlot(nFirst);

    const short nSecond = (short)((pContext->GetActor()->GetSlot() + 3) % PLAYER_SLOT_COUNT);

    EngineObject* pSecond = g_pGameEngine->GetSlot(nSecond);

    pnCarrier = g_pGameEngine->GetSlot(SLOT_ROK)->GetPhysicsBody()->pAttach;

    if (pnCarrier != 0 && pnCarrier->nSlot < PLAYER_OBJECT_COUNT)
    {
        nCarrier = pnCarrier->nSlot;
        bCarrier = true;
    }
    else
    {
        bCarrier = false;
    }

    const bool bLoose = !bCarrier;

    if (!bLoose)
    {
        if (nCarrier == pContext->GetActor()->GetSlot())
        {
            return;
        }
    }

    if (IsInReach(pContext, pFirst, FACING_MIN_DOT, TACKLE_RANGE, TACKLE_MIN_DOT))
    {
        pSecond = pFirst;
    }
    else if (!IsInReach(pContext, pSecond, FACING_MIN_DOT, TACKLE_RANGE, TACKLE_MIN_DOT))
    {
        return;
    }

    if (pSecond == 0)
    {
        return;
    }

    pSetPosition = (CSetPositionRelative*)AllocAgendaItem(Agenda_CSetPositionRelative);

    vOffset.x = -7.0f;
    vOffset.y = g_flThreePlan;
    vOffset.z = g_flZeroPlan;

    GameBehaviorObject* pBindNode = pContext->GetActor()->GetPlanState()->m_pCurrent;

    pSetPosition->Bind(pContext->GetActor(), pBindNode);

    pSetPosition->SetSourceObjectByIndex(pSecond->GetSlot());

    pSetPosition->m_wMode = 1;

    pSetPosition->SetPositionWithTerrainHeight(&g_vZeroMarker);

    pSetPosition->SetOffset(vOffset);

    pSetPosition->Activate();

    CBungeeToward* const pBungee = (CBungeeToward*)AllocAgendaItem(Agenda_CBungeeToward);

    pBindNode = pContext->GetActor()->GetPlanState()->m_pCurrent;

    pBungee->Bind(pContext->GetActor(), pBindNode);

    pBungee->m_pAnchor = &g_vZeroMarker;
    pBungee->m_pTarget = pSecond;
    pBungee->m_flGain  = FACING_MIN_DOT;

    pBungee->SetNearRange(g_flZeroPlan);

    pBungee->m_flRestLength = 0.0f;
    pBungee->m_flFarRange   = 35.0f;

    if (bLoose)
    {
        flDotGate = g_flZeroPlan;
    }
    else
    {
        flDotGate = -0.5f;
    }

    pBungee->m_flDotGate = flDotGate;

    pBungee->m_cAttached = 1;

    pBungee->m_nCountdown = TackleCountdown(bLoose);

    pBungee->Activate();

    pSecond->PostEvent(MSG_YER_ATTACKED, TACKLE_MESSAGE_ARG, pContext->GetActor()->GetSlot());
}

// Healthy enough, facing within range, and heading the same way.
namespace PlanCond
{

inline bool CanEngage(Interactor* pContext, PlayerObject* pOpponent, float flDamage)
{
    if (flDamage <= DamageGate())
    {
        return false;
    }

    float flDistance;

    if ((flDistance = pContext->GetActor()->GetDistIfFacingDefault(pOpponent)) == 0.0f || flDistance >= 45.0)
    {
        return false;
    }

    PhysicsBody* const pOpponentBody = pOpponent->GetPhysicsBody();
    PhysicsBody* const pOwnerBody    = pContext->GetActor()->GetPhysicsBody();

    return Vec3::DotInline(&pOwnerBody->velDir, &pOpponentBody->velDir) > 0.2f;
}

} // namespace PlanCond

// Answers zero for the plus-one opponent, one for plus-three and two for neither.
// FUNCTION: HYPERX 0x0045f740
void __cdecl PlanCond::EngageWhichOpponent(Interactor* pContext, PlanNode*, short* pResult, int)
{
    PlayerObject* pTarget;
    Vec3          vOffset;

    const short sFirst = (short)((pContext->GetActor()->GetSlot() + 1) % PLAYER_SLOT_COUNT);

    PlayerObject* const pFirst = g_pGameEngine->GetPlayer(sFirst);

    const short sSecond = (short)((pContext->GetActor()->GetSlot() + 3) % PLAYER_SLOT_COUNT);

    PlayerObject* const pSecond = g_pGameEngine->GetPlayer(sSecond);

    const float flDamageA = pFirst->m_stats.GetScalar(RATING_DAMAGE);
    const float flDamageB = pSecond->m_stats.GetScalar(RATING_DAMAGE);

    if (CanEngage(pContext, pFirst, flDamageA))
    {
        *pResult = 0;
        pTarget  = pFirst;
    }
    else if (CanEngage(pContext, pSecond, flDamageB))
    {
        *pResult = 1;
        pTarget  = pSecond;
    }
    else
    {
        pTarget = 0;
    }

    if (pTarget != 0)
    {
        CSetPositionRelative* const pSetPosition = (CSetPositionRelative*)AllocAgendaItem(Agenda_CSetPositionRelative);

        GameBehaviorObject* const pCurrent = pContext->GetActor()->GetPlanState()->m_pCurrent;

        pSetPosition->Bind(pContext->GetActor(), pCurrent);

        pSetPosition->SetSourceObjectByIndex(pTarget->GetSlot());

        pSetPosition->m_wMode = 1;

        pSetPosition->SetPositionWithTerrainHeight(&g_vZeroMarker);

        const Vec3* pPosOwner = pContext->GetActor()->GetPositionPtr();
        const Vec3* pPosMate  = pTarget->GetPositionPtr();
        vOffset.x             = pPosOwner->x - pPosMate->x;

        pPosOwner = pContext->GetActor()->GetPositionPtr();
        pPosMate  = pTarget->GetPositionPtr();
        vOffset.y = pPosOwner->y - pPosMate->y;

        pPosOwner = pContext->GetActor()->GetPositionPtr();
        pPosMate  = pTarget->GetPositionPtr();
        vOffset.z = pPosOwner->z - pPosMate->z;

        pSetPosition->SetOffset(vOffset);

        pSetPosition->Activate();
    }
    else
    {
        *pResult = 2;
    }
}

// A holder whose slot agrees with the actor's in its low bit is the same side, which answers zero.

// FUNCTION: HYPERX 0x0045fa70
void PlanCond::TeammateHasRok(Interactor* pContext, PlanNode*, short* pResult, int)
{
    AttachPak* const pState = g_pGameEngine->m_ppSlots[SLOT_ROK]->GetPhysicsBody()->pAttach;

    if (pState != 0)
    {
        const short nHolderSlot = pState->nSlot;

        if (nHolderSlot < PLAYER_OBJECT_COUNT)
        {
            EngineObject* const pActor = pContext->GetActor();

            if (((pActor->GetSlot() ^ nHolderSlot) & 1) == 0)
            {
                *pResult = 0;

                return;
            }
        }
    }

    *pResult = 1;
}

#define ANNOUNCER_GAG_MS 0x3a98

// Inside the gag window it answers one past the highest draw.
// FUNCTION: HYPERX 0x0045fac0
void __cdecl PlanCond::GagTheAnnouncer(Interactor*, PlanNode* pArgs, short* pnOut, int)
{
    const int nNow = g_pGameEngine->m_clock.nTickMs;

    if ((unsigned int)(nNow - g_nAnnouncerLineMs) < ANNOUNCER_GAG_MS)
    {
        *pnOut = (short)(pArgs->m_nArgCount - 2);

        return;
    }

    *pnOut = (short)(rand() * (pArgs->m_nArgCount - 2) / (RAND_MAX + 1));

    g_nAnnouncerLineMs = nNow;
}

// FUNCTION: HYPERX 0x0045fb30
void PlanCond::VictimCamIsOn(Interactor*, PlanNode*, short* pResult, int)
{
    *pResult = g_bVictimCam == 0;
}

// FUNCTION: HYPERX 0x0045fb50
void PlanCond::IsScanningModeOn(Interactor*, PlanNode*, short* pResult, int)
{
    *pResult = !g_bScanningModesEnabled;
}

// Zero only when the second object heads away from the first, both move the same way, and they are within a hundred and fifty.

// FUNCTION: HYPERX 0x0045fb70
void __cdecl PlanCond::AttackerIsInRear(Interactor* pContext, PlanNode*, short* pResult, int)
{
    EngineObject* pFirst;
    EngineObject* pSecond;
    Vec3          vGap;

    pFirst = g_pGameEngine->m_ppSlots[(short)CollisionObj::GetModelId(
        (int)pContext->GetModelSlot())];

    Vec3 vFirst = *pFirst->GetPositionPtr();

    pSecond = g_pGameEngine->m_ppSlots[(short)CollisionObj::GetModelId(
        (int)pContext->GetOtherSlot())];

    Vec3 vSecond = *pSecond->GetPositionPtr();

    vGap.x = vFirst.x - vSecond.x;
    vGap.y = vFirst.y - vSecond.y;
    vGap.z = vFirst.z - vSecond.z;

    Vec3 vOffset = vGap;

    Math::Vec3Normalize(&vOffset);

    const Vec3* pDirB = &pSecond->GetPhysicsBody()->velDir;

    if (vOffset.x * pDirB->x + vOffset.y * pDirB->y + vOffset.z * pDirB->z < -0.1f)
    {
        PhysicsBody* pBlockA = pFirst->GetPhysicsBody();
        PhysicsBody* pBlockB = pSecond->GetPhysicsBody();

        if (Vec3::DotInline(&pBlockB->velDir, &pBlockA->velDir) > 0.25 && (float)sqrt(vGap.x * vGap.x + vGap.y * vGap.y + vGap.z * vGap.z) < 150.0f)
        {
            *pResult = 0;

            return;
        }
    }

    *pResult = 1;
}

// The selector that arms the throttle instead of answering a state.
#define SELECTOR_ARM 0x18

#define ARM_WINDOW_MS   300
#define STATE_WINDOW_MS 800

// The tick the arming selector last ran at.
// GLOBAL: HYPERX 0x004cf9f8
extern unsigned int g_nRokCameraArmedTick;

// The tick this condition last answered at.
// GLOBAL: HYPERX 0x004cf9f4
extern unsigned int g_nRokCameraStateTick;

// FUNCTION: HYPERX 0x0045fcf0
void PlanCond::AnalyzePadA(Interactor*, PlanNode*, short* pResult, int nSelector)
{
    AttachPak* pTarget;
    CPlayer*   pOwner;
    short      nSlot;
    short      nKind;
    int        nOther;
    float      flLimit;

    if (nSelector == SELECTOR_ARM)
    {
        g_nRokCameraArmedTick = g_pGameEngine->m_clock.nTickMs;

        pTarget = g_pGameEngine->m_ppSlots[SLOT_ROK]->GetPhysicsBody()->pAttach;

        if (pTarget != 0 && (nSlot = pTarget->nSlot) < PLAYER_OBJECT_COUNT)
        {
            if (nSlot == g_pGameEngine->m_nViewedSlot)
            {
                *pResult = 0;
            }
            else
            {
                nKind = g_pGameEngine->m_ppSlots[g_pGameEngine->m_nViewedSlot]->GetSlot();

                if (nSlot == TeammateSlot(nKind))
                {
                    *pResult = 1;
                }
                else
                {
                    *pResult = 2;
                }
            }
        }
        else
        {
            *pResult = 2;
        }
    }
    else
    {
        pTarget = g_pGameEngine->m_ppSlots[SLOT_ROK]->GetPhysicsBody()->pAttach;

        if (pTarget != 0 && (nSlot = pTarget->nSlot) < PLAYER_OBJECT_COUNT)
        {
            if (nSlot == g_pGameEngine->m_nViewedSlot)
            {
                if (g_pGameEngine->GetPlayer(TeammateSlot(nSlot))->m_wDesiredSubstitute != -1)
                {
                    *pResult = 4;
                }
                else
                {
                    flLimit = DamageGate();

                    PlayerObject* const pCandidate = g_pGameEngine->GetPlayer(TeammateSlot(nSlot));

                    if (pCandidate->m_stats.GetScalar(RATING_DAMAGE) <= flLimit)
                    {
                        *pResult = 4;
                    }
                    else
                    {
                        const float flFacing = (float)cos(PlanUtil::ComputeConeHalfAngle(
                            nSlot, (short)TeammateSlot(nSlot)));

                        if (g_pGameEngine->GetSlot(g_pGameEngine->m_nViewedSlot)->GetDistIfFacing(g_pGameEngine->GetSlot(TeammateSlot(nSlot)), flFacing) != 0.0f)
                        {
                            *pResult = 3;
                        }
                        else
                        {
                            *pResult = 4;
                        }
                    }
                }
            }
            else
            {
                nKind = g_pGameEngine->m_ppSlots[g_pGameEngine->m_nViewedSlot]->GetSlot();

                if (nKind < 4)
                {
                    nOther = nKind + 2;
                }
                else
                {
                    nOther = nKind;
                }

                if (nSlot == (short)(nOther & 3))
                {
                    if (g_pGameEngine->m_clock.nTickMs - g_nRokCameraArmedTick > ARM_WINDOW_MS)
                    {
                        const float flFacing = (float)cos(PlanUtil::ComputeConeHalfAngle(
                            nSlot, g_pGameEngine->m_nViewedSlot));

                        if (g_pGameEngine->GetSlot(nSlot)->GetDistIfFacing(
                                g_pGameEngine->GetSlot(g_pGameEngine->m_nViewedSlot), flFacing) != 0.0f)
                        {
                            *pResult = 5;
                        }
                        else
                        {
                            *pResult = 4;
                        }
                    }
                    else if (g_pGameEngine->m_clock.nTickMs - g_nRokCameraStateTick > STATE_WINDOW_MS)
                    {
                        *pResult = (short)((g_pGameEngine->m_bFollowingOther != 0) + 6);
                    }
                    else
                    {
                        *pResult = 7;
                    }
                }
                else if (g_pGameEngine->m_clock.nTickMs - g_nRokCameraArmedTick < ARM_WINDOW_MS && g_pGameEngine->m_clock.nTickMs - g_nRokCameraStateTick > STATE_WINDOW_MS)
                {
                    if (g_pGameEngine->m_bFollowingOther != 0)
                    {
                        *pResult = 7;
                    }
                    else
                    {
                        *pResult = 6;
                    }
                }
                else
                {
                    *pResult = 7;
                }
            }
        }
        else if (g_pGameEngine->m_clock.nTickMs - g_nRokCameraArmedTick < ARM_WINDOW_MS)
        {
            if (g_pGameEngine->m_bFollowingOther != 0)
            {
                *pResult = 7;
            }
            else
            {
                *pResult = 6;
            }
        }
        else
        {
            *pResult = 7;
        }

        g_nRokCameraStateTick = g_pGameEngine->m_clock.nTickMs;
    }

    pOwner = g_pGameEngine->m_ppSlots[g_pGameEngine->m_nViewedSlot]->GetPhysicsBody()->pPlayer;

    if (pOwner == 0)
    {
        return;
    }

    nKind = g_pGameEngine->m_nViewedSlot;

    CPlayer* const pOtherOwner = g_pGameEngine->m_ppSlots[TeammateSlot(nKind)]->GetPhysicsBody()->pPlayer;

    if (pOtherOwner == 0)
    {
        return;
    }

    if (pOwner->pState->nHuman == 0)
    {
        return;
    }

    if (pOtherOwner->pState->nHuman == 0)
    {
        return;
    }

    nKind = *pResult;

    if (nKind == 0)
    {
        *pResult = 2;

        return;
    }

    if (nKind == 1)
    {
        *pResult = 2;

        return;
    }

    if (nKind == 5 || nKind == 6)
    {
        *pResult = 7;
    }
}

#define RELATIONSHIP_KIND_DIRECTION 0x16

// Kind 0x16 normalises a direction nothing reads, in the original too.
// FUNCTION: HYPERX 0x00460140
void __cdecl PlanCond::AnalyzePadB(Interactor*, PlanNode*, short* pResult, int nKind)
{
    EngineObject* pTarget;
    AttachPak*    pSlot;
    short         nSlot;
    Vec3          vDirection;

    if (nKind == RELATIONSHIP_KIND_DIRECTION)
    {
        EngineObject* const pRok = g_pGameEngine->m_ppSlots[SLOT_ROK];
        pSlot                    = pRok->GetPhysicsBody()->pAttach;

        if (pSlot != 0 && (nSlot = pSlot->nSlot) < PLAYER_OBJECT_COUNT)
        {
            PlayerObject* const pFollowed = g_pGameEngine->GetPlayer(g_pGameEngine->m_nViewedSlot);

            if (nSlot == pFollowed->GetSlot())
            {
                pTarget = g_pGameEngine->m_ppSlots[pFollowed->m_wTargetGoalSlot];

                Vec3* const pFrom = pRok->GetPositionPtr();
                Vec3* const pTo   = pTarget->GetPositionPtr();

                vDirection.x = pTo->x - pFrom->x;
                vDirection.y = pTo->y - pFrom->y;
                vDirection.z = pTo->z - pFrom->z;

                Math::Vec3Normalize(&vDirection);

                *pResult = 0;

                return;
            }
        }

        *pResult = 1;

        return;
    }

    pSlot = g_pGameEngine->m_ppSlots[SLOT_ROK]->GetPhysicsBody()->pAttach;

    if (pSlot != 0 && (nSlot = pSlot->nSlot) < PLAYER_OBJECT_COUNT && nSlot == g_pGameEngine->m_nViewedSlot)
    {
        *pResult = 2;

        return;
    }

    *pResult = 3;
}

// Five hundred, as a squared distance, a little over twenty two: both opponents have to be beyond it.
#define FAR_RANGE_SQUARED 500.0f

// FUNCTION: HYPERX 0x00460240
void __cdecl PlanCond::InRangeToAttack(Interactor* pContext, PlanNode*, short* pResult, int)
{
    Vec3 vNear;
    Vec3 vFar;

    unsigned char bySlot = (unsigned char)pContext->GetActor()->GetSlot();

    const Vec3* pNearAt = g_pGameEngine->m_ppSlots[OPPOSING_FIRST(bySlot)]->GetPositionPtr();

    bySlot = (unsigned char)pContext->GetActor()->GetSlot();

    const Vec3* pFarAt = g_pGameEngine->m_ppSlots[OPPOSING_SECOND(bySlot)]->GetPositionPtr();

    const Vec3* pSelfAt = pContext->GetActor()->GetPositionPtr();

    vNear.x = pNearAt->x - pSelfAt->x;
    vNear.y = pNearAt->y - pSelfAt->y;
    vNear.z = pNearAt->z - pSelfAt->z;

    vFar.x = pFarAt->x - pSelfAt->x;
    vFar.y = pFarAt->y - pSelfAt->y;
    vFar.z = pFarAt->z - pSelfAt->z;

    if (vNear.z * vNear.z + vNear.y * vNear.y + vNear.x * vNear.x >= FAR_RANGE_SQUARED && vFar.z * vFar.z + vFar.y * vFar.y + vFar.x * vFar.x >= FAR_RANGE_SQUARED)
    {
        *pResult = 0;

        return;
    }

    *pResult = 1;
}

// Zero only when the nearer opponent is within a hundred and the detour is at most 60 per cent.

// How much longer the path through the actor may be than the direct one.
#define PATH_SLACK 1.6f

// FUNCTION: HYPERX 0x00460340
void __cdecl PlanCond::Decide_BlockOrGoOut(Interactor* pContext, PlanNode*, short* pResult, int)
{
    const Vec3* pFirst;
    const Vec3* pSecond;
    const Vec3* pObject;
    Vec3        vFirst;
    Vec3        vSecond;
    Vec3        vFirstSelf;
    Vec3        vSecondSelf;
    Vec3        vObjectSelf;
    float       flToSecond;
    float       flToObject;

    EngineObject* pActor = pContext->GetActor();
    unsigned char bySlot = (unsigned char)pActor->GetSlot();
    pFirst               = g_pGameEngine->m_ppSlots[OPPOSING_FIRST(bySlot)]->GetPositionPtr();

    pActor  = pContext->GetActor();
    bySlot  = (unsigned char)pActor->GetSlot();
    pSecond = g_pGameEngine->m_ppSlots[OPPOSING_SECOND(bySlot)]->GetPositionPtr();

    pActor = pContext->GetActor();

    const short nSlot = pActor->GetSlot();

    pObject = g_pGameEngine->m_ppSlots[TeammateSlot(nSlot)]->GetPositionPtr();

    const Vec3* pSelf = pContext->GetActor()->GetPositionPtr();

    vFirst.x = pFirst->x - pObject->x;
    vFirst.y = pFirst->y - pObject->y;
    vFirst.z = pFirst->z - pObject->z;

    vSecond.x = pSecond->x - pObject->x;
    vSecond.y = pSecond->y - pObject->y;
    vSecond.z = pSecond->z - pObject->z;

    vFirstSelf.x = pFirst->x - pSelf->x;
    vFirstSelf.y = pFirst->y - pSelf->y;
    vFirstSelf.z = pFirst->z - pSelf->z;

    vSecondSelf.x = pSecond->x - pSelf->x;
    vSecondSelf.y = pSecond->y - pSelf->y;
    vSecondSelf.z = pSecond->z - pSelf->z;

    vObjectSelf.x = pObject->x - pSelf->x;
    vObjectSelf.y = pObject->y - pSelf->y;
    vObjectSelf.z = pObject->z - pSelf->z;

    const float flHypotFirst = (float)sqrt(vFirst.x * vFirst.x + vFirst.y * vFirst.y + vFirst.z * vFirst.z);
    flToSecond               = (float)sqrt(vSecond.x * vSecond.x + vSecond.y * vSecond.y + vSecond.z * vSecond.z);

    const bool bFirstCloser = flHypotFirst < flToSecond;

    flToObject = (float)sqrt(vObjectSelf.x * vObjectSelf.x + vObjectSelf.y * vObjectSelf.y + vObjectSelf.z * vObjectSelf.z);

    if (bFirstCloser)
    {
        if (flHypotFirst <= 100.0 && (float)sqrt(vFirstSelf.x * vFirstSelf.x + vFirstSelf.y * vFirstSelf.y + vFirstSelf.z * vFirstSelf.z) + flToObject <= flHypotFirst * PATH_SLACK)
        {
            *pResult = 0;

            return;
        }

        *pResult = 1;

        return;
    }

    if (flToSecond <= 100.0 && (float)sqrt(vSecondSelf.x * vSecondSelf.x + vSecondSelf.y * vSecondSelf.y + vSecondSelf.z * vSecondSelf.z) + flToObject <= flToSecond * PATH_SLACK)
    {
        *pResult = 0;

        return;
    }

    *pResult = 1;
}

// FUNCTION: HYPERX 0x004605a0
void PlanCond::SpeedBurstIsRelaxed(Interactor* pContext, PlanNode*, short* pResult, int)
{
    EngineObject* const pActor = pContext->GetActor();

    if (pActor->GetPhysicsBody()->flSteerScale > 1.0001)
    {
        *pResult = 1;
    }
    else
    {
        *pResult = 0;
    }
}

// Higher damage is healthier in this bank, so at or below the limit means hurt.

// FUNCTION: HYPERX 0x004605e0
void __cdecl PlanCond::WhichOppnIsClosestToRok(Interactor* pContext, PlanNode*, short* pResult, int)
{
    const Vec3* pRokAt;
    Vec3        vFar;
    float       flDamageNear;

    pRokAt = g_pGameEngine->m_ppSlots[SLOT_ROK]->GetPositionPtr();

    unsigned char bySlot = (unsigned char)pContext->GetActor()->GetSlot();

    PlayerObject* pOpponent = g_pGameEngine->GetPlayer(OPPOSING_FIRST(bySlot));

    const Vec3* pNearAt = pOpponent->GetPositionPtr();

    bySlot = (unsigned char)pContext->GetActor()->GetSlot();

    pOpponent = g_pGameEngine->GetPlayer(OPPOSING_SECOND(bySlot));

    const Vec3* pFarAt = pOpponent->GetPositionPtr();

    const float flNearX = pRokAt->x - pNearAt->x;
    const float flNearY = pRokAt->y - pNearAt->y;
    const float flNearZ = pRokAt->z - pNearAt->z;

    vFar.x = pRokAt->x - pFarAt->x;
    vFar.y = pRokAt->y - pFarAt->y;
    vFar.z = pRokAt->z - pFarAt->z;

    if (flNearX * flNearX + flNearY * flNearY + flNearZ * flNearZ < vFar.x * vFar.x + vFar.y * vFar.y + vFar.z * vFar.z)
    {
        *pResult = 0;
    }
    else
    {
        *pResult = 1;
    }

    pOpponent = g_pGameEngine->GetPlayer(OPPOSING_FIRST(pContext->GetActor()->GetSlot()));

    flDamageNear = pOpponent->m_stats.GetScalar(RATING_DAMAGE);

    pOpponent = g_pGameEngine->GetPlayer(OPPOSING_SECOND(pContext->GetActor()->GetSlot()));

    const float flDamageFar = pOpponent->m_stats.GetScalar(RATING_DAMAGE);

    if (flDamageNear <= DamageGate())
    {
        if (flDamageFar <= DamageGate())
        {
            *pResult = 2;

            return;
        }
    }

    if (*pResult == 0)
    {
        if (flDamageNear <= DamageGate())
        {
            *pResult = 1;

            return;
        }
    }

    if (*pResult == 1)
    {
        if (flDamageFar <= DamageGate())
        {
            *pResult = 0;
        }
    }
}

// Both team mates hurt; the goalie's plan passes to the opponents' goal instead.
#define NO_PASS_TARGET 4

// FUNCTION: HYPERX 0x004608a0
void __cdecl PlanCond::BallGotBehindMe(Interactor* pContext, PlanNode*, short* pResult, int)
{
    const Vec3* pGoalCentre;
    Vec3        vSpawn;
    Vec3        vRok;
    Vec3        vActor;

    EngineObject* const pActor = pContext->GetActor();

    pActor->GetSlot();

    EngineObject* const pRok = g_pGameEngine->m_ppSlots[SLOT_ROK];

    const short nTeam = pActor->GetSlot();

    if (nTeam % 2 == 0)
    {
        pGoalCentre = g_apPositionMarkers[POS_N_GOAL_CENTER];
    }
    else
    {
        pGoalCentre = g_apPositionMarkers[POS_S_GOAL_CENTER];
    }

    vSpawn.x = pGoalCentre->x;
    vSpawn.y = pGoalCentre->y;
    vSpawn.z = pGoalCentre->z;

    vActor = *pActor->GetPositionPtr();

    vRok = *pRok->GetPositionPtr();

    const float flDx = vSpawn.x - vRok.x;
    const float flDy = vSpawn.y - vRok.y;
    const float flDz = vSpawn.z - vRok.z;

    Vec3 vToSpawn;

    vToSpawn.x = vSpawn.x - vActor.x;
    vToSpawn.y = vSpawn.y - vActor.y;
    vToSpawn.z = vSpawn.z - vActor.z;

    if ((float)sqrt(flDx * flDx + flDy * flDy + flDz * flDz) > (float)sqrt(vToSpawn.x * vToSpawn.x + vToSpawn.y * vToSpawn.y + vToSpawn.z * vToSpawn.z))
    {
        *pResult = 0;

        return;
    }

    *pResult = 1;
}

// A team mate in the Sideln plan counts as hurt.
// FUNCTION: HYPERX 0x004609d0
void __cdecl PlanCond::GoaliePassingTarget(Interactor* pContext, PlanNode*, short* pResult, int)
{
    EngineObject* const pOwner = pContext->GetActor();

    const int nSlot = pOwner->GetSlot() % 2;

    EngineObject** const apObjects = g_pGameEngine->m_ppSlots;

    EngineObject* const pNear = apObjects[(short)nSlot];

    const int nOther = nSlot + 2;

    EngineObject* const pFar = apObjects[(short)nOther];

    Vec3 vOwn = *pOwner->GetPositionPtr();

    Vec3 vNear = *pNear->GetPositionPtr();

    Vec3 vFar = *pFar->GetPositionPtr();

    Vec3 vToNear;

    vToNear.x = vNear.x - vOwn.x;
    vToNear.y = vNear.y - vOwn.y;
    vToNear.z = vNear.z - vOwn.z;

    Vec3 vToFar;

    vToFar.x = vFar.x - vOwn.x;
    vToFar.y = vFar.y - vOwn.y;
    vToFar.z = vFar.z - vOwn.z;

    const float flDistNear = (float)sqrt((vToNear.z * vToNear.z + vToNear.y * vToNear.y) + vToNear.x * vToNear.x);

    const float flDistFar = (float)sqrt((vToFar.z * vToFar.z + vToFar.y * vToFar.y) + vToFar.x * vToFar.x);

    float flDamageNear = ((PlayerObject*)pNear)->m_stats.GetScalar(RATING_DAMAGE);

    float flDamageFar = ((PlayerObject*)pFar)->m_stats.GetScalar(RATING_DAMAGE);

    const unsigned char* pszNear = (const unsigned char*)pNear->GetPlanState()->m_pszName;

    // STRING: HYPERX 0x004bb37c
    if (_mbscmp(pszNear, (const unsigned char*)"Sideln") == 0)
    {
        flDamageNear = 0.0f;
    }

    const unsigned char* pszFar = (const unsigned char*)pFar->GetPlanState()->m_pszName;

    // STRING: HYPERX 0x004bb37c
    if (_mbscmp(pszFar, (const unsigned char*)"Sideln") == 0)
    {
        flDamageFar = 0.0f;
    }

    if (flDamageNear <= DamageGate())
    {
        if (flDamageFar <= DamageGate())
        {
            *pResult = NO_PASS_TARGET;

            return;
        }
    }
    else
    {
        if (flDamageFar <= DamageGate() || flDistNear >= flDistFar)
        {
            *pResult = (short)nSlot;

            return;
        }
    }

    *pResult = (short)nOther;
}

// Answers zero or one for the opponent to attack and two for neither, DECIDE's child index.

// FUNCTION: HYPERX 0x00460c80
void __cdecl PlanCond::WhichOppnToAttack(Interactor* pContext, PlanNode*, short* pResult, int)
{
    EngineObject* const pOwner = pContext->GetActor();

    PlayerObject* pOpponent = g_pGameEngine->GetPlayer(OPPOSING_FIRST(pContext->GetActor()->GetSlot()));

    const float flDamageA = pOpponent->m_stats.GetScalar(RATING_DAMAGE);

    pOpponent = g_pGameEngine->GetPlayer(OPPOSING_SECOND(pContext->GetActor()->GetSlot()));

    const float flDamageB = pOpponent->m_stats.GetScalar(RATING_DAMAGE);

    pOpponent = g_pGameEngine->GetPlayer(OPPOSING_FIRST(pContext->GetActor()->GetSlot()));

    float flDistanceA = pContext->GetActor()->GetDistIfFacingDefault(pOpponent);

    pOpponent = g_pGameEngine->GetPlayer(OPPOSING_SECOND(pContext->GetActor()->GetSlot()));

    float flDistanceB = pContext->GetActor()->GetDistIfFacingDefault(pOpponent);

    if (flDamageA <= DamageGate())
    {
        flDistanceA = g_flZeroPlan;
    }
    else
    {
        if (flDamageB <= DamageGate())
        {
            flDistanceB = g_flZeroPlan;
        }
    }

    if (flDistanceA == 0.0f)
    {
        if (flDistanceB == 0.0f)
        {
            *pResult = 2;
        }
        else
        {
            *pResult = 1;
        }
    }
    else if (flDistanceB != 0.0f && flDistanceA < flDistanceB)
    {
        *pResult = 0;
    }
    else if (flDistanceB == 0.0f)
    {
        *pResult = 0;
    }
    else
    {
        *pResult = 1;
    }

    if (flDamageA <= DamageGate())
    {
        if (flDamageB <= DamageGate())
        {
            *pResult = 2;
        }
    }
}

// Like WhichOppnToAttack, but falls back on plain distance when facing neither opponent.

// FUNCTION: HYPERX 0x00460f20
void __cdecl PlanCond::WhichOppnIsClosestToMe(Interactor* pContext, PlanNode*, short* pResult, int)
{
    Vec3 vOffsetA;
    Vec3 vOffsetB;

    EngineObject* const pOwner = pContext->GetActor();

    PlayerObject* pOpponent = g_pGameEngine->GetPlayer(OPPOSING_FIRST(pContext->GetActor()->GetSlot()));

    const float flDamageA = pOpponent->m_stats.GetScalar(RATING_DAMAGE);

    pOpponent = g_pGameEngine->GetPlayer(OPPOSING_SECOND(pContext->GetActor()->GetSlot()));

    const float flDamageB = pOpponent->m_stats.GetScalar(RATING_DAMAGE);

    pOpponent = g_pGameEngine->GetPlayer(OPPOSING_FIRST(pContext->GetActor()->GetSlot()));

    float flDistanceA = pContext->GetActor()->GetDistIfFacingDefault(pOpponent);

    pOpponent = g_pGameEngine->GetPlayer(OPPOSING_SECOND(pContext->GetActor()->GetSlot()));

    float flDistanceB = pContext->GetActor()->GetDistIfFacingDefault(pOpponent);

    if (flDamageA <= DamageGate())
    {
        flDistanceA = g_flZeroPlan;
    }

    if (flDamageB <= DamageGate())
    {
        flDistanceB = g_flZeroPlan;
    }

    if (flDistanceA == g_flZeroPlan && flDistanceB == g_flZeroPlan)
    {
        unsigned char bySlot = (unsigned char)pContext->GetActor()->GetSlot();

        pOpponent = g_pGameEngine->GetPlayer(OPPOSING_FIRST(bySlot));

        const Vec3* pPosA = pOpponent->GetPositionPtr();

        bySlot = (unsigned char)pContext->GetActor()->GetSlot();

        pOpponent = g_pGameEngine->GetPlayer(OPPOSING_SECOND(bySlot));

        const Vec3* pPosB = pOpponent->GetPositionPtr();

        const Vec3* pPosOwner = pOwner->GetPositionPtr();

        vOffsetA.x = pPosA->x - pPosOwner->x;
        vOffsetA.y = pPosA->y - pPosOwner->y;
        vOffsetA.z = pPosA->z - pPosOwner->z;

        pPosOwner = pOwner->GetPositionPtr();

        vOffsetB.x = pPosB->x - pPosOwner->x;
        vOffsetB.y = pPosB->y - pPosOwner->y;
        vOffsetB.z = pPosB->z - pPosOwner->z;

        if (flDamageA > DamageGate())
        {
            flDistanceA = (float)sqrt(vOffsetA.z * vOffsetA.z + vOffsetA.y * vOffsetA.y + vOffsetA.x * vOffsetA.x);
        }

        if (flDamageB > DamageGate())
        {
            flDistanceB = (float)sqrt(vOffsetB.z * vOffsetB.z + vOffsetB.y * vOffsetB.y + vOffsetB.x * vOffsetB.x);
        }
    }

    if (flDistanceA == 0.0f)
    {
        if (flDistanceB == 0.0f)
        {
            *pResult = 2;
        }
        else
        {
            *pResult = 1;
        }
    }
    else if (flDistanceB != 0.0f && flDistanceA < flDistanceB)
    {
        *pResult = 0;
    }
    else if (flDistanceB == 0.0f)
    {
        *pResult = 0;
    }
    else
    {
        *pResult = 1;
    }

    if (flDamageA <= DamageGate())
    {
        if (flDamageB <= DamageGate())
        {
            *pResult = 2;
        }
    }
}

// Without a valid pAttach slot this falls back on the low half of the result pointer.

// A tenth of a thousandth, below which the forward axis is left alone.
#define MIN_SPEED_FOR_FACING 0.0001

// FUNCTION: HYPERX 0x00461370
void __cdecl PlanCond::ClosestTargetToProjectile(Interactor* pContext, PlanNode*, short* pResult, int)
{
    AttachPak* pnTargetSlot;
    float      flSpeed;
    float      flLimit;
    float      flNear;
    float      flFar;
    short      nSlot;

    Vec3* const pVelocity = &pContext->GetActor()->GetPhysicsBody()->vel;

    flSpeed = pContext->GetActor()->GetPhysicsBody()->flSpeed;

    pnTargetSlot = pContext->GetActor()->GetPhysicsBody()->pAttach;

    if (pnTargetSlot == 0 || (nSlot = pnTargetSlot->nSlot) >= PLAYER_OBJECT_COUNT)
    {
        nSlot = (short)pResult;
    }

    EngineObject* const pActor = g_pGameEngine->GetSlot(nSlot);

    const short nNear = (short)(~nSlot & 1);
    const short nFar  = (short)((~nSlot & 1) | 2);

    if (flSpeed > MIN_SPEED_FOR_FACING)
    {
        PhysicsBody* pBlock = pContext->GetActor()->GetPhysicsBody();

        Vec3::ScaleTo(pVelocity, g_flOnePlan / flSpeed, &pBlock->velDir);
    }

    EngineObject* pFacing = pActor;

    flNear = pFacing->GetDistIfFacing(g_pGameEngine->GetSlot(nNear), FACING_MIN_DOT);

    pFacing = pActor;

    flFar = pFacing->GetDistIfFacing(g_pGameEngine->GetSlot(nFar), FACING_MIN_DOT);

    flLimit = DamageGate();

    PlayerObject* pCandidate = g_pGameEngine->GetPlayer(nNear);

    if (pCandidate->m_stats.GetScalar(RATING_DAMAGE) <= flLimit)
    {
        flNear = g_flZeroPlan;
    }

    flLimit = DamageGate();

    pCandidate = g_pGameEngine->GetPlayer(nFar);

    if (pCandidate->m_stats.GetScalar(RATING_DAMAGE) <= flLimit)
    {
        flFar = g_flZeroPlan;
    }

    if (flNear == 0.0f && flFar == 0.0f)
    {
        *pResult = GOALIE_SLOT_A;

        return;
    }

    if (flNear != 0.0f)
    {
        if (flFar != 0.0f)
        {
            if (flNear < flFar)
            {
                *pResult = nNear;

                return;
            }

            *pResult = nFar;

            return;
        }

        *pResult = nNear;

        return;
    }

    *pResult = nFar;
}

// A null behaviour target still reaches the vtable: the original's null test jumps into the block.

// FUNCTION: HYPERX 0x004615e0
void __cdecl PlanCond::ClosestUncoveredOppn(Interactor* pContext, PlanNode*, short* pResult, int)
{
    EngineObject* pTarget;
    AttachPak*    pCarrierSlot;
    EngineObject* pOther;
    Vec3          vFar;
    float         flDistNear;

    const short sSlot = pContext->GetActor()->GetSlot();

    pTarget = g_pGameEngine->m_ppSlots[TeammateSlot(sSlot)];

    PlayerObject* pOpponent = g_pGameEngine->GetPlayer(OPPOSING_FIRST(pContext->GetActor()->GetSlot()));

    const float flDamageNear = pOpponent->m_stats.GetScalar(RATING_DAMAGE);

    pOpponent = g_pGameEngine->GetPlayer(OPPOSING_SECOND(pContext->GetActor()->GetSlot()));

    float flDamageFar = pOpponent->m_stats.GetScalar(RATING_DAMAGE);

    if (flDamageNear <= DamageGate() && flDamageFar <= DamageGate())
    {
        *pResult = 2;
    }
    else
    {
        pCarrierSlot = g_pGameEngine->m_ppSlots[SLOT_ROK]->GetPhysicsBody()->pAttach;

        const int bNoCarrier = pCarrierSlot == 0 || pCarrierSlot->nSlot >= PLAYER_OBJECT_COUNT;

        if (bNoCarrier != 0)
        {
            *pResult = 2;
        }
        else
        {
            GameBehaviorObject* const pBehaviour = pTarget->GetPlanState()->m_pCurrent;

            if (pBehaviour->m_wKind == BEH_GLIDE_BETWEEN_FACING && ((pOther = ((BehObj_GlideBetweenFacing*)pBehaviour)->m_pAimObject) == 0 || pOther->GetSlot() <= PLAYER_SLOT_COUNT - 1))
            {
                const short sOther = pOther->GetSlot();

                const short sMasked = (short)TeammateSlot(sOther);

                if (sMasked >= pOther->GetSlot())
                {
                    *pResult = 1;
                }
                else
                {
                    *pResult = 0;
                }
            }
            else
            {
                unsigned char bySlot = (unsigned char)pContext->GetActor()->GetSlot();

                pOpponent = g_pGameEngine->GetPlayer(OPPOSING_FIRST(bySlot));

                const Vec3* pNearAt = pOpponent->GetPositionPtr();

                bySlot = (unsigned char)pContext->GetActor()->GetSlot();

                pOpponent = g_pGameEngine->GetPlayer(OPPOSING_SECOND(bySlot));

                const Vec3* pFarAt = pOpponent->GetPositionPtr();

                const Vec3* pOwnAt = pContext->GetActor()->GetPositionPtr();

                const float flNearX = pNearAt->x - pOwnAt->x;
                const float flNearY = pNearAt->y - pOwnAt->y;
                const float flNearZ = pNearAt->z - pOwnAt->z;

                vFar.x = pFarAt->x - pOwnAt->x;
                vFar.y = pFarAt->y - pOwnAt->y;
                vFar.z = pFarAt->z - pOwnAt->z;

                flDistNear            = (float)sqrt((flNearZ * flNearZ + flNearY * flNearY) + flNearX * flNearX);
                const float flDistFar = (float)sqrt((vFar.z * vFar.z + vFar.y * vFar.y) + vFar.x * vFar.x);

                if (flDistNear < flDistFar)
                {
                    *pResult = 0;
                }
                else
                {
                    *pResult = 1;
                }
            }
        }
    }

    if (*pResult == 0)
    {
        if (flDamageNear <= DamageGate())
        {
            *pResult = 1;

            return;
        }
    }

    if (*pResult == 1)
    {
        if (flDamageFar <= DamageGate())
        {
            *pResult = 0;
        }
    }
}

// Slots named Spin or ScoreSpin are the scoring spinners and are skipped; the answer is one-based.

// FUNCTION: HYPERX 0x00461960
void __cdecl PlanCond::NearestSuspenseMult(Interactor* pContext, PlanNode*, short* pResult, int)
{
    EngineObject* const pActor    = pContext->GetActor();
    const Vec3*         pPosition = pActor->GetPositionPtr();

    float flBest = 200000000.0f;
    int   nBest  = MODEL_ID_NONE;

    for (int nSlot = SLOT_MULTIPLIER; nSlot <= SLOT_MULTIPLIER_LAST; nSlot++)
    {
        EngineObject* const pMultiplier = g_pGameEngine->m_ppSlots[(short)nSlot];

        if (pMultiplier != 0)
        {
            const unsigned char* pszName = (const unsigned char*)pMultiplier->GetPlanState()->m_pCurrent->m_pszName;

            // STRING: HYPERX 0x004bcac8
            if (_mbscmp(pszName, (const unsigned char*)"Spin") != 0
                // STRING: HYPERX 0x004bcabc
                && _mbscmp(pszName, (const unsigned char*)"ScoreSpin") != 0)
            {
                const Vec3* pOther = pMultiplier->GetPositionPtr();

                const float flDx = pOther->x - pPosition->x;
                const float flDy = pOther->y - pPosition->y;
                const float flDz = pOther->z - pPosition->z;

                const float flRange = (float)sqrt(flDx * flDx + flDy * flDy + flDz * flDz);

                if (flRange < flBest)
                {
                    flBest = flRange;
                    nBest  = nSlot;
                }
            }
        }
    }

    if (nBest != MODEL_ID_NONE)
    {
        *pResult = (short)(nBest - (SLOT_MULTIPLIER - 1));
    }
    else
    {
        *pResult = 0;
    }
}

// FUNCTION: HYPERX 0x00461a50
void __cdecl PlanCond::OffenseOrWRok(Interactor* pContext, PlanNode*, short* pResult, int)
{
    short nSlot;
    int   nWide;

    AttachPak* const pSlot = g_pGameEngine->m_ppSlots[SLOT_ROK]->GetPhysicsBody()->pAttach;

    const bool bNoCarrier = pSlot == 0 || pSlot->nSlot >= PLAYER_OBJECT_COUNT;

    if (bNoCarrier)
    {
        *pResult = 2;

        return;
    }

    nSlot = pSlot->nSlot;

    EngineObject* pActor = pContext->GetActor();

    if (nSlot == pActor->GetSlot())
    {
        *pResult = 1;

        return;
    }

    pActor       = pContext->GetActor();
    short nOther = pActor->GetSlot();

    if (nOther < 4)
    {
        nWide = nOther + 2;
    }
    else
    {
        nWide = nOther;
    }

    nOther = (short)(nWide & 3);
    nSlot -= nOther;

    if (nSlot != 0)
    {
        *pResult = 2;
    }
    else
    {
        *pResult = 0;
    }
}

// FUNCTION: HYPERX 0x00461ae0
void PlanCond::HostIsViewOwner(Interactor*, PlanNode*, short* pResult, int)
{
    AttachPak* const pSlot = g_pGameEngine->m_ppSlots[SLOT_ROK]->GetPhysicsBody()->pAttach;

    if (pSlot != 0)
    {
        const short nSlot = pSlot->nSlot;

        if (nSlot < 6 && g_pGameEngine->m_nViewedSlot == nSlot)
        {
            *pResult = 0;

            return;
        }
    }

    *pResult = 1;
}

// FUNCTION: HYPERX 0x00461b20
void PlanCond::RokInWhichTeamsPoss(Interactor*, PlanNode*, short* pResult, int)
{
    AttachPak* const pSlot = g_pGameEngine->m_ppSlots[SLOT_ROK]->GetPhysicsBody()->pAttach;

    if (pSlot != 0)
    {
        const short nSlot = pSlot->nSlot;

        if (nSlot < 6)
        {
            *pResult = (signed char)(nSlot & 1);

            return;
        }
    }

    *pResult = 2;
}

// Zero when the rok is loose, else one to four by which skater holds it relative to the actor; a goalie holder leaves the answer alone.

// FUNCTION: HYPERX 0x00461b60
void __cdecl PlanCond::WhoHasRok(Interactor* pContext, PlanNode*, short* pResult, int)
{
    short nSlot;

    EngineObject* const pActor = pContext->GetActor();
    AttachPak* const    pSlot  = g_pGameEngine->m_ppSlots[SLOT_ROK]->GetPhysicsBody()->pAttach;

    if (pSlot != 0)
    {
        nSlot = pSlot->nSlot;

        if (nSlot < PLAYER_OBJECT_COUNT)
        {
            const int nTeamWide = pActor->GetSlot();
            short     nTeam     = (short)nTeamWide;
            int       nOther    = nTeam;

            if (nTeam < 4)
            {
                nOther += 2;
            }

            if (nSlot == nTeamWide)
            {
                *pResult = 1;

                return;
            }

            if (nSlot == (short)(nOther & 3))
            {
                *pResult = 2;

                return;
            }

            nTeam = (short)(~nTeam & 1);

            if (nSlot == nTeam)
            {
                *pResult = 3;

                return;
            }

            if (nSlot == (short)(nTeam | 2))
            {
                *pResult = 4;

                return;
            }

            return;
        }
    }

    *pResult = 0;
}

// FUNCTION: HYPERX 0x00461c00
void PlanCond::IsDoubleScore(Interactor* pContext, PlanNode*, short* pResult, int)
{
    if (g_pGameEngine->m_ppSlots[SLOT_ROK]->GetPhysicsBody()->pAttach != 0)
    {
        *pResult = 2;

        return;
    }

    PlayerObject* const pActor = pContext->GetPlayerActor();

    // Only PlayerObject has m_wTargetGoalSlot.
    *pResult = pActor->m_wTargetGoalSlot == 0;
}

// The low nibble of the flag launches the pass; clear, it only announces it.
#define LAUNCH_MASK 0x0f

#define LATE_PASS_STEP 4

#define BUNGEE_GAIN     1.85f
#define BUNGEE_DOT_GATE 0.7f

// The word at 0x004d2b82, named and marked in src/planvm/planactionrecoil.cpp.
extern short g_nCheckImpulseFacing;

// FUNCTION: HYPERX 0x00461c40
void __cdecl PlanCond::BungeeVictimLorR(Interactor* pContext, PlanNode*, short* pResult, int nFlags)
{
    EngineObject*  pTarget;
    CBungeeToward* pBungee;
    float          flScore;

    if ((nFlags & LAUNCH_MASK) != 0)
    {
        pTarget = Player::FindBungeeVictim(pContext->GetActor(), &flScore);

        *pResult = 0;

        if (pTarget != 0)
        {
            pBungee = (CBungeeToward*)AllocAgendaItem(Agenda_CBungeeToward);

            GameBehaviorObject* const pBindNode = pContext->GetActor()->GetPlanState()->m_pCurrent;

            pBungee->Bind(pContext->GetActor(), pBindNode);

            pBungee->m_pTarget   = pTarget;
            pBungee->m_flGain    = BUNGEE_GAIN;
            pBungee->m_flDotGate = BUNGEE_DOT_GATE;

            pBungee->SetNearRange(g_flOnePlan);

            pBungee->Activate();
        }
    }
    else
    {
        pTarget = Player::FindBungeeVictim(pContext->GetActor(), &flScore);

        const int nLate = (g_nCheckImpulseFacing >= LATE_PASS_STEP);

        if (pTarget != 0)
        {
            g_pGameEngine->m_ppSlots[SLOT_SOUND_MAN]->PostSceneEvent(MSG_YER_ATTACKED, 0);
        }

        *pResult = (short)(nLate + 1);
    }
}

// The stat handler is asked twice with the same index and its two answers are used differently.

// FUNCTION: HYPERX 0x00461d20
void __cdecl PlanAction::IncrementStat(Interactor* pContext, PlanNode* pArgs, short*, int)
{
    EngineObject* pSource;
    PlayerObject* pTarget;

    pContext->GetActor();

    if (pArgs->m_pArgs[1].IsPronoun())
    {
        pSource = g_pGameEngine->m_ppSlots[g_apPlanStatFns[pArgs->m_pArgs[1].wPronoun](
            pContext->GetActor(), pArgs)];

        pTarget = (PlayerObject*)g_pGameEngine->m_ppSlots[g_apPlanStatFns[pArgs->m_pArgs[1].wPronoun](
            pContext->GetActor(), pArgs)];

        pTarget->m_matchStats.AccumulateStat(pArgs->m_pArgs[2].n,
                                             pSource->GetSlot());
    }
}

#define BUNGEE_NEAR_RANGE 1.5f

// A packed argument whose top half is 0xfedc names its entity slot through a pronoun.
// FUNCTION: HYPERX 0x00461dc0
void __cdecl PlanAction::StartStatTimer(Interactor* pContext, PlanNode* pArgs, short*, int)
{
    PlanWord* const pBlock = pArgs->m_pArgs;

    if (pBlock[1].IsPronoun())
    {
        const short nSlot = g_apPlanStatFns[pBlock[1].wPronoun](
            pContext->GetActor(), pArgs);
        PlayerObject* const pEntity = g_pGameEngine->GetPlayer(nSlot);

        if (nSlot >= 0 && nSlot < ENTITY_SLOT_LIMIT && pEntity != 0)
        {
            pEntity->m_matchStats.StartTimer(pArgs->m_pArgs[2].n);
        }
    }
}

// FUNCTION: HYPERX 0x00461e30
void __cdecl PlanAction::EndStatTimer(Interactor* pContext, PlanNode* pArgs, short*, int)
{
    PlanWord* const pBlock = pArgs->m_pArgs;

    if (pBlock[1].IsPronoun())
    {
        const short nSlot = g_apPlanStatFns[pBlock[1].wPronoun](
            pContext->GetActor(), pArgs);
        PlayerObject* const pEntity = g_pGameEngine->GetPlayer(nSlot);

        if (nSlot >= 0 && nSlot < ENTITY_SLOT_LIMIT && pEntity != 0)
        {
            pEntity->m_matchStats.StopTimer(pArgs->m_pArgs[2].n);
        }
    }
}

// FUNCTION: HYPERX 0x00461ea0
void __cdecl PlanAction::ScaleBubToRating(Interactor* pContext, PlanNode* pNode, short*, int)
{
    BoundBubble* const pColObj = g_pGameEngine->GetColObjPtr((short)pNode->m_pArgs[ARG_COL_OBJECT].n);

    PlayerObject* const pActor = pContext->GetPlayerActor();

    const float flStat = pActor->m_stats.GetScalar(pNode->m_pArgs[ARG_STAT].n);

    const float flLow = PlanArg::ResolveFloat(pContext, pNode, 3);

    pColObj->m_flRadius = (PlanArg::ResolveFloat(pContext, pNode, 4) - flLow) * (flStat * g_flPlanStatScale) + flLow;
}

// Slot one on the tracked object answers a short, read here as a slot and elsewhere as a team.

// FUNCTION: HYPERX 0x00461f30
void PlanAction::SetTargetWithData(Interactor* pContext, PlanNode*, short*, int nArg)
{
    const short nIndex = (short)nArg;

    Vec3 vToTracked;
    Vec3 vToCandidate;

    EngineObject*       pActor     = pContext->GetActor();
    PlanState*          pPlanState = pActor->GetPlanState();
    CSteerToTarget*     pSteer     = (CSteerToTarget*)pPlanState->m_pCurrent->m_apSlots[0];
    EngineObject* const pTracked   = pSteer->m_pTarget;

    EngineObject* const pCandidate = g_pGameEngine->m_ppSlots[nIndex];

    if (pTracked != 0)
    {
        pActor = pContext->GetActor();

        Vec3* const pSelfPos      = pActor->GetPositionPtr();
        Vec3* const pTrackedPos   = pTracked->GetPositionPtr();
        Vec3* const pCandidatePos = pCandidate->GetPositionPtr();

        vToTracked.x = pSelfPos->x - pTrackedPos->x;
        vToTracked.y = pSelfPos->y - pTrackedPos->y;
        vToTracked.z = pSelfPos->z - pTrackedPos->z;

        vToCandidate.x = pSelfPos->x - pCandidatePos->x;
        vToCandidate.y = pSelfPos->y - pCandidatePos->y;
        vToCandidate.z = pSelfPos->z - pCandidatePos->z;

        short nTracked = pTracked->GetSlot();

        if (Vec3::DotInline(&vToTracked, &vToTracked) > Vec3::DotInline(&vToCandidate, &vToCandidate) && nTracked != nIndex)
        {
            pActor     = pContext->GetActor();
            pPlanState = pActor->GetPlanState();
            pSteer     = (CSteerToTarget*)pPlanState->m_pCurrent->m_apSlots[0];

            pSteer->SetTargetObjectByIndex(nIndex);
        }
    }
}

// Points a fresh bungee at whatever the current behaviour's first bungee tracks, if anything.
// FUNCTION: HYPERX 0x00462040
void __cdecl PlanAction::BungeeTowardTarget(Interactor* pContext, PlanNode*, short*, int)
{
    EngineObject* pTracked;

    pTracked = ((CBungeeToward*)pContext->GetActor()->GetPlanState()->m_pCurrent->m_apSlots[0])->m_pTarget;

    if (pTracked != 0)
    {
        CBungeeToward* const pTrack = (CBungeeToward*)AllocAgendaItem(Agenda_CBungeeToward);

        GameBehaviorObject* const pBindNode = pContext->GetActor()->GetPlanState()->m_pCurrent;

        pTrack->Bind(pContext->GetActor(), pBindNode);

        pTrack->m_pTarget     = pTracked;
        pTrack->m_flGain      = BUNGEE_GAIN;
        pTrack->m_flDotGate   = BUNGEE_DOT_GATE;
        pTrack->m_flNearRange = BUNGEE_NEAR_RANGE;

        pTrack->Activate();
    }
}

// Zero only when the owner carries the rok and the mate is over twenty further along.

// The fifty the owner's energy is measured against.
#define ENERGY_SPLIT 50.0

// FUNCTION: HYPERX 0x004620c0
void __cdecl PlanCond::Decide_PassOrRun(Interactor* pContext, PlanNode*, short* pResult, int)
{
    float flSign;
    short nTracked;

    const short nSlot = pContext->GetActor()->GetSlot();

    const short nMate = (short)TeammateSlot(nSlot);

    EngineObject** const apObjects = g_pGameEngine->m_ppSlots;

    PlayerObject* const pActor = (PlayerObject*)apObjects[nSlot];

    if (pActor->m_wTargetGoalSlot == SLOT_NGOAL)
    {
        flSign = g_flOnePlan;
    }
    else
    {
        flSign = -g_flOnePlan;
    }

    PhysicsBody* const pBlock = apObjects[SLOT_ROK]->GetPhysicsBody();

    AttachPak* const pnTracked = pBlock->pAttach;

    if (pnTracked != 0 && (nTracked = pnTracked->nSlot) < PLAYER_OBJECT_COUNT && nTracked == nSlot)
    {
        EngineObject* const pMate = g_pGameEngine->m_ppSlots[nMate];

        const float flLimit = flSign * pActor->GetPositionPtr()->y + 20.0f;

        if (flSign * pMate->GetPositionPtr()->y > flLimit)
        {
            *pResult = 0;

            return;
        }
    }

    if (pActor->m_stats.m_aStats[0] < ENERGY_SPLIT)
    {
        *pResult = 1;

        return;
    }

    *pResult = 2;
}

// When the owner carries the rok, adds fifty to its energy, clamped to nought and a hundred.

// The ceiling, as a float for the assignment.
#define SPEED_CEILING 100.0f

// The window the facing word has to sit in to answer one.
#define FACING_MAX 0.75f
#define FACING_MIN 0.1f

// Two, negated at run time: the height the body's pos.z has to beat.
// GLOBAL: HYPERX 0x004b36f0
extern const float g_flTwoPlan;

// FUNCTION: HYPERX 0x004621b0
void __cdecl PlanCond::Decide_HPSpin_Rail_orNot(Interactor* pContext, PlanNode*, short* pResult, int)
{
    AttachPak* pnTracked;
    short      nTracked;

    PhysicsBody* pBlock = pContext->GetActor()->GetPhysicsBody();

    if (-g_flTwoPlan < pBlock->pos.z)
    {
        if (pBlock->velDir.z < FACING_MAX && pBlock->velDir.z >= FACING_MIN)
        {
            *pResult = 1;

            return;
        }

        *pResult = 0;
    }
    else
    {
        *pResult = 2;
    }

    pnTracked = g_pGameEngine->m_ppSlots[SLOT_ROK]->GetPhysicsBody()->pAttach;

    if (pnTracked != 0 && (nTracked = pnTracked->nSlot) < PLAYER_OBJECT_COUNT && nTracked == pContext->GetActor()->GetSlot())
    {
        const float flEnergy = pContext->GetPlayerActor()->m_stats.m_aStats[0];

        EngineObject* const pDest = pContext->GetActor();

        float flRaised = flEnergy;

        flRaised += 50.0f;

        if (flRaised > 100.0)
        {
            flRaised = SPEED_CEILING;
        }
        else if (flRaised < 0.0)
        {
            flRaised = g_flZeroPlan;
        }

        ((PlayerObject*)pDest)->m_stats.m_aStats[0] = flRaised;
    }
}

// The rail-hug impulse, behaviour six in HUMAN.PBN; its vertical part is never below one.

// FUNCTION: HYPERX 0x004622b0
void __cdecl PlanAction::KickOffRim(Interactor* pContext, PlanNode*, short*, int)
{
    Vec3 vDirection;

    EngineObject* const pActor    = pContext->GetActor();
    PhysicsBody* const  pBody     = pActor->GetPhysicsBody();
    Vec3                vPosition = *pActor->GetPositionPtr();

    vDirection.x = -vPosition.x;
    vDirection.y = -vPosition.y;
    vDirection.z = 1.0f;

    Math::Vec3Normalize(&vDirection);

    vDirection.z += 1.0f;

    const float flStep = g_pGameEngine->m_clock.flDelta * 15.0f;

    pBody->vel.x = flStep * vDirection.x + pBody->vel.x;
    pBody->vel.y = flStep * vDirection.y + pBody->vel.y;
    pBody->vel.z = flStep * vDirection.z + pBody->vel.z;
}

// The two axis tests answer two and one, and the answer is their sum.

// FUNCTION: HYPERX 0x00462360
void PlanCond::PickACell(Interactor* pContext, PlanNode*, short* pResult, int)
{
    EngineObject* const pActor    = pContext->GetActor();
    Vec3* const         pPosition = pActor->GetPositionPtr();

    short nHalf;

    if (pPosition->y > g_flZeroPlan)
    {
        nHalf = 0;
    }
    else
    {
        nHalf = 2;
    }

    short nSide;

    if (pPosition->x > g_flZeroPlan)
    {
        nSide = 0;
    }
    else
    {
        nSide = 1;
    }

    *pResult = nHalf + nSide;
}

// FUNCTION: HYPERX 0x004623c0
void PlanCond::Random(Interactor*, PlanNode* pArgs, short* pResult, int)
{
    *pResult = (short)(rand() * (pArgs->m_nArgCount - 1) / (RAND_MAX + 1));
}

// Turns the head camera's shot power into a child index, inverted and clamped.

// FUNCTION: HYPERX 0x004623f0
void __cdecl PlanCond::ShotPower(Interactor*, PlanNode* pArgs, short* pResult, int)
{
    const int nScaled = g_pGameEngine->GetCamera(CAMERA_HEAD_SLOT)->m_nShotPowerLevel * (pArgs->m_nArgCount - 1);

    const int nPosition = (int)((double)nScaled * g_dPeriodScale);

    *pResult = (short)(1 - nPosition);

    if (*pResult > pArgs->m_nArgCount - 2)
    {
        *pResult = (short)(pArgs->m_nArgCount - 2);
    }
}

#define SPAWN_GRID_HALF  28
#define SPAWN_GRID_PITCH 56

#define SPAWN_SLOT_LIMIT 3
#define SPAWN_AXES       4

#define SPAWN_RESULT_OUT         1
#define SPAWN_RESULT_TEAM_MOVING 2

// The third axis difference is always zero: both operands are the same shared zero.
// FUNCTION: HYPERX 0x00462450
void __cdecl PlanCond::IfInFaceOff(Interactor*, PlanNode*, short* pResult, int)
{
    Vec3  vSquare;
    float flLimit;
    int   nSlot;

    *pResult = 0;

    for (nSlot = 0; (short)nSlot <= SPAWN_SLOT_LIMIT; nSlot++)
    {
        PlayerObject* const pObject = g_pGameEngine->GetPlayer((short)nSlot);

        vSquare.x = (float)(SPAWN_GRID_HALF - (short)nSlot / 2 * SPAWN_GRID_PITCH);
        vSquare.y = (float)(SPAWN_GRID_HALF - (short)nSlot % 2 * SPAWN_GRID_PITCH);
        vSquare.z = g_flZeroPlan;

        Vec3 vPosition = *pObject->GetPositionPtr();

        const float flDx = vPosition.x - vSquare.x;
        const float flDy = vPosition.y - vSquare.y;
        const float flDz = g_flZeroPlan - vSquare.z;

        const float flSq = flDx * flDx;

        if ((flSq + flDy * flDy) + flDz * flDz > g_flSpawnEscapeDistance)
        {
            flLimit = DamageGate();

            if (pObject->m_stats.GetScalar(RATING_DAMAGE) > flLimit)
            {
                *pResult = SPAWN_RESULT_OUT;
                return;
            }

            int nMoving = 0;
            int nAxis   = 0;
            int nLeft   = SPAWN_AXES;

            do
            {
                if (g_pGameEngine->IsFitSubstitute((short)nSlot, nAxis) != 0)
                {
                    nMoving++;
                }

                nAxis++;
                nLeft--;
            } while (nLeft != 0);

            if (nMoving != 0)
            {
                *pResult = SPAWN_RESULT_OUT;
                return;
            }
        }
    }

    if (g_pGameEngine->IsHomeTeamInjured() != 0 || g_pGameEngine->IsAwayTeamInjured() != 0)
    {
        *pResult = SPAWN_RESULT_TEAM_MOVING;
    }
}

#define ARENA_TRACK_UP   7
#define ARENA_TRACK_DOWN 8

// FUNCTION: HYPERX 0x00462620
void PlanAction::EndPeriodNow(Interactor*, PlanNode*, short*, int)
{
    g_pGameEngine->m_bEndPeriodRequested = 1;
}

// Three when both teams say yes, one for the first only, two for the second only, else zero.

// FUNCTION: HYPERX 0x00462630
void PlanCond::TeamIsDead(Interactor*, PlanNode*, short* pResult, int)
{
    const unsigned char bHome = g_pGameEngine->IsHomeTeamDead();
    unsigned char       bAway = g_pGameEngine->IsAwayTeamDead();

    if (bHome != 0 && bAway != 0)
    {
        *pResult = 3;

        return;
    }

    if (bHome == 0 && bAway != 0)
    {
        *pResult = 2;

        return;
    }

    if (bHome != 0 && bAway == 0)
    {
        *pResult = 1;

        return;
    }

    *pResult = 0;
}

// FUNCTION: HYPERX 0x00462680
void PlanCond::TeamIsInjured(Interactor*, PlanNode*, short* pResult, int)
{
    const unsigned char bHome = g_pGameEngine->IsHomeTeamInjured();
    unsigned char       bAway = g_pGameEngine->IsAwayTeamInjured();

    if (bHome != 0 && bAway != 0)
    {
        *pResult = 3;

        return;
    }

    if (bHome == 0 && bAway != 0)
    {
        *pResult = 2;

        return;
    }

    if (bHome != 0 && bAway == 0)
    {
        *pResult = 1;

        return;
    }

    *pResult = 0;
}

// FUNCTION: HYPERX 0x004626d0
void PlanCond::MyTeamIsInjured(Interactor* pContext, PlanNode*, short* pResult, int)
{
    EngineObject* const pActor = pContext->GetActor();

    if (pActor->GetSlot() % 2 == 0)
    {
        *pResult = g_pGameEngine->IsHomeTeamInjured() != 0;
    }
    else
    {
        *pResult = g_pGameEngine->IsAwayTeamInjured() != 0;
    }
}

// One means team two won: a dead team loses outright, and a draw goes to team two.

// FUNCTION: HYPERX 0x00462720
void PlanCond::WhoWonTheGame(Interactor*, PlanNode*, short* pResult, int)
{
    if (g_pGameEngine->IsHomeTeamDead() != 0)
    {
        *pResult = 1;
    }
    else if (g_pGameEngine->IsAwayTeamDead() != 0)
    {
        *pResult = 0;
    }
    else
    {
        *pResult = (short)(g_pGameEngine->GetHomeScore() <= g_pGameEngine->GetAwayScore());
    }
}

// Two is the answer when the target and the actor are the same object.

// FUNCTION: HYPERX 0x00462780
void PlanCond::IsSenderTeammate(Interactor* pContext, PlanNode*, short* pResult, int)
{
    // Not TeammateSlot: the client adds two without an else arm (0x00462794 to 0x004627a8).
    const short nOwnSlot = pContext->GetActor()->GetSlot();

    int nSlot = nOwnSlot;

    if (nOwnSlot < PLAYER_SLOT_COUNT)
    {
        nSlot += 2;
    }

    nSlot &= PLAYER_SLOT_MASK;

    *pResult = pContext->GetTargetSlot() != (short)nSlot;

    EngineObject* const pActor = pContext->GetActor();

    if (pContext->GetTargetSlot() == pActor->GetSlot())
    {
        *pResult = 2;
    }
}

// FUNCTION: HYPERX 0x004627f0
void PlanCond::AmMainDromer(Interactor* pContext, PlanNode*, short* pResult, int)
{
    EngineObject* const pActor = pContext->GetActor();

    *pResult = pActor->GetSlot() >= 2;
}

// FUNCTION: HYPERX 0x00462820
void PlanCond::MyID(Interactor* pContext, PlanNode*, short* pResult, int)
{
    EngineObject* pActor = pContext->GetActor();

    if (pActor->GetSlot() < PLAYER_OBJECT_COUNT)
    {
        pActor = pContext->GetActor();

        *pResult = pActor->GetSlot();
    }
    else
    {
        *pResult = g_pGameEngine->m_nViewedSlot;
    }
}

// FUNCTION: HYPERX 0x00462870
void PlanCond::MessageDataNibl0(Interactor*, PlanNode*, short* pResult, int nArg)
{
    *pResult = MessageNibble(nArg, 0);
}

// FUNCTION: HYPERX 0x00462880
void PlanCond::MessageDataNibl1(Interactor*, PlanNode*, short* pResult, int nArg)
{
    *pResult = MessageNibble(nArg, 4);
}

// FUNCTION: HYPERX 0x004628a0
void PlanCond::MessageDataNibl4(Interactor*, PlanNode*, short* pResult, int nArg)
{
    *pResult = MessageNibble(nArg, 16);
}

// FUNCTION: HYPERX 0x004628c0
void PlanCond::MsgData4Nibl1X2Nibl0(Interactor*, PlanNode*, short* pResult, int nArg)
{
    *pResult = MessageNibble(nArg, 0) + MessageNibble(nArg, 4) * 2;
}

// FUNCTION: HYPERX 0x004628e0
void PlanCond::SideKeysAreDown(Interactor*, PlanNode*, short* pResult, int)
{
    EngineObject* pObject;

    pObject                   = g_pGameEngine->m_ppSlots[g_pGameEngine->m_nViewedSlot];
    PhysicsBody* const pState = pObject->GetPhysicsBody();

    *pResult = ((char)pState->nHeldBindings & HELD_ROTATE) == 0;
}

// FUNCTION: HYPERX 0x00462910
void __cdecl PlanCond::AmViewOwner(Interactor* pContext, PlanNode*, short* pResult, int)
{
    // Nought when the actor is either tracked object.
    *pResult = pContext->GetActor()->GetSlot() != g_pGameEngine->m_apTracked[0]->GetSlot() && pContext->GetActor()->GetSlot() != g_pGameEngine->m_apTracked[1]->GetSlot();
}

// An actor on the Input plan stands in for the viewed slot when finding its team mate.
// FUNCTION: HYPERX 0x00462990
void __cdecl PlanCond::TeammateIsViewOwner(Interactor* pContext, PlanNode*, short* pResult, int)
{
    EngineObject* const pActor = pContext->GetActor();

    int nTeam = pActor->GetSlot();

    const unsigned char* pszName = (const unsigned char*)pActor->GetPlanState()->m_pszName;

    // STRING: HYPERX 0x004c45f4
    if (_mbscmp((const unsigned char*)"Input", pszName) == 0)
    {
        nTeam = g_pGameEngine->m_nViewedSlot;
    }

    *pResult = (unsigned short)(
        // STRING: HYPERX 0x004bb340
        _mbscmp((const unsigned char*)"Human",
                (const unsigned char*)g_pGameEngine->m_ppSlots[TeammateSlot((short)nTeam)]
                    ->GetPlanState()
                    ->m_pszName) != 0);
}

// Answers one for a bot team mate that is fit and wants no substitute, else nought.
// FUNCTION: HYPERX 0x00462a10
void __cdecl PlanCond::TeammateIsHumanOrInjured(Interactor* pContext, PlanNode*, short* pResult, int)
{
    short nIsBot;

    EngineObject* const pActor = pContext->GetActor();

    int nTeam = pActor->GetSlot();

    const unsigned char* pszName = (const unsigned char*)pActor->GetPlanState()->m_pszName;

    // STRING: HYPERX 0x004c45f4
    if (_mbscmp((const unsigned char*)"Input", pszName) == 0)
    {
        nTeam = g_pGameEngine->m_nViewedSlot;
    }

    const short nSlot = (short)TeammateSlot((short)nTeam);

    nIsBot = (short)(
        // STRING: HYPERX 0x004bb340
        _mbscmp((const unsigned char*)"Human",
                (const unsigned char*)g_pGameEngine->m_ppSlots[nSlot]->GetPlanState()->m_pszName) != 0);

    *pResult = nIsBot;

    if (nIsBot == 1)
    {
        PlayerObject* const pBody = g_pGameEngine->GetPlayer(nSlot);

        const float flDamage = pBody->m_stats.GetScalar(RATING_DAMAGE);

        if (flDamage <= DamageGate() || g_pGameEngine->GetPlayer(nSlot)->m_wDesiredSubstitute != -1)
        {
            *pResult = 0;
        }
    }
}

// FUNCTION: HYPERX 0x00462b30
void __cdecl PlanCond::OkToSwitchToTeammate(Interactor* pContext, PlanNode*, short* pResult, int)
{
    const short nTeam = pContext->GetActor()->GetSlot();

    const short nSlot = (short)TeammateSlot(nTeam);

    *pResult = (short)(
        // STRING: HYPERX 0x004bb340
        _mbscmp((const unsigned char*)"Human",
                (const unsigned char*)g_pGameEngine->m_ppSlots[nSlot]->GetPlanState()->m_pszName) != 0);

    PlayerObject* const pOther = g_pGameEngine->GetPlayer(nSlot);

    PlayerObject* const pActor = pContext->GetPlayerActor();

    if (*pResult == 1)
    {
        const float flDamage = pOther->m_stats.GetScalar(RATING_DAMAGE);

        if (flDamage <= DamageGate() || pActor->m_wClockStoppedSemaphore != 0 || pOther->m_wClockStoppedSemaphore != 0 || pOther->m_wDesiredSubstitute != -1)
        {
            *pResult = 0;
        }
    }
}

// FUNCTION: HYPERX 0x00462c40
void __cdecl PlanCond::OkToPassToTeammate(Interactor* pContext, PlanNode*, short* pResult, int)
{
    const short nTeam = pContext->GetActor()->GetSlot();

    const short nSlot = (short)TeammateSlot(nTeam);

    PlayerObject* const pActor = pContext->GetPlayerActor();

    PlayerObject* const pOther = g_pGameEngine->GetPlayer(nSlot);

    *pResult = 1;

    PlayerObject* const pBody = g_pGameEngine->GetPlayer(nSlot);

    const float flDamage = pBody->m_stats.GetScalar(RATING_DAMAGE);

    if (flDamage <= DamageGate() || pActor->m_wClockStoppedSemaphore != 0 || pOther->m_wClockStoppedSemaphore != 0 || pOther->m_wDesiredSubstitute != -1)
    {
        *pResult = 0;
    }
}

// Five below the damage gate and four for the actor's m_wClockStoppedSemaphore, before the rok is asked.

// The slot two along, wrapped: zero pairs with two and one with three.
#define SLOT_OPPOSITE_STEP  2
#define SLOT_OPPOSITE_WRAP  3
#define SLOT_OPPOSITE_BELOW 4

// The half of the four the tail answers with.
#define SLOT_FAR_HALF 2

#define ROK_TARGETED 2

#define STATE_HELD  4
#define STATE_GATED 5

// FUNCTION: HYPERX 0x00462d20
void __cdecl PlanCond::DecidePosition(Interactor* pContext, PlanNode*, short* pResult, int)
{
    AttachPak* pnTargetSlot;
    int        nOpposite;
    float      flLimit;

    PlayerObject* const pActor = pContext->GetPlayerActor();

    const float flDamage = pActor->m_stats.GetScalar(RATING_DAMAGE);

    flLimit = DamageGate();

    if (flDamage <= flLimit)
    {
        *pResult = STATE_GATED;

        return;
    }

    if (pActor->m_wClockStoppedSemaphore != 0)
    {
        *pResult = STATE_HELD;

        return;
    }

    pnTargetSlot = g_pGameEngine->m_ppSlots[SLOT_ROK]->GetPhysicsBody()->pAttach;

    if (pnTargetSlot != 0)
    {
        const short nTarget = pnTargetSlot->nSlot;

        if (nTarget < PLAYER_OBJECT_COUNT)
        {
            const short nOwn = pContext->GetActor()->GetSlot();

            if (nTarget != nOwn)
            {
                const short nOther = pContext->GetActor()->GetSlot();

                if (nOther < SLOT_OPPOSITE_BELOW)
                {
                    nOpposite = nOther + SLOT_OPPOSITE_STEP;
                }
                else
                {
                    nOpposite = nOther;
                }

                if (nTarget != (short)(nOpposite & SLOT_OPPOSITE_WRAP))
                {
                    *pResult = (unsigned short)(pContext->GetActor()->GetSlot() >= SLOT_FAR_HALF);

                    return;
                }
            }

            *pResult = (unsigned short)((nTarget != pContext->GetActor()->GetSlot()) + ROK_TARGETED);

            return;
        }
    }

    *pResult = (unsigned short)(pContext->GetActor()->GetSlot() >= SLOT_FAR_HALF);
}

// FUNCTION: HYPERX 0x00462e50
void __cdecl PlanCond::CollisionDetail(Interactor* pContext, PlanNode*, short* pResult, int)
{
    // The first object is fetched only to be checked: nothing reads it afterwards.
    g_pGameEngine->GetColObjPtr(pContext->GetOtherSlot());

    BoundBubble* const pCollision = g_pGameEngine->GetColObjPtr(pContext->GetModelSlot());

    PhysicsBody* const pBody = pCollision->GetOwner()->GetPhysicsBody();

    if (pBody->flSpeed < 1.0f)
    {
        *pResult = 0;

        return;
    }

    *pResult = 1;
}

// FUNCTION: HYPERX 0x00462ee0
void PlanCond::OwnerIsViewOwner(Interactor*, PlanNode*, short* pResult, int)
{
    AttachPak* const pSlot = g_pGameEngine->m_ppSlots[SLOT_ROK]->GetPhysicsBody()->pAttach;

    if (pSlot != 0)
    {
        short nSlot = pSlot->nSlot;

        if (nSlot < 6 && nSlot == g_pGameEngine->m_nViewedSlot)
        {
            *pResult = 0;

            return;
        }
    }

    *pResult = 1;
}

#define SLOT_WRAP   0x03
#define SLOT_PAIRED 0x04

// FUNCTION: HYPERX 0x00462f20
void __cdecl PlanCond::ViewOwnersMateInFront(Interactor*, PlanNode*, short* pOutFacing, int)
{
    Vec3* pThere;
    short nFacing;
    int   nIndex;
    Vec3  vToThere;

    EngineObject* pTracked = g_pGameEngine->m_ppSlots[g_pGameEngine->m_nViewedSlot];

    Vec3* const pHere = pTracked->GetPositionPtr();

    const short nSlot = g_pGameEngine->m_nViewedSlot;

    if (nSlot < SLOT_PAIRED)
    {
        nIndex = nSlot + 2;
    }
    else
    {
        nIndex = nSlot;
    }

    nIndex &= SLOT_WRAP;

    pThere = g_pGameEngine->m_ppSlots[nIndex]->GetPositionPtr();

    vToThere.x = pThere->x - pHere->x;
    vToThere.y = pThere->y - pHere->y;
    vToThere.z = pThere->z - pHere->z;

    const Vec3* pForward = &pTracked->GetPhysicsBody()->velDir;

    if (vToThere.z * pForward->z + vToThere.y * pForward->y + vToThere.x * pForward->x > 0.0f)
    {
        nFacing = 0;
    }
    else
    {
        nFacing = 1;
    }

    *pOutFacing = nFacing;
}

// FUNCTION: HYPERX 0x00462fd0
void __cdecl PlanCond::IHaveRok(Interactor* pContext, PlanNode*, short* pResult, int nFlag)
{
    const AttachPak* pCarrier;
    short            nCarrier;

    if (pContext->GetOtherSlot() < 0)
    {
        // The flagged query answers two and three where the plain one answers nought and one.
        if (nFlag != 0)
        {
            pCarrier = g_pGameEngine->m_ppSlots[SLOT_ROK]->GetPhysicsBody()->pAttach;

            if (pCarrier != 0 && (nCarrier = pCarrier->nSlot) < PLAYER_OBJECT_COUNT && nCarrier == pContext->GetActor()->GetSlot())
            {
                *pResult = 2;

                return;
            }

            *pResult = 3;

            return;
        }

        pCarrier = g_pGameEngine->m_ppSlots[SLOT_ROK]->GetPhysicsBody()->pAttach;

        if (pCarrier != 0 && (nCarrier = pCarrier->nSlot) < PLAYER_OBJECT_COUNT && nCarrier == pContext->GetActor()->GetSlot())
        {
            *pResult = 0;

            return;
        }

        *pResult = 1;

        return;
    }

    pCarrier = g_pGameEngine->m_ppSlots[SLOT_ROK]->GetPhysicsBody()->pAttach;

    if (pCarrier != 0 && (nCarrier = pCarrier->nSlot) < PLAYER_OBJECT_COUNT && nCarrier == CollisionObj::GetModelId(pContext->GetModelSlot()))
    {
        *pResult = 0;

        return;
    }

    *pResult = 1;
}

// Inverted: zero when the carrier matches; the tag test cannot fire, as the slot is narrowed first.
// FUNCTION: HYPERX 0x004630d0
void __cdecl PlanCond::DromerHasRok(Interactor* pContext, PlanNode* pArgs, short* pResult, int nPrevSlot)
{
    int nSlot;

    if (pContext->GetNamedSlot() == -1)
    {
        nSlot = CollisionObj::GetModelId(pContext->GetModelSlot());
    }
    else
    {
        nSlot = nPrevSlot;
    }

    // The client tests the short at 0x004630fe and indexes the int at 0x00463115.
    if ((unsigned short)((unsigned int)(short)nSlot >> 16) == PLAN_ARG_PRONOUN)
    {
        nSlot = ((PlanStatFnWide*)g_apPlanStatFns)[nSlot & 0xffff](pContext->GetActor(), pArgs);
    }

    AttachPak* pCarrier = g_pGameEngine->m_ppSlots[SLOT_ROK]->GetPhysicsBody()->pAttach;

    if (pCarrier != 0 && pCarrier->nSlot < PLAYER_OBJECT_COUNT && pCarrier->nSlot == (short)nSlot)
    {
        *pResult = 0;
        return;
    }

    *pResult = 1;
}

// FUNCTION: HYPERX 0x00463170
void PlanCond::SetResultDirect(Interactor*, PlanNode*, short* pResult, int nValue)
{
    *pResult = (short)nValue;
}

// FUNCTION: HYPERX 0x00463180
void __cdecl PlanCond::IsRokInMyZone(Interactor* pContext, PlanNode*, short* pResult, int)
{
    int nAnswer;

    if ((pContext->GetActor()->GetSlot() & 1) != 0)
    {
        nAnswer = g_pGameEngine->m_ppSlots[SLOT_ROK]->GetPositionPtr()->y > g_flZeroPlan;
    }
    else
    {
        nAnswer = g_pGameEngine->m_ppSlots[SLOT_ROK]->GetPositionPtr()->y < g_flZeroPlan;
    }

    *pResult = (short)nAnswer;
}

// FUNCTION: HYPERX 0x004631f0
void PlanAction::SetRenderFlag(Interactor* pContext, PlanNode* pArgs, short*, int)
{
    EngineObject* pActor;

    if (pArgs->m_pArgs[1].n != 0)
    {
        pActor = pContext->GetActor();

        pActor->m_nFlags &= ~ENGINEOBJECT_HIDDEN;
    }
    else
    {
        pActor = pContext->GetActor();

        pActor->m_nFlags |= ENGINEOBJECT_HIDDEN;
    }
}

// The upper arm announces once more going up than the lower one does coming down.

// FUNCTION: HYPERX 0x00463300
void __cdecl PlanAction::IncRokValue(Interactor*, PlanNode* pArgs, short*, int nArg)
{
    const unsigned char nFlag = (unsigned char)nArg;

    short nRequest;

    EngineObject** const pSlots = g_pGameEngine->m_ppSlots;
    RokObject* const     pRok   = (RokObject*)pSlots[SLOT_ROK];
    short                nMode  = pRok->m_wRokValue;

    if (nMode == 1)
    {
        nMode = 0;
    }

    if (pArgs->m_pArgs[1].n == PLAN_TOKEN_MESSAGE_DATA)
    {
        nRequest = (short)(((nFlag & 1) > 0) + 7);
    }
    else
    {
        nRequest = (short)pArgs->m_pArgs[1].n;
    }

    if (nRequest == ARENA_TRACK_UP)
    {
        nMode += 2;

        if (nMode <= 4)
        {
            pSlots[SLOT_SOUND_MAN]->PostSceneEvent(MSG_DOUBLE_SCORE, 0);
        }

        if (nMode >= 4)
        {
            nMode = 4;
        }
    }
    else if (nRequest == ARENA_TRACK_DOWN)
    {
        nMode -= 2;

        if (nMode >= -4)
        {
            pSlots[SLOT_SOUND_MAN]->PostSceneEvent(MSG_DOUBLE_SCORE, 1);
        }

        if (nMode <= -4)
        {
            nMode = -4;
        }
    }
    else if (nRequest == 0)
    {
        nMode = 0;
    }

    GameEngine::ShowRokValue(nMode);

    pRok->m_wRokValue = nMode;

    pRok->SetRenderSlotFlags();
}

// FUNCTION: HYPERX 0x004633c0
void __cdecl PlanAction::SetRokKillState(Interactor*, PlanNode* pArgs, short*, int nArg)
{
    const short nDefault = (short)nArg;

    short            nValue = (short)pArgs->m_pArgs[1].n;
    RokObject* const pRok   = g_pGameEngine->GetRok();

    if (nValue == PLAN_TOKEN_MESSAGE_DATA)
    {
        nValue = nDefault;
    }

    if (nValue == -1)
    {
        pRok->m_nKillState = -1;

        pRok->SetRenderSlotFlags();

        g_pGameEngine->m_ppSlots[SLOT_SOUND_MAN]->PostSceneEvent(MSG_KILL_BALL_CHANGE, 0);

        return;
    }

    pRok->m_nKillState = (unsigned char)~(unsigned char)nValue & 1;

    pRok->SetRenderSlotFlags();

    g_pGameEngine->m_ppSlots[SLOT_SOUND_MAN]->PostSceneEvent(MSG_KILL_BALL_CHANGE, 1);

    pRok->SetEntitySlot(pRok->m_nCarrierSlot);
}

// Any kind but the randomizers indexes by the low word of the argument block's own address.

// The frame the first saved row is pinned to.
#define COLOR_PINNED_FRAME 0x78

// One word a colour row, set to one so the first save wins; marked in src/globals.cpp.
extern short g_awColorLatched[2];

// Two rows of twenty saved colours. src/globals.cpp carries the marker.
extern int g_aaColorSaved[2][20];

// FUNCTION: HYPERX 0x00463450
void __cdecl PlanAction::GrayscaleRandomizer(Interactor* pContext, PlanNode* pArgs, short*, int)
{
    EngineObject* pObject;
    int           nValue;
    PaletteRange  aRange;

    const short nEnable = (short)pArgs->m_pArgs[1].n;

    if (pContext->GetNamedSlot() == -1)
    {
        BoundBubble* const pCol = g_pGameEngine->GetColObjPtr(pContext->GetOtherSlot());

        pObject = pCol->m_pOwner;
    }
    else
    {
        pObject = pContext->GetActor();
    }

    // The original leaves the index unset for any other kind.
    if (pObject->m_nKind == KIND_GREEN_RANDOMIZER)
    {
        nValue = 1;
    }

    if (pObject->m_nKind == KIND_RED_RANDOMIZER)
    {
        nValue = 0;
    }

    const short nIndex = (short)nValue;

    if (g_awColorLatched[nIndex] != 0)
    {
        for (int nMat = 0; nMat < pObject->m_pModelBlock->m_nMatCount; nMat++)
        {
            g_aaColorSaved[nIndex][nMat] = pObject->m_pModelBlock->m_pMaterials[nMat].nColour;

            if (nIndex == 0)
            {
                g_aaColorSaved[0][1] = COLOR_PINNED_FRAME;
            }

            g_awColorLatched[nIndex] = 0;
        }
    }

    if (nEnable != 0)
    {
        aRange = g_pColorPalette->GetFrameRange(1);

        for (int nMat = 0; nMat < pObject->m_pModelBlock->m_nMatCount; nMat++)
        {
            pObject->m_pModelBlock->m_pMaterials[nMat].nColour = aRange.nFirst;
        }

        return;
    }

    for (int nMat = 0; nMat < pObject->m_pModelBlock->m_nMatCount; nMat++)
    {
        pObject->m_pModelBlock->m_pMaterials[nMat].nColour = g_aaColorSaved[nIndex][nMat];
    }
}

// Argument one guards the body and two to four are the position.
#define ARG_GUARD               1
#define ARG_X                   2
#define ARG_Y                   3
#define ARG_Z                   4
#define ARG_COUNT_WITH_POSITION 5

// FUNCTION: HYPERX 0x004635e0
void PlanAction::SetLocalReal(Interactor* pContext, PlanNode* pArgs, short*, int)
{
    const PlanWord* pBlock  = pArgs->m_pArgs;
    const float     flValue = pBlock[2].fl;
    const short     nIndex  = (short)pBlock[1].n;

    EngineObject* const pActor  = pContext->GetActor();
    PlanState* const    pLocals = pActor->GetPlanState();

    pLocals->m_pflLocalReals[nIndex] = flValue;
}

// The CTrackTraj belongs to the plan's object; the position goes to the camera in slot nine.

// FUNCTION: HYPERX 0x00463620
void PlanAction::SetInDrag(Interactor* pContext, PlanNode* pNode, short*, int)
{
    if (pNode->m_pArgs[ARG_GUARD].n != 0)
    {
        EngineObject* const pActor = pContext->GetActor();

        PlanState* const pPlanState = pActor->GetPlanState();

        static_cast<CTrackTraj*>(pPlanState->m_pCurrent->m_apSlots[0])->InitComplexRange();

        if (pNode->m_nArgCount == ARG_COUNT_WITH_POSITION)
        {
            EngineObject* pCamera = g_pGameEngine->m_ppSlots[CAMERA_MAIN_SLOT];

            Vec3* const pPos = pCamera->GetPositionPtr();

            pCamera = g_pGameEngine->m_ppSlots[CAMERA_MAIN_SLOT];

            Vec3* const pBody = &pCamera->GetPhysicsBody()->pos;

            pPos->x = pBody->x = (float)(unsigned int)pNode->m_pArgs[ARG_X].n;

            pPos->y = pBody->y = (float)(unsigned int)pNode->m_pArgs[ARG_Y].n;

            pPos->z = pBody->z = (float)(unsigned int)pNode->m_pArgs[ARG_Z].n;
        }
    }
}

// Argument one is a mode that doubles as the range's low end; two is its high end.
#define ARG_RIM_MODE         1
#define ARG_HIGH             2
#define ARG_COUNT_WITH_RANGE 3

// With both arguments a mode of nought picks the directed range and any other the complex one.
// FUNCTION: HYPERX 0x004636e0
void PlanAction::SetOnRim(Interactor*, PlanNode* pNode, short*, int)
{
    EngineObject* pActor;
    PlanState*    pPlanState;

    if (pNode->m_nArgCount == ARG_COUNT_WITH_RANGE)
    {
        if (pNode->m_pArgs[ARG_RIM_MODE].n == 0)
        {
            pActor = g_pGameEngine->m_ppSlots[CAMERA_MAIN_SLOT];

            pPlanState = pActor->GetPlanState();

            static_cast<CTrackTraj*>(pPlanState->m_pCurrent->m_apSlots[0])->SetDirectedRange((float)(unsigned int)pNode->m_pArgs[ARG_RIM_MODE].n, (float)(unsigned int)pNode->m_pArgs[ARG_HIGH].n);

            return;
        }

        pActor = g_pGameEngine->m_ppSlots[CAMERA_MAIN_SLOT];

        pPlanState = pActor->GetPlanState();

        static_cast<CTrackTraj*>(pPlanState->m_pCurrent->m_apSlots[0])->InitComplexRange();

        return;
    }

    pActor = g_pGameEngine->m_ppSlots[CAMERA_MAIN_SLOT];

    pPlanState = pActor->GetPlanState();

    static_cast<CTrackTraj*>(pPlanState->m_pCurrent->m_apSlots[0])->InitRange();
}

#define ARG_VALUE 1

#define STEER_SCALE_THRESHOLD 0x19

// The camera's plan sets the blend fraction; any other object takes a hundredth as its friction.
// FUNCTION: HYPERX 0x00463790
void PlanAction::SetFriction(Interactor* pContext, PlanNode* pNode, short*, int)
{
    EngineObject* pActor = pContext->GetActor();

    if (pActor->GetSlot() != CAMERA_MAIN_SLOT)
    {
        pActor = pContext->GetActor();

        const float flParam = (float)(unsigned int)pNode->m_pArgs[ARG_VALUE].n * 0.01f;

        pActor->GetPhysicsBody()->flFriction = flParam;

        if ((unsigned int)pNode->m_pArgs[ARG_VALUE].n > STEER_SCALE_THRESHOLD)
        {
            pActor = pContext->GetActor();

            pActor->GetPhysicsBody()->flSteerScale = g_flOnePlan;
        }
    }
    else
    {
        PlanState* const pPlanState = g_pGameEngine->m_ppSlots[CAMERA_MAIN_SLOT]->GetPlanState();

        static_cast<CTrackTraj*>(pPlanState->m_pCurrent->m_apSlots[0])
            ->SetBlendT(pNode->m_pArgs[ARG_VALUE].n);
    }
}

#define ARG_COUNT_WITHOUT_SLOT 3

// The argument block's own fields: which rating, how much, and which slot.
#undef ARG_STAT
#define ARG_STAT   1
#define ARG_AMOUNT 2
#undef ARG_SLOT
#define ARG_SLOT 3

// The top of the range every amount is clamped to; the bottom is nought.
#define STAT_CEILING 10.0f

// FUNCTION: HYPERX 0x00463840
void PlanAction::SetMyGravity(Interactor* pContext, PlanNode* pArgs, short*, int)
{
    EngineObject* const pActor = pContext->GetActor();

    const float flValue = (float)(unsigned int)pArgs->m_pArgs[1].n;

    pActor->GetPhysicsBody()->flGravAccel = flValue;
}

// FUNCTION: HYPERX 0x00463880
void __cdecl PlanAction::ModifyAvailableReds(Interactor*, PlanNode* pArgs, short*, int)
{
    int nReds;

    const int nJudge = (int)g_pGameEngine->m_ppSlots[SLOT_RED_RANDOMIZER]->GetPhysicsBody()->axis.x;

    nReds = max(JUDGE_STATE_LOW,
                min(JUDGE_STATE_HIGH, (int)(short)nJudge + (int)(short)pArgs->m_pArgs[1].n));

    g_pGameEngine->m_ppSlots[SLOT_RED_RANDOMIZER]->GetPhysicsBody()->axis.x = (float)nReds;
}

// The two enable ratings are flags; every other rating is added and clamped to nought and ten.

// FUNCTION: HYPERX 0x00463900
void __cdecl PlanAction::ModifyRating(Interactor* pContext, PlanNode* pArgs, short*, int nArg)
{
    const short nSlot = (short)nArg;

    PlayerObject* pObject;
    int           nIndex;

    const int nStat = pArgs->m_pArgs[ARG_STAT].n;

    if (pContext->GetNamedSlot() == -1)
    {
        if (pArgs->m_nArgCount == ARG_COUNT_WITHOUT_SLOT)
        {
            BoundBubble* const pColObj = g_pGameEngine->GetColObjPtr(pContext->GetModelSlot());

            pObject = (PlayerObject*)pColObj->m_pOwner;
        }
        else
        {
            nIndex = (short)pArgs->m_pArgs[ARG_SLOT].n;

            pObject = g_pGameEngine->GetPlayer(nIndex);
        }
    }
    else
    {
        nIndex = nSlot;

        if (pArgs->m_pArgs[ARG_SLOT].n != PLAN_TOKEN_MESSAGE_DATA)
        {
            nIndex = (short)pArgs->m_pArgs[ARG_SLOT].n;
        }

        pObject = g_pGameEngine->GetPlayer(nIndex);
    }

    const short nAmount = (short)pArgs->m_pArgs[ARG_AMOUNT].n;

    const float flScalar = pObject->m_stats.GetScalar(nStat);

    switch (nStat)
    {
        case RATING_LONG_SHOT_ENABLE:
        case RATING_LONG_PASS_ENABLE:
            if (nAmount <= 0)
            {
                pObject->m_stats.m_aStats[RATING_SLOT(nStat)] = 0.0f;

                return;
            }

            pObject->m_stats.m_aStats[RATING_SLOT(nStat)] = 1.0f;

            return;
    }

    float flValue = (float)nAmount + flScalar;

    if (STAT_CEILING >= flValue && 0.0f > flValue)
    {
        flValue = 0.0f;
    }
    else if (STAT_CEILING <= flValue)
    {
        flValue = STAT_CEILING;
    }

    pObject->m_stats.m_aStats[RATING_SLOT(nStat)] = flValue;
}

// Nothing moves unless the judge or the named object is the actor itself.
// FUNCTION: HYPERX 0x00463a30
void __cdecl PlanAction::SwapRandomizers(Interactor* pContext, PlanNode* pNode, short*, int nPassed)
{
    const short nDefaultSlot = (short)nPassed;

    EngineObject* pActor;
    EngineObject* pNamed;
    short         nSlot;

    Vec3 vRandomizerAt = *g_pGameEngine->m_ppSlots[SLOT_RED_RANDOMIZER]->GetPositionPtr();

    EngineObject* const pRandomizer = g_pGameEngine->m_ppSlots[SLOT_RED_RANDOMIZER];

    if (pContext->GetNamedSlot() == -1)
    {
        nSlot = pContext->GetOtherSlot();

        BoundBubble* const pCollision = g_pGameEngine->GetColObjPtr(nSlot);

        pActor = pCollision->GetOwner();

        PlanWord* const pBlock = pNode->m_pArgs;

        if (pBlock[1].n == PLAN_TOKEN_MESSAGE_DATA)
        {
            pNamed = g_pGameEngine->m_ppSlots[nDefaultSlot];
        }
        else
        {
            if (pBlock[1].IsPronoun())
            {
                nSlot = g_apPlanStatFns[pBlock[1].wPronoun](pContext->GetActor(), pNode);
            }
            else
            {
                nSlot = (short)pBlock[1].n;
            }

            pNamed = g_pGameEngine->m_ppSlots[nSlot];
        }
    }
    else
    {
        pActor = pContext->GetActor();

        PlanWord* const pBlock = pNode->m_pArgs;

        if (pBlock[1].n == PLAN_TOKEN_MESSAGE_DATA)
        {
            pNamed = g_pGameEngine->m_ppSlots[nDefaultSlot];
        }
        else
        {
            if (pBlock[1].IsPronoun())
            {
                nSlot = g_apPlanStatFns[pBlock[1].wPronoun](pContext->GetActor(), pNode);
            }
            else
            {
                nSlot = (short)pBlock[1].n;
            }

            pNamed = g_pGameEngine->m_ppSlots[nSlot];
        }
    }

    if (pRandomizer == 0 || pNamed == 0)
    {
        return;
    }

    if (pRandomizer != pActor && pNamed != pActor)
    {
        return;
    }

    const Vec3* pAt = pActor->GetPositionPtr();

    pRandomizer->m_pBody->pos = *pAt;

    Vec3* pFrom = pActor->GetPositionPtr();

    pRandomizer->GetPhysicsBody()->SetFlatOnTerrain(pFrom);

    pRandomizer->m_nFlags &= ~ENGINEOBJECT_HIDDEN;

    pAt = pNamed->GetPositionPtr();

    pActor->m_pBody->pos = *pAt;

    pFrom = pNamed->GetPositionPtr();

    pActor->GetPhysicsBody()->SetFlatOnTerrain(pFrom);

    pActor->m_nFlags &= ~ENGINEOBJECT_HIDDEN;

    pNamed->m_pBody->pos = vRandomizerAt;

    pNamed->m_nFlags |= ENGINEOBJECT_HIDDEN;
}

#define ARG_TRACKED 2

#define ARG_COUNT_WITH_TRACKED 3

// A positive damage rating clears the killer slot to minus one.
// FUNCTION: HYPERX 0x00463c20
void __cdecl PlanAction::SetKilledDromer(Interactor* pContext, PlanNode* pArgs, short*, int nMessageData)
{
    int nObject;
    int nTracked;

    PlanWord* pBlock = pArgs->m_pArgs;

    if (pBlock[ARG_OBJECT].IsPronoun())
    {
        nObject = g_apPlanStatFns[pBlock[ARG_OBJECT].wPronoun](pContext->GetActor(),
                                                               pArgs);
    }
    else
    {
        nObject = pBlock[ARG_OBJECT].n;
    }

    if (nObject == PLAN_TOKEN_MESSAGE_DATA)
    {
        nObject = nMessageData;
    }

    if (pArgs->m_nArgCount == ARG_COUNT_WITH_TRACKED)
    {
        pBlock = pArgs->m_pArgs;

        if (pBlock[ARG_TRACKED].IsPronoun())
        {
            nTracked = g_apPlanStatFns[pBlock[ARG_TRACKED].wPronoun](
                pContext->GetActor(), pArgs);
        }
        else
        {
            nTracked = pBlock[ARG_TRACKED].n;
        }

        if (nTracked == PLAN_TOKEN_MESSAGE_DATA)
        {
            nTracked = nMessageData;
        }
    }
    else
    {
        nTracked = pContext->GetActor()->GetSlot();
    }

    PlayerObject* const pObject = g_pGameEngine->GetPlayer((short)nObject);

    if (pObject->m_stats.GetScalar(RATING_DAMAGE) <= 0.0f)
    {
        pObject->m_wKilledBySlot = (short)nTracked;

        return;
    }

    pObject->m_wKilledBySlot = -1;
}

#define ARG_COUNT_WRITE 2

// What a read answers when m_wDesiredSubstitute is minus one, no substitute: one past the skaters.
#define SUBSTITUTE_NONE_ANSWER 4

// FUNCTION: HYPERX 0x00463d20
void PlanAction::ResetCameraTrack(Interactor* pContext, PlanNode*, short*, int)
{
    CameraObject* const pCamera = pContext->GetCameraActor();

    pCamera->SetLastTrackedObject(0);
}

// FUNCTION: HYPERX 0x00463d40
void PlanAction::ClearBehaviorAnimation(Interactor* pContext, PlanNode*, short*, int)
{
    EngineObject* pActor = pContext->GetActor();

    pActor->GetPlanState()->m_pCurrent->m_pAnimation = 0;

    pActor = pContext->GetActor();

    pActor->GetPlanState()->m_pCurrent->m_pPersistentAnimation = 0;
}

// FUNCTION: HYPERX 0x00463d80
void PlanAction::FlyingSemaphore(Interactor* pContext, PlanNode* pArgs, short* pResult, int)
{
    PlayerObject* const pActor = pContext->GetPlayerActor();

    if (pArgs->m_nArgCount == 2)
    {
        pActor->m_wFlyingSemaphore = (short)pArgs->m_pArgs[1].n;
    }
    else
    {
        *pResult = pActor->m_wFlyingSemaphore != 0;
    }
}

// FUNCTION: HYPERX 0x00463dc0
void PlanAction::AnimationSemaphore(Interactor* pContext, PlanNode* pArgs, short* pResult, int)
{
    PlayerObject* const pActor = pContext->GetPlayerActor();

    if (pArgs->m_nArgCount == 2)
    {
        pActor->m_wAnimationSemaphore = (short)pArgs->m_pArgs[1].n;
    }
    else
    {
        *pResult = pActor->m_wAnimationSemaphore != 0;
    }
}

// FUNCTION: HYPERX 0x00463e00
void PlanAction::ClockStoppedSemaphore(Interactor* pContext, PlanNode* pArgs, short* pResult, int)
{
    PlayerObject* const pActor = pContext->GetPlayerActor();

    if (pArgs->m_nArgCount == 2)
    {
        pActor->m_wClockStoppedSemaphore = (short)pArgs->m_pArgs[1].n;
    }
    else
    {
        *pResult = pActor->m_wClockStoppedSemaphore != 0;
    }
}

// A count of two writes and any other reads.

// FUNCTION: HYPERX 0x00463e40
void __cdecl PlanAction::DesiredSubstitute(Interactor* pContext, PlanNode* pArgs, short* pResult, int nPassed)
{
    const unsigned int nSlot = (unsigned int)nPassed;

    unsigned int nArg;

    PlayerObject* const pActor = pContext->GetPlayerActor();

    const PlanWord* pBlock = pArgs->m_pArgs;

    if (pBlock[1].IsPronoun())
    {
        nArg = g_apPlanStatFns[pBlock[1].wPronoun](pContext->GetActor(), pArgs);
    }
    else
    {
        nArg = pBlock[1].n;
    }

    if (nArg == PLAN_TOKEN_MESSAGE_DATA)
    {
        nArg = nSlot;
    }

    if (pArgs->m_nArgCount == ARG_COUNT_WRITE)
    {
        pActor->m_wDesiredSubstitute = (short)nArg;

        return;
    }

    const short nValue = pActor->m_wDesiredSubstitute;

    *pResult = nValue;

    if (nValue == -1)
    {
        *pResult = SUBSTITUTE_NONE_ANSWER;
    }
}

// FUNCTION: HYPERX 0x00463ed0
void PlanCond::SubstitutionSemaphore(Interactor* pContext, PlanNode*, short* pResult, int)
{
    PlayerObject* const pActor = pContext->GetPlayerActor();

    *pResult = pActor->m_wDesiredSubstitute != -1;
}

// Of the first and last fit substitutes the healthier wins, with the near half favoured.

// Answered when no substitute is fit.
#define SLOT_ANSWER_NONE 4

// FUNCTION: HYPERX 0x00463ef0
void __cdecl PlanCond::PickValidSubstitute(Interactor* pContext, PlanNode*, short* pResult, int)
{
    short            sIndex;
    int              nWhich;
    const TeamBlock* pTeam;
    float            flBoost;

    pContext->GetActor();

    const int nSlot = pContext->GetActor()->GetSlot();

    short sCandidate  = SLOT_NONE;
    short sChallenger = SLOT_NONE;

    for (sIndex = 0, nWhich = 0; sIndex < PLAYER_SLOT_COUNT; sIndex++, nWhich++)
    {
        if (g_pGameEngine->IsFitSubstitute(nSlot, nWhich) != 0)
        {
            if (sCandidate == SLOT_NONE)
            {
                sCandidate = sIndex;
            }
            else
            {
                sChallenger = sIndex;
            }
        }
    }

    if (sCandidate == SLOT_NONE)
    {
        *pResult = SLOT_ANSWER_NONE;
        return;
    }

    if (sChallenger == SLOT_NONE)
    {
        *pResult = sCandidate;
        return;
    }

    if (nSlot % 2 != 0)
    {
        pTeam = g_pGameEngine->m_pTeamB;
    }
    else
    {
        pTeam = g_pGameEngine->m_pTeamA;
    }

    const int nFirst  = sCandidate;
    const int nSecond = sChallenger;

    float flFirst  = pTeam->aRows[nFirst].aStat[RATING_DAMAGE];
    float flSecond = pTeam->aRows[nSecond].aStat[RATING_DAMAGE];

    if (pTeam->aReserveSince[nFirst] != 0)
    {
        const float flGate = DamageGate();

        flFirst = (float)(g_pGameEngine->m_clock.nTickMs - pTeam->aReserveSince[nFirst]) / (13500.0f / flGate) + flFirst;
    }

    if (pTeam->aReserveSince[nSecond] != 0)
    {
        flSecond = (float)(g_pGameEngine->m_clock.nTickMs - pTeam->aReserveSince[nSecond]) / (13500.0f / DamageGate()) + flSecond;
    }

    if (sCandidate < SLOT_FAR_HALF)
    {
        if (sChallenger >= SLOT_FAR_HALF && flFirst < 30.0f)
        {
            *pResult = sChallenger;
            return;
        }

        flBoost = flFirst;
        flFirst = flBoost * 0.2f + flFirst;
    }

    if (sChallenger < SLOT_FAR_HALF)
    {
        flBoost  = flSecond;
        flSecond = flBoost * 0.2f + flSecond;
    }

    if (flFirst >= flSecond)
    {
        *pResult = sCandidate;
        return;
    }

    *pResult = sChallenger;
}

#define CREDIT_OTHER()                                                                                 \
    if (nTeam % 2 == nOther % 2)                                                                       \
    {                                                                                                  \
        return;                                                                                        \
    }                                                                                                  \
                                                                                                       \
    g_pGameEngine->GetPlayer((short)nOther)->m_matchStats.AccumulateStat(MATCH_STAT_INJURIES, nOther); \
    return;

#define ACCUMULATE_HIT(nLowStat)                                                       \
    if (nSlot == SLOT_MINE)                                                            \
    {                                                                                  \
        nOther = g_pGameEngine->m_ppSlots[SLOT_MINE]->m_nEntitySlot;                   \
        CREDIT_OTHER()                                                                 \
    }                                                                                  \
    else if (nSlot == SLOT_ROK)                                                        \
    {                                                                                  \
        nOther = g_pGameEngine->m_ppSlots[SLOT_ROK]->m_nEntitySlot;                    \
        CREDIT_OTHER()                                                                 \
    }                                                                                  \
    else if (nSlot >= SLOT_TURNSTILE && nSlot <= SLOT_TURNSTILE_LAST)                  \
    {                                                                                  \
        nOther = g_pGameEngine->m_ppSlots[nSlot]->m_nEntitySlot;                       \
        CREDIT_OTHER()                                                                 \
    }                                                                                  \
    else                                                                               \
    {                                                                                  \
        if (nSlot >= SLOT_LASER_HURDLE && nSlot <= SLOT_LASER_HURDLE_LAST)             \
        {                                                                              \
            return;                                                                    \
        }                                                                              \
                                                                                       \
        if (nSlot < 0)                                                                 \
        {                                                                              \
            return;                                                                    \
        }                                                                              \
                                                                                       \
        if (nSlot > SLOT_DROMER3)                                                      \
        {                                                                              \
            return;                                                                    \
        }                                                                              \
                                                                                       \
        if (nTeam % 2 == nSlot % 2)                                                    \
        {                                                                              \
            return;                                                                    \
        }                                                                              \
                                                                                       \
        g_pGameEngine->GetPlayer(nSlot)->m_matchStats.AccumulateStat(nLowStat, nSlot); \
        return;                                                                        \
    }

// FUNCTION: HYPERX 0x00464140
void PlanAction::SubstitutePlayer(Interactor* pContext, PlanNode* pArgs, short* pResult, int nParam)
{
    PlayerObject* const pPlayer = pContext->GetPlayerActor();
    EngineObject* const pActor  = pContext->GetActor();

    const int nSlot = pActor->GetSlot();

    if (pArgs->m_nArgCount == 2)
    {
        const short nSub = pPlayer->m_wDesiredSubstitute;

        if (nSub == -1)
        {
            return;
        }

        g_pGameEngine->SubstitutePlayer(nSlot, nSub);

        return;
    }

    *pResult = (short)(g_pGameEngine->IsFitSubstitute(nSlot, nParam) != 0);
}

// Whether the team mate may substitute in; the byte answer is normalised to one or zero.

// FUNCTION: HYPERX 0x004641b0
void __cdecl PlanCond::OkToSubstituteTeammate(Interactor* pContext, PlanNode*, short* pResult, int nParam)
{
    pContext->GetActor();

    EngineObject* const pActor = pContext->GetActor();

    const unsigned int nSlot = (unsigned int)pActor->GetSlot();

    *pResult = (unsigned short)(g_pGameEngine->IsFitSubstitute(TeammateSlot((short)nSlot), nParam) != 0);
}

// FUNCTION: HYPERX 0x00464200
void PlanAction::TrackTurnstile(Interactor* pContext, PlanNode* pArgs, short*, int nParam)
{
    int nValue = pArgs->m_pArgs[1].n;

    if (nValue == PLAN_TOKEN_MESSAGE_DATA)
    {
        nValue = nParam;
    }

    EngineObject* const pActor = pContext->GetActor();

    pActor->SetEntitySlot(nValue);
}

// Nothing is credited when hitter and target are on the same side, the two parities.

// FUNCTION: HYPERX 0x00464230
void __cdecl PlanAction::CreditDamage(Interactor* pContext, PlanNode* pArgs, short*, int)
{
    int nOther;

    PlayerObject* const pObject = g_pGameEngine->GetPlayer((short)pArgs->m_pArgs[1].n);

    const short nTeam = pObject->GetSlot();

    const float flDamage = pObject->m_stats.GetScalar(RATING_DAMAGE);

    const short nSlot = pContext->GetTargetSlot();

    if (flDamage <= DamageGate())
    {
        if (flDamage > 0.0f)
        {
            ACCUMULATE_HIT(MATCH_STAT_INJURIES)
        }
    }
    else if (flDamage > 0.0f)
    {
        return;
    }

    ACCUMULATE_HIT(MATCH_STAT_FATALITIES)
}

#define ARG_COUNT_COL_SLOT 2

#define DAMAGE_CEILING 100.0f

// The scale a number of nought or less takes, one rung per difficulty; a network bot takes a half.
#define SCALE_EASY        0.5f
#define SCALE_MEDIUM      0.75f
#define SCALE_HARD        1.0f
#define SCALE_NETWORK_BOT 0.5f

// FUNCTION: HYPERX 0x004644a0
void PlanAction::SetMaxSpeed(Interactor* pContext, PlanNode* pArgs, short*, int)
{
    pContext->GetActor();

    const float flSpeed = pArgs->m_pArgs[1].fl;

    g_flMaxSpeed = flSpeed;
}

// Nought or less is scaled by difficulty; the damage is added and clamped to nought to a hundred.

// FUNCTION: HYPERX 0x004644c0
void __cdecl PlanAction::ModifyDamage(Interactor* pContext, PlanNode* pArgs, short*, int nParam)
{
    PlayerObject*   pObject;
    const PlanWord* pRecord;
    BoundBubble*    pCollision;
    int             nWhich;
    int             nValue;
    int             nKillerSlot;
    float           flScale;
    float           flClamped;

    if (pContext->GetNamedSlot() == SLOT_NONE)
    {
        if (pArgs->m_nArgCount == ARG_COUNT_COL_SLOT)
        {
            pCollision = g_pGameEngine->GetColObjPtr(pContext->GetModelSlot());

            pObject = (PlayerObject*)pCollision->GetOwner();
            nValue  = pArgs->m_pArgs[1].n;
        }
        else
        {
            pRecord = pArgs->m_pArgs;
            pObject = g_pGameEngine->GetPlayer((short)pRecord[1].n);
            nValue  = pRecord[2].n;
        }
    }
    else
    {
        pRecord = pArgs->m_pArgs;

        if (pRecord[1].n == PLAN_TOKEN_MESSAGE_DATA)
        {
            nWhich = (short)nParam;
        }
        else
        {
            nWhich = (short)pRecord[1].n;
        }

        nValue = pRecord[2].n;

        pObject = g_pGameEngine->GetPlayer(nWhich);
    }

    pObject->GetSlot();

    ObjectStats* const pStats = &pObject->m_stats;

    const float flDamage = pStats->GetScalar(RATING_DAMAGE);

    if (nValue == PLAN_TOKEN_MESSAGE_DATA)
    {
        nKillerSlot = pContext->GetTargetSlot();
        nValue      = nParam;
    }
    else
    {
        nKillerSlot = pContext->GetActor()->GetSlot();
    }

    if ((flDamage > DamageGate() || (float)nValue > g_flZeroPlan) && pObject->m_wClockStoppedSemaphore == 0)
    {
        if (g_pGameEngine->GetDifficultyLevel() == DIFFICULTY_EASY)
        {
            flScale = SCALE_EASY;
        }
        else if (g_pGameEngine->GetDifficultyLevel() == DIFFICULTY_MEDIUM)
        {
            flScale = SCALE_MEDIUM;
        }
        else if (g_pGameEngine->GetDifficultyLevel() == DIFFICULTY_HARD)
        {
            flScale = SCALE_HARD;
        }
        else
        {
            flScale = SCALE_HARD;
        }

        if (g_bSessionActive != 0)
        {
            char* const pszName = pObject->GetPlanState()->m_pszName;

            // STRING: HYPERX 0x004bb340
            if (_mbscmp((const unsigned char*)"Human", (const unsigned char*)pszName) != 0)
            {
                flScale = SCALE_NETWORK_BOT;
            }
        }

        if (nValue <= 0)
        {
            nValue = (int)((float)nValue * flScale);
        }

        const float flTotal = (float)nValue + flDamage;

        if (DAMAGE_CEILING <= flTotal || 0.0f <= flTotal)
        {
            if (DAMAGE_CEILING < flTotal)
            {
                flClamped = DAMAGE_CEILING;
            }
            else
            {
                flClamped = flTotal;
            }
        }
        else
        {
            flClamped = 0.0f;
        }

        pStats->m_aStats[RATING_SLOT(RATING_DAMAGE)] = flClamped;

        if (flTotal <= 0.0f)
        {
            pObject->m_wKilledBySlot = (short)nKillerSlot;
        }
        else
        {
            pObject->m_wKilledBySlot = SLOT_NONE;
        }
    }
}

#define ARG_GATE       1
#define ARG_HUNDREDTHS 2

#define SPEED_CACHE_COUNT 4

// The clamp, as two doubles.
#define SPEED_RATING_MIN 0.99
#define SPEED_RATING_MAX 10.99

// A hundredth, the scale the raw word is in, and the offset added to the scaled reading.
#define HUNDREDTHS 0.01f
#define SPEED_BIAS 0.00999f

// Minus one as its bits; the cache entry is tested as a dword.
#define SPEED_UNSET_BITS 0xbf800000

// The cache of the four skaters' speeds and its dirty word, which ships set.
// GLOBAL: HYPERX 0x004c4540
extern float g_aOpposSpeedCache[SPEED_CACHE_COUNT];
// GLOBAL: HYPERX 0x004c4550
extern short g_nOpposSpeedCacheStale;

// FUNCTION: HYPERX 0x00464720
void PlanAction::IncrementScore(Interactor*, PlanNode* pArgs, short*, int)
{
    if (pArgs->m_pArgs[1].n != 0)
    {
        InfoBar::IncrementScore(SCORE_LINE_TEAM1);
    }
    else
    {
        InfoBar::IncrementScore(SCORE_LINE_TEAM2);
    }
}

// The twin of ModifySpeedRating, writing a pair: the complement's low bit and that slot plus two.

// FUNCTION: HYPERX 0x00464750
void __cdecl PlanAction::ModifyOpposTeamFriction(Interactor* pContext, PlanNode* pNode, short*, int nArg)
{
    const short nSlot = (short)nArg;

    PlanWord*     pArgs;
    PlayerObject* pObject;
    short         nGate;
    short         nTarget;
    int           nHundredths;
    float         flSpeed;
    double        dReading;

    if (pContext->GetNamedSlot() == -1)
    {
        pArgs       = pNode->m_pArgs;
        nGate       = (short)pArgs[ARG_GATE].n;
        nHundredths = (short)pArgs[ARG_HUNDREDTHS].n;

        flSpeed = (float)nHundredths * HUNDREDTHS + SPEED_BIAS;

        if (pNode->m_nArgCount == ARG_COUNT_WITHOUT_SLOT)
        {
            BoundBubble* const pColObj = g_pGameEngine->GetColObjPtr(pContext->GetModelSlot());

            nTarget = pColObj->GetOwner()->GetSlot();
        }
        else
        {
            nTarget = (short)pArgs[ARG_SLOT].n;
        }
    }
    else
    {
        pArgs       = pNode->m_pArgs;
        nGate       = (short)pArgs[ARG_GATE].n;
        nHundredths = (short)pArgs[ARG_HUNDREDTHS].n;

        flSpeed = (float)nHundredths * HUNDREDTHS + SPEED_BIAS;

        if ((unsigned int)pArgs[ARG_SLOT].n == PLAN_TOKEN_MESSAGE_DATA)
        {
            nTarget = nSlot;
        }
        else
        {
            nTarget = (short)pArgs[ARG_SLOT].n;
        }
    }

    if (g_nOpposSpeedCacheStale != 0)
    {
        for (int i = 0; i < SPEED_CACHE_COUNT; i++)
        {
            pObject = g_pGameEngine->GetPlayer((short)i);

            g_aOpposSpeedCache[i] = pObject->m_stats.GetScalar(RATING_SPEED);
        }

        g_nOpposSpeedCacheStale = 0;
    }

    const float flBoosted = flSpeed + 1.0f;

    dReading = (float)(short)(long)(flBoosted * g_aOpposSpeedCache[nTarget] + 0.5f) + 0.99f;

    if (SPEED_RATING_MAX <= dReading || dReading >= SPEED_RATING_MIN)
    {
        if (dReading > SPEED_RATING_MAX)
        {
            dReading = SPEED_RATING_MAX;
        }
    }
    else
    {
        dReading = SPEED_RATING_MIN;
    }

    int nFlip = ((unsigned char)~nTarget) & 1;

    if (nGate != 0)
    {
        if (FloatBits(g_aOpposSpeedCache[(short)nFlip]) != SPEED_UNSET_BITS)
        {
            pObject = g_pGameEngine->GetPlayer((short)nFlip);

            pObject->m_stats.SetStat(RATING_SLOT(RATING_SPEED), g_aOpposSpeedCache[nTarget]);

            nFlip += 2;

            pObject = g_pGameEngine->GetPlayer((short)nFlip);

            pObject->m_stats.SetStat(RATING_SLOT(RATING_SPEED), g_aOpposSpeedCache[nTarget]);
        }

        g_nOpposSpeedCacheStale = 1;
    }
    else
    {
        pObject = g_pGameEngine->GetPlayer((short)nFlip);

        pObject->m_stats.m_aStats[RATING_SLOT(RATING_SPEED)] = (float)dReading;

        nFlip += 2;

        pObject = g_pGameEngine->GetPlayer((short)nFlip);

        pObject->m_stats.m_aStats[RATING_SLOT(RATING_SPEED)] = (float)dReading;
    }
}

// The cache of the four skaters' speeds and its dirty word, which ships set.
// GLOBAL: HYPERX 0x004c4554
extern float g_aSpeedCache[SPEED_CACHE_COUNT];
// GLOBAL: HYPERX 0x004c4564
extern short g_nSpeedCacheStale;

// The reading is (speed + 1) times the cached speed, rounded to a short, plus 0.99.
// FUNCTION: HYPERX 0x00464940
void __cdecl PlanAction::ModifySpeedRating(Interactor* pContext, PlanNode* pNode, short*, int nArg)
{
    const short nSlot = (short)nArg;

    PlanWord*     pArgs;
    PlayerObject* pObject;
    short         nGate;
    short         nTarget;
    int           nHundredths;
    float         flSpeed;
    double        dReading;

    if (pContext->GetNamedSlot() == -1)
    {
        pArgs       = pNode->m_pArgs;
        nGate       = (short)pArgs[ARG_GATE].n;
        nHundredths = (short)pArgs[ARG_HUNDREDTHS].n;

        flSpeed = (float)nHundredths * HUNDREDTHS;

        if (pNode->m_nArgCount == ARG_COUNT_WITHOUT_SLOT)
        {
            BoundBubble* const pColObj = g_pGameEngine->GetColObjPtr(pContext->GetModelSlot());

            nTarget = pColObj->GetOwner()->GetSlot();
        }
        else
        {
            nTarget = (short)pArgs[ARG_SLOT].n;
        }
    }
    else
    {
        pArgs       = pNode->m_pArgs;
        nGate       = (short)pArgs[ARG_GATE].n;
        nHundredths = (short)pArgs[ARG_HUNDREDTHS].n;

        flSpeed = (float)nHundredths * HUNDREDTHS;

        if ((unsigned int)pArgs[ARG_SLOT].n == PLAN_TOKEN_MESSAGE_DATA)
        {
            nTarget = nSlot;
        }
        else
        {
            nTarget = (short)pArgs[ARG_SLOT].n;
        }
    }

    if (g_nSpeedCacheStale != 0)
    {
        for (int i = 0; i < SPEED_CACHE_COUNT; i++)
        {
            pObject = g_pGameEngine->GetPlayer((short)i);

            g_aSpeedCache[i] = pObject->m_stats.GetScalar(RATING_SPEED);
        }

        g_nSpeedCacheStale = 0;
    }

    const float flBoosted = flSpeed + 1.0f;

    dReading = (float)(short)(long)(flBoosted * g_aSpeedCache[nTarget] + 0.5f) + 0.99f;

    if (SPEED_RATING_MAX <= dReading || dReading >= SPEED_RATING_MIN)
    {
        if (dReading > SPEED_RATING_MAX)
        {
            dReading = SPEED_RATING_MAX;
        }
    }
    else
    {
        dReading = SPEED_RATING_MIN;
    }

    const float flReading = (float)dReading;

    if (nGate != 0)
    {
        if (g_aSpeedCache[0] != -1.0f)
        {
            pObject = g_pGameEngine->GetPlayer(nTarget);

            if (pObject->m_stats.GetScalar(RATING_SPEED) == flReading)
            {
                pObject = g_pGameEngine->GetPlayer(nTarget);

                pObject->m_stats.SetStat(RATING_SLOT(RATING_SPEED), g_aSpeedCache[nTarget]);
            }

            g_nSpeedCacheStale = 1;
        }
    }
    else
    {
        pObject = g_pGameEngine->GetPlayer(nTarget);

        pObject->m_stats.SetStat(RATING_SLOT(RATING_SPEED), flReading);
    }
}

#define ARG_RANGE       1
#define ARG_FACING_GATE 2
#define ARG_DOT_GATE    3
#undef ARG_SLOT
#define ARG_SLOT 4
#undef ARG_DURATION
#define ARG_DURATION 5
#define ARG_LEAD     6

#define DEFAULT_RANGE       100.0f
#define DEFAULT_FACING_GATE 0.1f
#define DEFAULT_DOT_GATE    0.2f
#define DEFAULT_DURATION    20

// A fiftieth, as a double.
#define GATE_SCALE 0.02

#define LAUNCH_MESSAGE_ARG 2

// FUNCTION: HYPERX 0x00464b10
void __cdecl PlanAction::SetImmunity(Interactor* pContext, PlanNode* pArgs, short*, int nArg)
{
    const short nSlot = (short)nArg;

    PlanWord* pBlock;
    int       nIndex;
    float     flValue;

    if (pContext->GetNamedSlot() == -1)
    {
        if (pArgs->m_nArgCount == 2)
        {
            BoundBubble* const pCollision = g_pGameEngine->GetColObjPtr(pContext->GetModelSlot());

            pBlock = pArgs->m_pArgs;

            ((PlayerObject*)pCollision->GetOwner())->m_stats.m_aStats[RATING_SLOT(RATING_IMMUNITY)] =
                (float)(unsigned int)pBlock[1].n;

            return;
        }

        pBlock  = pArgs->m_pArgs;
        nIndex  = (short)pBlock[1].n;
        flValue = (float)(unsigned int)pBlock[2].n;

        g_pGameEngine->GetPlayer(nIndex)->m_stats.m_aStats[RATING_SLOT(RATING_IMMUNITY)] = flValue;

        return;
    }

    pBlock  = pArgs->m_pArgs;
    flValue = (float)(unsigned int)pBlock[2].n;

    if (pBlock[1].n == PLAN_TOKEN_MESSAGE_DATA)
    {
        nIndex = nSlot;
    }
    else
    {
        nIndex = (short)pBlock[1].n;
    }

    g_pGameEngine->GetPlayer(nIndex)->m_stats.m_aStats[RATING_SLOT(RATING_IMMUNITY)] = flValue;
}

// FUNCTION: HYPERX 0x00464c00
void PlanCond::GetImmunity(Interactor* pContext, PlanNode*, short* pResult, int)
{
    PlayerObject* const pActor = pContext->GetPlayerActor();

    if (pActor->m_stats.GetScalar(RATING_IMMUNITY) == g_flZeroPlan)
    {
        *pResult = 0;
    }
    else
    {
        *pResult = 1;
    }
}

// Under five arguments the target is whichever flanking opponent the owner faces, plus one first.
// FUNCTION: HYPERX 0x00464c40
void __cdecl PlanAction::ThrowBungee(Interactor* pContext, PlanNode* pNode, short*, int)
{
    EngineObject* pWinner;

    const short nFirst = (short)((pContext->GetActor()->GetSlot() + 1) % PLAYER_SLOT_COUNT);

    EngineObject* const pFirst = g_pGameEngine->m_ppSlots[nFirst];

    const short nSecond = (short)((pContext->GetActor()->GetSlot() + 3) % PLAYER_SLOT_COUNT);

    float flRange = DEFAULT_RANGE;

    EngineObject* const pSecond = g_pGameEngine->m_ppSlots[nSecond];

    float flLead       = g_flZeroPlan;
    float flFacingGate = DEFAULT_FACING_GATE;
    float flDotGate    = DEFAULT_DOT_GATE;
    short nDuration    = DEFAULT_DURATION;

    const short nArgCount = pNode->m_nArgCount;

    if (nArgCount > 3)
    {
        flRange = (float)(unsigned int)pNode->m_pArgs[ARG_RANGE].n;

        flFacingGate = (float)((unsigned int)pNode->m_pArgs[ARG_FACING_GATE].n * GATE_SCALE - g_flOnePlan);

        flDotGate = (float)((unsigned int)pNode->m_pArgs[ARG_DOT_GATE].n * GATE_SCALE - g_flOnePlan);
    }

    if (nArgCount >= 5)
    {
        PlanWord* const pArgs = pNode->m_pArgs;

        pWinner = g_pGameEngine->m_ppSlots[(short)pArgs[ARG_SLOT].n];

        if (nArgCount >= 6)
        {
            nDuration = (short)pArgs[ARG_DURATION].n;

            if (nArgCount >= 7)
            {
                flLead = (float)(unsigned int)pArgs[ARG_LEAD].n;
            }
        }
    }
    else
    {
        if (IsInReach(pContext, pFirst, flFacingGate, flRange, flDotGate))
        {
            pWinner = pFirst;
        }
        else if (IsInReach(pContext, pSecond, flFacingGate, flRange, flDotGate))
        {
            pWinner = pSecond;
        }
        else
        {
            return;
        }
    }

    if (pWinner == 0)
    {
        return;
    }

    CBungeeToward* const pBungee = (CBungeeToward*)AllocAgendaItem(Agenda_CBungeeToward);

    GameBehaviorObject* const pBindNode = pContext->GetActor()->GetPlanState()->m_pCurrent;

    pBungee->Bind(pContext->GetActor(), pBindNode);

    pBungee->m_pAnchor      = pWinner->GetPositionPtr();
    pBungee->m_flFarRange   = flRange;
    pBungee->m_pTarget      = pWinner;
    pBungee->m_flGain       = DEFAULT_DOT_GATE;
    pBungee->m_flRestLength = 0.0f;
    pBungee->m_flDotGate    = flDotGate;
    pBungee->m_nCountdown   = nDuration;

    if (flLead > g_flZeroPlan)
    {
        pBungee->m_cAttached   = 1;
        pBungee->m_flNearRange = flLead;
    }
    else
    {
        pBungee->SetNearRange(g_flZeroPlan);
        pBungee->m_cAttached = 0;
    }

    pBungee->Activate();

    pWinner->PostEvent(MSG_YER_ATTACKED, LAUNCH_MESSAGE_ARG, pContext->GetActor()->GetSlot());
}

// The holder slot is compared even on the path that never wrote it, an uninitialised read.

// FUNCTION: HYPERX 0x00464f40
void PlanCond::AttachedToSender(Interactor* pContext, PlanNode*, short* pResult, int nArg)
{
    short nHolderSlot;

    AttachPak* pState = pContext->GetActor()->GetPhysicsBody()->pAttach;

    if (pState != 0 && pState->nSlot < 6)
    {
        nHolderSlot = pState->nSlot;
    }

    *pResult = (short)nArg == nHolderSlot;
}

#define OWNER_SCALE 0.125f
#define JITTER_RATE 15.0f

// Four; its marker is in src/globals.cpp.
extern const float g_flFourPlan;

// Without a carrier nCarrier is unset; VC6 homes it on pResult's slot, as the original does.
// FUNCTION: HYPERX 0x00464f80
void __cdecl PlanCond::AmIKnockedLoose(Interactor* pContext, PlanNode*, short* pResult, int nArg)
{
    const short nOwnerSlot = (short)nArg;

    short nCarrier;
    Vec3  vKick;
    Vec3  vVelocity;
    Vec3  vJitter;

    AttachPak* pCarrier = pContext->GetActor()->GetPhysicsBody()->pAttach;

    if (pCarrier != 0 && pCarrier->nSlot < PLAYER_OBJECT_COUNT)
    {
        nCarrier = pCarrier->nSlot;
    }

    if (nOwnerSlot == nCarrier)
    {
        PhysicsBody* pCarrierBlock = g_pGameEngine->m_ppSlots[nCarrier]->GetPhysicsBody();

        Vec3::ScaleTo(&pCarrierBlock->vel, g_flFourPlan, &vKick);

        PhysicsBody* const pBody  = pContext->GetActor()->m_pBody;
        Vec3               vOwner = pBody->vel;

        Vec3::ScaleTo(&vOwner, OWNER_SCALE, &vVelocity);

        vJitter.Set(0.0f, 0.0f, 0.0f);
        vVelocity.Add(vKick);

        vJitter.x += ((float)rand() * (1.0f / RAND_MAX)) * g_flTwoPlan - g_flOnePlan;
        vJitter.y += ((float)rand() * (1.0f / RAND_MAX)) * g_flTwoPlan - g_flOnePlan;
        vJitter.z += (float)rand() * (1.0f / RAND_MAX) * g_flOnePlan + g_flOnePlan;

        Vec3::ScaleTo(&vJitter, g_pGameEngine->m_clock.flDelta * JITTER_RATE, &vJitter);

        vVelocity.Add(vJitter);

        pContext->GetActor()->SetVelocity(vVelocity);

        *pResult = 0;

        return;
    }

    *pResult = 1;
}

// The south goal clamps the Rok's value up to one; the north clamps it down to minus one and flips it.

// FUNCTION: HYPERX 0x00465150
void PlanCond::RokScoreValue(Interactor* pContext, PlanNode*, short* pResult, int)
{
    int nClamped;

    short nRokValue = g_pGameEngine->GetRok()->m_wRokValue;

    const short nSlot = pContext->GetActor()->GetSlot();

    if (nSlot == SLOT_SGOAL)
    {
        if (nRokValue < 1)
        {
            nRokValue = 1;
        }
    }
    else if (nSlot == SLOT_NGOAL)
    {
        nClamped = min(-1, nRokValue);

        nRokValue = -nClamped;
    }
    else
    {
        nRokValue = 0;
    }

    if (g_pGameEngine->m_ppSlots[SLOT_ROK]->GetPhysicsBody()->pAttach != 0)
    {
        *pResult = 4;
    }

    if (nRokValue == 1)
    {
        *pResult = 0;
    }
    else if (nRokValue == 2)
    {
        *pResult = 1;
    }
    else
    {
        if (nRokValue == 4)
        {
            *pResult = 2;
        }
        else
        {
            *pResult = 0;
        }
    }

    if (g_pGameEngine->GetRok()->m_bAtSpawnSlot != 0)
    {
        *pResult = 3;
    }
}

// FUNCTION: HYPERX 0x00465210
void PlanCond::CatchBlockOrMiss(Interactor* pContext, PlanNode*, short* pResult, int)
{
    g_pGameEngine->GetColObjPtr(pContext->GetModelSlot());

    g_pGameEngine->GetColObjPtr(pContext->GetOtherSlot());

    *pResult = 0;
}

// Zero for fast and one for slow; a flying actor answers one without measuring.

// Ten units a second, at 0x004b14d4.
#define SPEED_THRESHOLD 10.0f

// FUNCTION: HYPERX 0x00465280
void __cdecl PlanCond::Jumping(Interactor* pContext, PlanNode*, short* pResult, int)
{
    BoundBubble* const pCollision = g_pGameEngine->GetColObjPtr(pContext->GetModelSlot());

    EngineObject* const pActor = pCollision->GetOwner();

    if (pActor->GetPlanState()->m_pCurrent->m_wKind != BEH_FLY)
    {
        const float flInvDelta = g_pGameEngine->m_clock.flInvDelta;

        if (flInvDelta * pActor->GetPhysicsBody()->flSpeed >= SPEED_THRESHOLD)
        {
            *pResult = 0;

            return;
        }
    }

    *pResult = 1;
}

// Within ten of an object without the 0x10 bit answers two; both or neither answers three or a coin.

#define STATE_AVAILABLE 0x10

// Inside this the actor counts as standing at the object.
#define AT_OBJECT_RANGE 10.0f

// The four answers: the mine, the swarf, no, and neither.
enum NearestAnswer
{
    ANSWER_MINE    = 0,
    ANSWER_SWARF   = 1,
    ANSWER_NO      = 2,
    ANSWER_NEITHER = 3
};

// FUNCTION: HYPERX 0x00465310
void __cdecl PlanCond::RandomInactiveWeapon(Interactor* pContext, PlanNode*, short* pResult, int)
{
    short nAtFirst  = 0;
    short nAtSecond = 0;

    EngineObject* const pActor = pContext->GetActor();

    EngineObject** const apSlots = g_pGameEngine->m_ppSlots;
    EngineObject* const  pFirst  = apSlots[SLOT_MINE];
    EngineObject* const  pSecond = apSlots[SLOT_SWARF];

    Vec3 vFirst  = *pFirst->GetPositionPtr();
    Vec3 vSecond = *pSecond->GetPositionPtr();

    Vec3 vSelf = *pActor->GetPositionPtr();

    float flDx = vSelf.x - vFirst.x;
    float flDy = vSelf.y - vFirst.y;
    float flDz = vSelf.z - vFirst.z;

    if ((float)sqrt(flDx * flDx + flDy * flDy + flDz * flDz) < AT_OBJECT_RANGE)
    {
        if (((unsigned char)pFirst->m_nFlags & STATE_AVAILABLE) != 0)
        {
            nAtFirst = 1;
        }
        else
        {
            *pResult = ANSWER_NO;
            return;
        }
    }

    flDx = vSelf.x - vSecond.x;
    flDy = vSelf.y - vSecond.y;
    flDz = vSelf.z - vSecond.z;

    if ((float)sqrt(flDx * flDx + flDy * flDy + flDz * flDz) < AT_OBJECT_RANGE)
    {
        if (((unsigned char)pSecond->m_nFlags & STATE_AVAILABLE) != 0)
        {
            nAtSecond = 1;
        }
        else
        {
            *pResult = ANSWER_NO;
            return;
        }
    }

    if (nAtFirst > nAtSecond)
    {
        *pResult = ANSWER_MINE;
        return;
    }

    if (nAtSecond > nAtFirst)
    {
        *pResult = ANSWER_SWARF;
        return;
    }

    if (nAtSecond == nAtFirst)
    {
        if (nAtSecond == 0)
        {
            *pResult = ANSWER_NEITHER;
            return;
        }

        *pResult = (short)(rand() % 2);
    }
}

// The answer is one-based, so zero means no armory holds fewer than every weapon.
// FUNCTION: HYPERX 0x004654b0
void __cdecl PlanCond::RandomEmptyArmory(Interactor*, PlanNode*, short* pResult, int)
{
    const int nSlots = g_pGameEngine->GetArmoryCount();

    int nFound = -1;

    int nBest = g_pGameEngine->GetMineCount() + g_pGameEngine->GetSwarfCount();

    for (int iSlot = 0; iSlot < nSlots; iSlot++)
    {
        const int nArmory = iSlot + 1;
        const int nCount  = GameEngine::CountEntitiesInRange((short)(nArmory + SLOT_ARMORY - 1));

        if (nBest > nCount)
        {
            nBest  = nCount;
            nFound = nArmory;
        }
    }

    if (nFound == -1)
    {
        *pResult = 0;

        return;
    }

    *pResult = (short)nFound;
}

// Two arms pick a different killed answer when GameEngine::m_nBrutality is off.

// FUNCTION: HYPERX 0x00465650
void PlanCond::WhoKilledMe(Interactor* pContext, PlanNode*, short* pResult, int)
{
    char bBrutalityOff = 0;

    PlayerObject* const pObject = pContext->GetPlayerActor();

    const float flDamage = pObject->m_stats.GetScalar(RATING_DAMAGE);

    if (g_pGameEngine->m_nBrutality == 0)
    {
        bBrutalityOff = 1;
    }

    switch (pObject->m_wKilledBySlot)
    {
        case SLOT_TURNSTILE:
        case SLOT_TURNSTILE + 1:
        case SLOT_TURNSTILE + 2:
        case SLOT_TURNSTILE + 3:
        case SLOT_TURNSTILE + 4:
        case SLOT_TURNSTILE + 5:
        case SLOT_TURNSTILE_LAST:
            if (flDamage <= g_flZeroPlan)
            {
                if (bBrutalityOff)
                {
                    *pResult = 9;
                }
                else
                {
                    *pResult = 1;
                }
                return;
            }

            *pResult = 5;
            return;

        case SLOT_LASER_HURDLE:
        case SLOT_LASER_HURDLE + 1:
        case SLOT_LASER_HURDLE + 2:
        case SLOT_LASER_HURDLE + 3:
        case SLOT_LASER_HURDLE + 4:
        case SLOT_LASER_HURDLE_LAST:
            if (flDamage <= g_flZeroPlan)
            {
                if (bBrutalityOff)
                {
                    *pResult = 3;
                }
                else
                {
                    *pResult = 4;
                }
                return;
            }

            *pResult = 6;
            return;

        case SLOT_MINE:
            if (flDamage <= g_flZeroPlan)
            {
                *pResult = 2;
                return;
            }

            *pResult = 7;
            return;

        case SLOT_ROK:
            break;

        default:
            if (flDamage <= g_flZeroPlan)
            {
                *pResult = 9;
                return;
            }

            *pResult = 0;
            return;
    }

    if (flDamage <= g_flZeroPlan)
    {
        *pResult = 3;
        return;
    }

    *pResult = 8;
}

// The two normalised direction copies are never read, in the original too.

// Skaters are asked for their stats; anything else stands in.

// What a non-player stands in with.
#define DAMAGE_STANDIN_CHECKING 2.0f
#define STANDIN_WEIGHT          10.0f
#define STANDIN_DAMAGE          100.0f

// The range the two objects have to be inside, taken twice over on the easiest level.
#define RANGE_BASE 150.0f

// The largest damage the condition will answer.
#define HIT_DAMAGE_CEILING 0x14

// An object that hits carries its own slot as the attack; the engine's own hit is the far goalie's.
// FUNCTION: HYPERX 0x004657b0
void __cdecl PlanCond::TakeDamage(Interactor* pContext, PlanNode* pArgs, short* pResult, int nAttack)
{
    float flOtherDamage;
    float flOtherWeight;
    float flOtherChecking;
    float flRange;
    float flDamage;
    float flStrength;
    Vec3  vOffset;

    PlayerObject* pActor = pContext->GetPlayerActor();

    short nAttacker = pContext->GetTargetSlot();

    if (nAttacker == SLOT_GOD)
    {
        if (pActor->GetSlot() % 2 != 0)
        {
            nAttacker = GOALIE_SLOT_A;
        }
        else
        {
            nAttacker = GOALIE_SLOT_B;
        }
    }

    pActor->m_wAttackeeSlot = nAttacker;

    *pResult = 0;

    if (nAttacker == SLOT_ROK)
    {
        if (nAttack == SLOT_ROK)
        {
            *pResult = pArgs->m_nArgCount - 3;

            return;
        }
    }
    else if (nAttacker == SLOT_MINE && nAttack == SLOT_MINE)
    {
        *pResult = pArgs->m_nArgCount - 3;

        return;
    }

    if (nAttacker == (short)nAttack)
    {
        if (nAttacker >= SLOT_TURNSTILE && nAttacker <= SLOT_TURNSTILE_LAST)
        {
            *pResult = pArgs->m_nArgCount - 3;

            return;
        }

        if (nAttacker >= SLOT_LASER_HURDLE && nAttacker <= SLOT_LASER_HURDLE_LAST)
        {
            *pResult = pArgs->m_nArgCount - 3;

            return;
        }
    }

    if (nAttacker != 0 && nAttacker != 1 && nAttacker != 2 && nAttacker != 3 && nAttacker != 4 && nAttacker != 5)
    {
        *pResult = pArgs->m_nArgCount - 3;

        return;
    }

    EngineObject* const pOther = g_pGameEngine->m_ppSlots[nAttacker];

    const Vec3* pPosSelf  = pActor->GetPositionPtr();
    const Vec3* pPosOther = pOther->GetPositionPtr();

    vOffset.x = pPosOther->x - pPosSelf->x;
    vOffset.y = pPosOther->y - pPosSelf->y;
    vOffset.z = pPosOther->z - pPosSelf->z;

    if (nAttacker <= SLOT_DROMER3)
    {
        ObjectStats* const pOtherStats = &((PlayerObject*)pOther)->m_stats;

        flOtherChecking = pOtherStats->GetWeightedStat(RATING_CHECKING);
        flOtherWeight   = pOtherStats->GetWeightedStat(RATING_WEIGHT_CLASS);
        flOtherDamage   = pOtherStats->GetWeightedStat(RATING_DAMAGE);
    }
    else
    {
        flOtherChecking = DAMAGE_STANDIN_CHECKING;
        flOtherWeight   = STANDIN_WEIGHT;
        flOtherDamage   = STANDIN_DAMAGE;
    }

    ObjectStats* const pStats = &pActor->m_stats;

    pStats->GetScalar(RATING_DAMAGE);

    const float flOwnWeight = pStats->GetWeightedStat(RATING_WEIGHT_CLASS);

    if (g_pGameEngine->GetDifficultyLevel() == DIFFICULTY_EASY)
    {
        flRange = RANGE_BASE;
    }
    else
    {
        flRange = 0.0f;
    }

    if (Vec3::DotInline(&vOffset, &vOffset) <= flRange + RANGE_BASE && (pStats->GetScalar(RATING_IMMUNITY) == 0.0f || nAttack == ATTACK_LEG_SWEEP_L || nAttack == ATTACK_LEG_SWEEP_R))
    {
        pStats->GetScalar(RATING_DAMAGE);

        Vec3         vSelfDir;
        Vec3         vOtherDir;
        PhysicsBody* pBlockOther;
        PhysicsBody* pBlockSelf;

        switch (nAttack)
        {
            case ATTACK_HIP_CHECK:
                flStrength = flOtherDamage * flOtherChecking;

                flStrength = flStrength * 0.01f + flOtherChecking;

                flStrength *= 0.5f;

                flStrength += 9.0f;

                flDamage = flStrength * -1.0f;

                break;

            case ATTACK_PUNCH:
            {
                vSelfDir  = pActor->GetPhysicsBody()->velDir;
                vOtherDir = pOther->GetPhysicsBody()->velDir;

                Math::Vec3Normalize(&vSelfDir);
                Math::Vec3Normalize(&vOtherDir);

                pBlockOther = pOther->GetPhysicsBody();
                pBlockSelf  = pActor->GetPhysicsBody();

                flDamage = Vec3::DotInline(&pBlockSelf->velDir, &pBlockOther->velDir);

                flDamage *= -0.25f;

                flDamage += 1.0f;

                flDamage *= flOtherWeight / flOwnWeight;

                flStrength = flOtherDamage * flOtherChecking;

                flStrength = flStrength * 0.01f + flOtherChecking;

                flStrength *= 0.5f;

                flDamage *= flStrength;

                flDamage *= 0.1666666716337204f;

                flDamage *= -4.0f;
            }

            break;

            case ATTACK_CLOTHES_LINE_L:
            case ATTACK_CLOTHES_LINE_R:
            case ATTACK_SHOULDER_CHECK_L:
            case ATTACK_SHOULDER_CHECK_R:
            {
                vSelfDir  = pActor->GetPhysicsBody()->velDir;
                vOtherDir = pOther->GetPhysicsBody()->velDir;

                Math::Vec3Normalize(&vSelfDir);
                Math::Vec3Normalize(&vOtherDir);

                pBlockOther = pOther->GetPhysicsBody();
                pBlockSelf  = pActor->GetPhysicsBody();

                flDamage = Vec3::DotInline(&pBlockSelf->velDir, &pBlockOther->velDir);

                flDamage *= -0.25f;

                flDamage += 1.0f;

                flDamage *= flOtherWeight / flOwnWeight;

                flStrength = flOtherDamage * flOtherChecking;

                flStrength = flStrength * 0.01f + flOtherChecking;

                flStrength *= 0.5f;

                flDamage *= flStrength;

                flDamage *= 0.1428571492433548f;

                flDamage *= -10.0f;
            }

            break;

            default:
                flDamage = g_flZeroPlan;

                break;
        }

        if (*pResult != 0)
        {
            return;
        }

        int nMagnitude = abs((short)flDamage);

        if (nMagnitude > HIT_DAMAGE_CEILING)
        {
            nMagnitude = HIT_DAMAGE_CEILING;
        }

        *pResult = (short)nMagnitude;

        return;
    }

    *pResult = pArgs->m_nArgCount - 2;
}

// Clears the owner's last-hit word on the way in, so each hit is read once.

// What a non-player stands in with.
#define ATTACKED_STANDIN_CHECKING 15.0f

// The squared range and the two dot thresholds; the dots are doubles in the image.
#define ATTACKED_RANGE_SQ 100.0f
#define DOT_FRONT         0.5
#define DOT_BEHIND        -0.8

// FUNCTION: HYPERX 0x00465c00
void __cdecl PlanCond::AttackedAnim(Interactor* pContext, PlanNode* pArgs, short* pResult, int nAttack)
{
    int   nReaction;
    float flOther;
    float flWeight;
    Vec3  vOffset;

    // Only the z is ever written.
    Vec3 vCross;

    PlayerObject* pActor = pContext->GetPlayerActor();

    const short nSlot = pActor->m_wAttackeeSlot;

    pActor->m_wAttackeeSlot = -1;

    if (nSlot == -1)
    {
        *pResult = pArgs->m_nArgCount - 2;

        return;
    }

    ObjectStats* const pStats = &pActor->m_stats;

    float flDamage = pStats->GetScalar(RATING_DAMAGE);

    if (flDamage <= DamageGate())
    {
        *pResult = pArgs->m_nArgCount - 3;

        switch (nAttack)
        {
            case ATTACK_HIP_CHECK:
                pActor->m_wKilledBySlot = SLOT_TURNSTILE;
                break;
        }

        return;
    }

    if (nAttack == ATTACK_PUNCH)
    {
        *pResult = pArgs->m_nArgCount - 2;

        return;
    }

    EngineObject* const pOther = g_pGameEngine->m_ppSlots[nSlot];

    const Vec3* pPosSelf  = pActor->GetPositionPtr();
    const Vec3* pPosOther = pOther->GetPositionPtr();

    vOffset.x = pPosOther->x - pPosSelf->x;
    vOffset.y = pPosOther->y - pPosSelf->y;
    vOffset.z = pPosOther->z - pPosSelf->z;

    if (nSlot <= SLOT_DROMER3)
    {
        ObjectStats* const pOtherStats = &((PlayerObject*)pOther)->m_stats;

        flOther  = pOtherStats->GetWeightedStat(RATING_CHECKING);
        flWeight = pOtherStats->GetWeightedStat(RATING_WEIGHT_CLASS);
    }
    else
    {
        flOther  = ATTACKED_STANDIN_CHECKING;
        flWeight = 20.0f;
    }

    flOther += flWeight;

    flDamage = pStats->GetScalar(RATING_DAMAGE);

    const float flOwnWeight = pStats->GetWeightedStat(RATING_WEIGHT_CLASS);

    float flScaled = flOwnWeight;

    flScaled *= flDamage;

    const float flSelf = flScaled * 0.01f + flOwnWeight;

    Math::Vec3Normalize(&vOffset);

    const Vec3* pDir = &pActor->GetPhysicsBody()->velDir;

    vCross.z = vOffset.y * pDir->x - vOffset.x * pDir->y;

    pDir = &pActor->GetPhysicsBody()->velDir;

    const float flDot = vOffset.z * pDir->z + vOffset.y * pDir->y + vOffset.x * pDir->x;

    if (Vec3::DotInline(&vOffset, &vOffset) <= ATTACKED_RANGE_SQ && (pStats->GetScalar(RATING_IMMUNITY) == 0.0f || nAttack == ATTACK_LEG_SWEEP_L || nAttack == ATTACK_LEG_SWEEP_R))
    {
        if (g_flTwoPlan + flOther > flSelf)
        {
            switch (nAttack)
            {
                case ATTACK_PUNCH:
                    if (flDot > 0.0f)
                    {
                        nReaction = 0;
                    }
                    else
                    {
                        nReaction = 1;
                    }

                    *pResult = (short)nReaction;

                    return;

                case ATTACK_HIP_CHECK:
                    if (flDot > DOT_FRONT)
                    {
                        nReaction = 4;
                    }
                    else if (flDot > 0.0f)
                    {
                        if (vCross.z > 0.0f)
                        {
                            nReaction = 2;
                        }
                        else
                        {
                            nReaction = 3;
                        }
                    }
                    else
                    {
                        nReaction = 5;
                    }

                    *pResult = (short)nReaction;

                    return;

                case ATTACK_CLOTHES_LINE_L:
                case ATTACK_CLOTHES_LINE_R:
                    if (flDot > 0.0f)
                    {
                        nReaction = 6;
                    }
                    else
                    {
                        nReaction = 7;
                    }

                    *pResult = (short)nReaction;

                    return;

                case ATTACK_SHOULDER_CHECK_L:
                case ATTACK_SHOULDER_CHECK_R:
                    if (flDot > DOT_BEHIND)
                    {
                        nReaction = 8;
                    }
                    else
                    {
                        nReaction = 9;
                    }

                    *pResult = (short)nReaction;

                    return;

                case ATTACK_LEG_SWEEP_L:
                case ATTACK_LEG_SWEEP_R:
                    if (flDot > 0.0f)
                    {
                        nReaction = 10;
                    }
                    else
                    {
                        nReaction = 11;
                    }

                    *pResult = (short)nReaction;

                    return;

                default:
                    *pResult = pArgs->m_nArgCount - 2;

                    return;
            }
        }
        else
        {
            switch (nAttack)
            {
                case ATTACK_PUNCH:
                    if (flDot > 0.0f)
                    {
                        nReaction = 0;
                    }
                    else
                    {
                        nReaction = 1;
                    }

                    *pResult = (short)nReaction;

                    return;

                case ATTACK_HIP_CHECK:
                    if (flDot > 0.0f)
                    {
                        if (vCross.z > 0.0f)
                        {
                            nReaction = 12;
                        }
                        else
                        {
                            nReaction = 13;
                        }
                    }
                    else
                    {
                        nReaction = 14;
                    }

                    *pResult = (short)nReaction;

                    return;

                case ATTACK_CLOTHES_LINE_L:
                case ATTACK_CLOTHES_LINE_R:
                    if (flDot > 0.0f)
                    {
                        nReaction = 15;
                    }
                    else
                    {
                        if (vCross.z > 0.0f)
                        {
                            nReaction = 16;
                        }
                        else
                        {
                            nReaction = 17;
                        }
                    }

                    *pResult = (short)nReaction;

                    return;

                case ATTACK_SHOULDER_CHECK_L:
                case ATTACK_SHOULDER_CHECK_R:
                    if (flDot > DOT_FRONT)
                    {
                        nReaction = 20;
                    }
                    else if (flDot > 0.0f)
                    {
                        if (vCross.z > 0.0f)
                        {
                            nReaction = 18;
                        }
                        else
                        {
                            nReaction = 19;
                        }
                    }
                    else
                    {
                        nReaction = 21;
                    }

                    *pResult = (short)nReaction;

                    return;

                case ATTACK_LEG_SWEEP_L:
                case ATTACK_LEG_SWEEP_R:
                    if (flDot > 0.0f)
                    {
                        nReaction = 10;
                    }
                    else
                    {
                        nReaction = 11;
                    }

                    *pResult = (short)nReaction;

                    return;
            }
        }
    }

    *pResult = pArgs->m_nArgCount - 2;
}

// The straight arm is looked up and never read; the shoulder checks split left from right only in states nine and thirteen.

// GameEngine::SetTeams writes the first from its home argument and the second from its away one.
// FUNCTION: HYPERX 0x00466190
void __cdecl PlanCond::SpecialAttacks(Interactor* pContext, PlanNode*, short* pResult, int nAnim)
{
    short nState;

    EngineObject* const pActor = pContext->GetActor();

    *pResult = 0;

    if ((pActor->GetSlot() & 1) == 0)
    {
        nState = g_pGameEngine->m_nTeamHomeIndex;
    }
    else
    {
        nState = g_pGameEngine->m_nTeamAwayIndex;
    }

    // STRING: HYPERX 0x004c4658
    g_pAnimRegistry->FindAnimByNameIdx("AA_StrtarmL");

    // STRING: HYPERX 0x004c464c
    const short nHipcheck = g_pAnimRegistry->FindAnimByNameIdx("AA_Hipcheck");

    // STRING: HYPERX 0x004c463c
    const short nClothesLineL = g_pAnimRegistry->FindAnimByNameIdx("AA_ClthslneL");

    // STRING: HYPERX 0x004c462c
    const short nClothesLineR = g_pAnimRegistry->FindAnimByNameIdx("AA_ClthslneR");

    // STRING: HYPERX 0x004c4620
    const short nShoulderL = g_pAnimRegistry->FindAnimByNameIdx("AA_ShlchekL");

    // STRING: HYPERX 0x004c4614
    const short nShoulderR = g_pAnimRegistry->FindAnimByNameIdx("AA_ShlchekR");

    // STRING: HYPERX 0x004c4608
    const short nLegTripL = g_pAnimRegistry->FindAnimByNameIdx("AA_LegtripL");

    // STRING: HYPERX 0x004c45fc
    const short nLegTripR = g_pAnimRegistry->FindAnimByNameIdx("AA_LegtripR");

    switch (nState)
    {
        case 1:
            if (nAnim == nClothesLineL)
            {
                *pResult = 4;
            }

            if (nAnim == nClothesLineR)
            {
                *pResult = 5;
            }
            break;

        case 4:
            if (nAnim == nHipcheck)
            {
                *pResult = 10;
            }
            break;

        case 5:
            if (nAnim == nLegTripL)
            {
                *pResult = 1;
            }

            if (nAnim == nLegTripR)
            {
                *pResult = 1;
            }
            break;

        case 8:
            if (nAnim == nShoulderL)
            {
                *pResult = 3;
            }

            if (nAnim == nShoulderR)
            {
                *pResult = 3;
            }
            break;

        case 9:
            if (nAnim == nShoulderL)
            {
                *pResult = 8;
            }

            if (nAnim == nShoulderR)
            {
                *pResult = 9;
            }
            break;

        case 11:
            if (nAnim == nShoulderL)
            {
                *pResult = 2;
            }

            if (nAnim == nShoulderR)
            {
                *pResult = 2;
            }
            break;

        case 13:
            if (nAnim == nShoulderL)
            {
                *pResult = 6;
            }

            if (nAnim == nShoulderR)
            {
                *pResult = 7;
            }
            break;
    }
}

// A rok bound to the actor or its team mate always counts as close.

// FUNCTION: HYPERX 0x004663f0
void __cdecl PlanCond::MeOrTeammateClosestToRok(Interactor* pContext, PlanNode*, short* pResult, int)
{
    const Vec3* pRokAt;
    const Vec3* pOppAt;
    AttachPak*  pnBoundSlot;
    short       nBound;
    Vec3        vOpp;

    pRokAt = g_pGameEngine->m_ppSlots[SLOT_ROK]->GetPositionPtr();

    const Vec3* pOwnerAt = pContext->GetActor()->GetPositionPtr();

    const short nSlotA = pContext->GetActor()->GetSlot();

    pOppAt = g_pGameEngine->m_ppSlots[TeammateSlot(nSlotA)]->GetPositionPtr();

    const float flOwnX = pRokAt->x - pOwnerAt->x;
    const float flOwnY = pRokAt->y - pOwnerAt->y;
    const float flOwnZ = pRokAt->z - pOwnerAt->z;

    vOpp.x = pRokAt->x - pOppAt->x;
    vOpp.y = pRokAt->y - pOppAt->y;
    vOpp.z = pRokAt->z - pOppAt->z;

    if (flOwnX * flOwnX + flOwnY * flOwnY + flOwnZ * flOwnZ < vOpp.x * vOpp.x + vOpp.y * vOpp.y + vOpp.z * vOpp.z)
    {
        *pResult = 0;
    }
    else
    {
        *pResult = 1;
    }

    const short nSlotB = pContext->GetActor()->GetSlot();

    PlayerObject* const pOppObj = g_pGameEngine->GetPlayer(TeammateSlot(nSlotB));

    const float flDamage = pOppObj->m_stats.GetScalar(RATING_DAMAGE);

    if (*pResult == 1)
    {
        if (flDamage <= DamageGate())
        {
            *pResult = 0;
        }
    }

    const int nSlotC = pContext->GetActor()->GetSlot();

    const int nOppC = TeammateSlot(nSlotC);

    pnBoundSlot = g_pGameEngine->m_ppSlots[SLOT_ROK]->GetPhysicsBody()->pAttach;

    if (pnBoundSlot != 0 && (nBound = pnBoundSlot->nSlot) < PLAYER_OBJECT_COUNT && (nBound == nSlotC || nBound == nOppC))
    {
        *pResult = 1;
    }
}

// Team two's score is read first, so the difference is the away team's lead; only its size matters.

// FUNCTION: HYPERX 0x004665b0
void PlanCond::TenOrMorePtSpread(Interactor*, PlanNode*, short* pResult, int)
{
    int nDiff;

    nDiff = g_pGameEngine->GetAwayScore() - g_pGameEngine->GetHomeScore();

    *pResult = abs(nDiff) < 10;
}

#define GREEN_RANDOMIZER_COUNT (SLOT_RED_RANDOMIZER - SLOT_GREEN_RANDOMIZER)

// The answer is one past the chosen slot, so zero means none.
// FUNCTION: HYPERX 0x004665f0
void __cdecl PlanCond::RedIsAvailable(Interactor*, PlanNode*, short* pResult, int)
{
    int aFound[GREEN_RANDOMIZER_COUNT];

    if (g_pGameEngine->m_ppSlots[SLOT_RED_RANDOMIZER]->GetPhysicsBody()->axis.x > g_flZeroPlan)
    {
        int  nSlot  = 0;
        int  nCount = 0;
        int* pFound = aFound;

        for (; nSlot < GREEN_RANDOMIZER_COUNT; nSlot++)
        {
            if (g_pGameEngine->m_ppSlots[(short)(nSlot + SLOT_GREEN_RANDOMIZER)] != 0)
            {
                *pFound = nSlot;
                nCount++;
                pFound++;
            }
        }

        if (nCount != 0)
        {
            *pResult = (short)(aFound[rand() % nCount] + 1);

            return;
        }
    }

    *pResult = 0;
}

#define SPEED_GATE 10.0f

// The rating index comes from the node, so one condition thresholds any stat.
// FUNCTION: HYPERX 0x00466680
void __cdecl PlanCond::RatingMaxedOut(Interactor* pContext, PlanNode* pNode, short* pnResult, int nArg)
{
    const short nDefaultSlot = (short)nArg;

    PlayerObject* pObject;

    if (pContext->GetNamedSlot() == -1)
    {
        BoundBubble* const pBubble = g_pGameEngine->GetColObjPtr(pContext->GetModelSlot());

        pObject = (PlayerObject*)pBubble->GetOwner();
    }
    else
    {
        pObject = g_pGameEngine->GetPlayer(nDefaultSlot);
    }

    if (pObject->m_stats.GetScalar(pNode->GetArgLeaf(1)) >= SPEED_GATE)
    {
        *pnResult = 1;
    }
    else
    {
        *pnResult = 2;
    }
}

// FUNCTION: HYPERX 0x00466710
void __cdecl PlanCond::ChooseTeamTune(Interactor* pContext, PlanNode*, short* pResult, int nTeam)
{
    int   nIndex;
    short nTeamIndex;

    short nSelected = pContext->GetNamedSlot();

    if (nSelected == -1)
    {
        nSelected = pContext->GetModelSlot();
        nIndex    = CollisionObj::GetModelId(nSelected);
    }
    else
    {
        nIndex = nTeam;
    }

    if ((nIndex & 1) != 0)
    {
        nTeamIndex = g_pGameEngine->m_nTeamAwayIndex;
    }
    else
    {
        nTeamIndex = g_pGameEngine->m_nTeamHomeIndex;
    }

    *pResult = nTeamIndex;
}

// FUNCTION: HYPERX 0x00466770
void __cdecl PlanCond::MyTeamLosingByAtLeast(Interactor* pContext, PlanNode*, short* pResult, int nArg)
{
    unsigned int nValue;
    int          nHomeScore;
    int          nAwayScore;

    if (pContext->GetNamedSlot() == -1)
    {
        nValue = CollisionObj::GetModelId(pContext->GetModelSlot());
    }
    else
    {
        nValue = nArg;
    }

    if ((nValue & 1) != 0)
    {
        nAwayScore = g_pGameEngine->GetAwayScore();
        nHomeScore = g_pGameEngine->GetHomeScore();

        if (nHomeScore - nAwayScore < 5)
        {
            *pResult = 2;
        }
        else
        {
            *pResult = 1;
        }

        return;
    }

    nAwayScore = g_pGameEngine->GetAwayScore();
    nHomeScore = g_pGameEngine->GetHomeScore();

    if (nAwayScore - nHomeScore < 5)
    {
        *pResult = 2;
    }
    else
    {
        *pResult = 1;
    }
}

// The range is two below the argument count, and one is the floor.

// FUNCTION: HYPERX 0x00466800
void PlanCond::DecideWithRating(Interactor* pContext, PlanNode* pArgs, short* pResult, int)
{
    PlayerObject* const pActor = pContext->GetPlayerActor();

    const int nStat = pArgs->m_pArgs[1].pNode->m_pArgs[0].n;

    const short nRange = (short)(pArgs->m_nArgCount - 2);

    *pResult = (short)(int)(pActor->m_stats.GetScalar(nStat) * (nRange * g_flPlanStatScale));

    if (*pResult > nRange)
    {
        *pResult = nRange;
    }

    if (*pResult < 1)
    {
        *pResult = 1;
    }
}

// The rok is asked only while its kill state is not minus one.

// FUNCTION: HYPERX 0x00466870
void PlanCond::AttackOrDeployWeapon(Interactor* pContext, PlanNode*, short* pResult, int nArg)
{
    const short nBase = (short)nArg;

    EngineObject* pActor;
    AttachPak*    pSlot;
    short         nSlot;

    EngineObject* pTracked = g_pGameEngine->m_ppSlots[SLOT_SWARF];

    if (pTracked != 0)
    {
        pSlot = pTracked->GetPhysicsBody()->pAttach;

        if (pSlot != 0)
        {
            nSlot = pSlot->nSlot;

            if (nSlot < PLAYER_OBJECT_COUNT)
            {
                pActor = pContext->GetActor();

                if (nSlot == pActor->GetSlot())
                {
                    *pResult = 0;

                    return;
                }
            }
        }
    }

    pTracked = g_pGameEngine->m_ppSlots[SLOT_MINE];

    if (pTracked != 0)
    {
        pSlot = pTracked->GetPhysicsBody()->pAttach;

        if (pSlot != 0)
        {
            nSlot = pSlot->nSlot;

            if (nSlot < PLAYER_OBJECT_COUNT)
            {
                pActor = pContext->GetActor();

                if (nSlot == pActor->GetSlot())
                {
                    *pResult = 0;

                    return;
                }
            }
        }
    }

    pTracked = g_pGameEngine->m_ppSlots[SLOT_ROK];

    if (pTracked != 0 && ((RokObject*)pTracked)->m_nKillState > -1)
    {
        pSlot = pTracked->GetPhysicsBody()->pAttach;

        if (pSlot != 0)
        {
            nSlot = pSlot->nSlot;

            if (nSlot < PLAYER_OBJECT_COUNT)
            {
                pActor = pContext->GetActor();

                if (nSlot == pActor->GetSlot())
                {
                    *pResult = 0;

                    return;
                }
            }
        }
    }

    *pResult = (short)(nBase + 1);
}

// FUNCTION: HYPERX 0x00466960
void __cdecl PlanCond::DromerNearMine(Interactor*, PlanNode* pArgs, short* pResult, int nArg)
{
    const short nSlot = (short)nArg;

    EngineObject* const pTarget = g_pGameEngine->m_ppSlots[nSlot];
    const short         nLimit  = (short)pArgs->m_pArgs[1].pNode->m_pArgs[0].n;
    EngineObject* const pTeam   = g_pGameEngine->m_ppSlots[SLOT_MINE];

    // A missing reference object leaves the answer alone rather than writing one.
    if (pTeam != 0)
    {
        Vec3* const pFrom = pTeam->GetPositionPtr();
        Vec3* const pTo   = pTarget->GetPositionPtr();

        const float dx = pTo->x - pFrom->x;
        const float dy = pTo->y - pFrom->y;
        const float dz = pTo->z - pFrom->z;

        // This one takes the root, unlike its neighbour: the limit here is a plain distance.
        if ((float)sqrt(dx * dx + dy * dy + dz * dz) <= nLimit)
        {
            *pResult = 1;

            return;
        }

        *pResult = 2;
    }
}

// Only even directions between minus four and two answer; everything else answers zero.

// FUNCTION: HYPERX 0x004669f0
void PlanCond::RokValueInMyFavor(Interactor*, PlanNode*, short* pResult, int)
{
    RokObject* const pCarrier = g_pGameEngine->GetRok();

    short nDirection = pCarrier->m_wRokValue;

    AttachPak* pState = pCarrier->GetPhysicsBody()->pAttach;

    if (pState != 0 && pState->nSlot < 6)
    {
        if ((pState->nSlot & 1) != 0)
        {
            nDirection = (short)-nDirection;
        }
    }
    else
    {
        *pResult = 0;
    }

    switch (nDirection)
    {
        case 2:
            *pResult = 1;
            break;

        case 0:
            *pResult = 2;
            break;

        case -2:
            *pResult = 3;
            break;

        case -4:
            *pResult = 4;
            break;

        case 4:
            *pResult = 0;
            break;

        default:
            *pResult = 0;
            break;
    }
}

#define PASS_NEAR_RANGE_SQUARED 22000.0f

// A team mate with long passes enabled, or one on its feet within range, answers nought.
// FUNCTION: HYPERX 0x00466a80
void __cdecl PlanCond::CheckPassingDistance(Interactor* pContext, PlanNode*, short* pResult, int)
{
    Vec3 vDiff;

    EngineObject* const pActor = pContext->GetActor();

    const int nSlot = pActor->GetSlot();

    PlayerObject* pMate = g_pGameEngine->GetPlayer(TeammateSlot((short)nSlot));

    Vec3* const pSelfPos = pContext->GetActor()->GetPositionPtr();

    Vec3* const pMatePos = pMate->GetPositionPtr();

    vDiff.x = pMatePos->x - pSelfPos->x;
    vDiff.y = pMatePos->y - pSelfPos->y;
    vDiff.z = pMatePos->z - pSelfPos->z;

    if (pMate->m_stats.GetScalar(RATING_LONG_PASS_ENABLE) != 0.0f)
    {
        *pResult = 0;
        return;
    }

    if (vDiff.x * vDiff.x + (vDiff.y * vDiff.y + vDiff.z * vDiff.z) < PASS_NEAR_RANGE_SQUARED && pMate->m_nKnockedDownState == 0)
    {
        *pResult = 0;
        return;
    }

    *pResult = 1;
}

// The range gate, as a square: about a hundred and sixty six units.
#define PASS_RANGE_SQUARED 27500.0f
#define SCORE_RANGE        250.0f
#define SCORE_FALLOFF      0.004f

// Pi, the plan bank's own copy; marked in src/ui/hudglobals.h.
extern const float g_flPiPlan;

// In a single player game a pass to the human team mate is never offered.
// FUNCTION: HYPERX 0x00466b70
void __cdecl PlanCond::RecvrInRangeNView(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg)
{
    const PlanWord param = { nArg };

    unsigned int nOperand;
    Vec3         vDelta;
    Vec3         vScratch;

    EngineObject* const pActor = pContext->GetActor();

    if (param.IsPronoun())
    {
        nOperand = g_apPlanStatFns[param.wPronoun](pContext->GetActor(), pArgs);
    }
    else
    {
        nOperand = param.n;
    }

    PlayerObject* const pTarget = g_pGameEngine->GetPlayer((short)nOperand);

    if ((nOperand & 0xffff) > SLOT_DROMER3)
    {
        // STRING: HYPERX 0x004bb348
        g_gameApp.Fatal("Fatal failure to initialize the receiver of a pass!");
    }

    const float flDamage = pTarget->m_stats.GetScalar(RATING_DAMAGE);

    const Vec3* pActorPos  = pContext->GetActor()->GetPositionPtr();
    const Vec3* pTargetPos = pTarget->GetPositionPtr();

    vDelta.x = pTargetPos->x - pActorPos->x;
    vDelta.y = pTargetPos->y - pActorPos->y;
    vDelta.z = pTargetPos->z - pActorPos->z;

    const float flEnable = pTarget->m_stats.GetScalar(RATING_LONG_PASS_ENABLE);

    if (flEnable != 0.0f)
    {
        *pResult = 0;
    }
    else if (Vec3::DotInline(&vDelta, &vDelta) < PASS_RANGE_SQUARED)
    {
        *pResult = 0;
    }
    else
    {
        *pResult = 1;
    }

    const short nTargetSlot = pTarget->GetSlot();
    const short nActorSlot  = pActor->GetSlot();

    EngineObject* const pTargetSlot = g_pGameEngine->m_ppSlots[nTargetSlot];

    vDelta = *pTargetSlot->GetPositionPtr()->Subtract(&vScratch,
                                                      g_pGameEngine->m_ppSlots[nActorSlot]->GetPositionPtr());

    const float flLength = Vec3::Length(&vDelta);

    const float flScale = g_flPiPlan / g_flThreePlan;

    const float flFalloff = (SCORE_RANGE - flLength) * flScale * SCORE_FALLOFF;

    const float flCos = (float)cos(flFalloff + flScale);

    if (pActor->GetDistIfFacing(pTarget, flCos) == 0.0f)
    {
        *pResult = 1;
    }

    const short nSlot = pContext->GetActor()->GetSlot();

    EngineObject* const pMate = g_pGameEngine->m_ppSlots[TeammateSlot(nSlot)];

    if (g_pGameEngine->m_nTeammateMode == 0)
    {
        // STRING: HYPERX 0x004bb340
        if (_mbscmp((const unsigned char*)"Human",
                    (const unsigned char*)pMate->GetPlanState()->m_pszName) == 0)
        {
            *pResult = 0;
        }
    }

    if (flDamage <= DamageGate())
    {
        *pResult = 1;
    }
}

#define MIDDLE_SLOT_BIAS 4

// Squared distances, so the limits of ten and just under eight read as a hundred and sixty.
// FUNCTION: HYPERX 0x00466e20
void __cdecl PlanCond::ChoosePassingPlay(Interactor* pContext, PlanNode*, short* pResult, int)
{
    float flSign;
    Vec3  vNear;
    Vec3  vFar;
    Vec3  vSpan;

    PlayerObject* const pActor = pContext->GetPlayerActor();

    const short nSlot = pActor->GetSlot();

    int nMiddle = nSlot;

    if (nSlot < MIDDLE_SLOT_BIAS)
    {
        nMiddle += 2;
    }

    const int nNearSlot = OPPOSING_FIRST(nSlot);

    GameEngine* const pEngine = g_pGameEngine;

    EngineObject* const pNear = pEngine->m_ppSlots[nNearSlot];

    const int nFarSlot = nNearSlot | 2;

    EngineObject* const pMiddle = pEngine->m_ppSlots[nMiddle & 3];
    EngineObject* const pFar    = pEngine->m_ppSlots[nFarSlot];

    if (pActor->m_wTargetGoalSlot == SLOT_NGOAL)
    {
        flSign = g_flOnePlan;
    }
    else
    {
        flSign = -g_flOnePlan;
    }

    const Vec3* pSelfNear = pActor->GetPositionPtr();
    const Vec3* pNearAt   = pNear->GetPositionPtr();

    vNear.x = pNearAt->x - pSelfNear->x;
    vNear.y = pNearAt->y - pSelfNear->y;
    vNear.z = pNearAt->z - pSelfNear->z;

    const Vec3* pSelfFar = pActor->GetPositionPtr();
    const Vec3* pFarAt   = pFar->GetPositionPtr();

    vFar.x = pFarAt->x - pSelfFar->x;
    vFar.y = pFarAt->y - pSelfFar->y;
    vFar.z = pFarAt->z - pSelfFar->z;

    const float flLimit = flSign * pActor->GetPositionPtr()->y + 20.0f;

    if (flLimit < flSign * pMiddle->GetPositionPtr()->y || vNear.z * vNear.z + vNear.y * vNear.y + vNear.x * vNear.x < 100.0 || vFar.z * vFar.z + vFar.y * vFar.y + vFar.x * vFar.x < 100.0)
    {
        const Vec3* pMidSpan = pMiddle->GetPositionPtr();
        const Vec3* pSpanAt  = pNear->GetPositionPtr();

        vSpan.x = pSpanAt->x - pMidSpan->x;
        vSpan.y = pSpanAt->y - pMidSpan->y;
        vSpan.z = pSpanAt->z - pMidSpan->z;

        const Vec3* pMidWide = pMiddle->GetPositionPtr();
        const Vec3* pWideAt  = pFar->GetPositionPtr();

        float       flWideX = pWideAt->x - pMidWide->x;
        const float flWideY = pWideAt->y - pMidWide->y;
        const float flWideZ = pWideAt->z - pMidWide->z;

        if (vSpan.z * vSpan.z + vSpan.y * vSpan.y + vSpan.x * vSpan.x > 60.0 && flWideX * flWideX + flWideY * flWideY + flWideZ * flWideZ > 60.0)
        {
            *pResult = 0;
        }
        else
        {
            *pResult = 1;
        }
    }
    else
    {
        *pResult = 1;
    }

    const float flDamage = ((PlayerObject*)pMiddle)->m_stats.GetScalar(RATING_DAMAGE);

    if (flDamage <= DamageGate())
    {
        *pResult = 1;
    }
}

// A missing model name falls back on the shared empty name, which never matches.

// FUNCTION: HYPERX 0x004670a0
void PlanCond::Ducking(Interactor* pContext, PlanNode*, short* pResult, int)
{
    const char* pszName;

    BoundBubble* const pColObj = g_pGameEngine->GetColObjPtr(pContext->GetModelSlot());

    const AnimDB* pSet = pColObj->GetOwner()->m_pAnimPlayer->m_pSet;

    if (pSet != 0)
    {
        pszName = pSet->GetName();
    }
    else
    {
        pszName = g_pszEmptyName;
    }

    // STRING: HYPERX 0x004c4664
    *pResult = _mbscmp((const unsigned char*)pszName, (const unsigned char*)"AA_QuikDuck") != 0;
}

// Two is added while the rok sits at its spawn slot.

// FUNCTION: HYPERX 0x00467110
void PlanCond::RokIsCharged(Interactor*, PlanNode*, short* pResult, int)
{
    RokObject* const pRok = g_pGameEngine->GetRok();

    const short nFlags = pRok->m_nKillState == -1;
    *pResult           = nFlags;

    if (pRok->m_bAtSpawnSlot != 0)
    {
        *pResult = (short)(nFlags + 2);
    }
}

// The collision object's kind is fetched and never read; only the rok's own word decides.

// FUNCTION: HYPERX 0x00467150
void PlanCond::RokIsChargedAgainst(Interactor* pContext, PlanNode*, short* pResult, int)
{
    RokObject* const pCarrier = g_pGameEngine->GetRok();

    if (pContext->GetNamedSlot() == -1)
    {
        BoundBubble* const pColObj = g_pGameEngine->GetColObjPtr(pContext->GetOtherSlot());

        pColObj->GetOwner()->GetSlot();
    }

    *pResult = pCarrier->m_nKillState == -1;
}

#define TURNSTILE_SPIN 10.0f
#define TURNSTILE_RATE 0.95f

// The spin is minus ten or ten by which side of the object's velocity the actor stands on.
// FUNCTION: HYPERX 0x004671c0
void __cdecl PlanAction::SpinTurnstile(Interactor* pContext, PlanNode*, short*, int nArg)
{
    const short nSlot = (short)nArg;

    Vec3  vDiff;
    Vec3  vImpulse;
    float flSpin;

    EngineObject* pObject = g_pGameEngine->m_ppSlots[nSlot];

    const Vec3* pVelocity  = &pObject->GetPhysicsBody()->vel;
    const Vec3* pDirection = &pObject->GetPhysicsBody()->velDir;

    vDiff.x = pObject->GetPositionPtr()->x - pContext->GetActor()->GetPositionPtr()->x;
    vDiff.y = pObject->GetPositionPtr()->y - pContext->GetActor()->GetPositionPtr()->y;
    vDiff.z = pObject->GetPositionPtr()->z - pContext->GetActor()->GetPositionPtr()->z;

    CRelaxYaw* const pRelax = (CRelaxYaw*)AllocAgendaItem(Agenda_CRelaxYaw);

    vImpulse.z = vDiff.y * pVelocity->x - vDiff.x * pVelocity->y;

    pRelax->Bind(pContext->GetActor(), 0);

    if (vImpulse.z > 0.0f)
    {
        flSpin = -TURNSTILE_SPIN;
    }
    else
    {
        flSpin = TURNSTILE_SPIN;
    }

    pRelax->m_flAngle = flSpin;
    pRelax->m_flDecay = TURNSTILE_RATE;

    pRelax->Activate();
}

// The actor is on the path when its squared distance from the slot's line of travel is under 2.2.

// The corridor's half width squared; marked in src/globals.cpp.
extern float g_flInterceptCorridorSq;

// FUNCTION: HYPERX 0x004672d0
void __cdecl PlanCond::SpinUp(Interactor* pContext, PlanNode*, short* pResult, int nArg)
{
    const short nSlot = (short)nArg;

    Vec3  vDelta;
    Vec3  vUnit;
    float flLength;

    EngineObject* pTarget = g_pGameEngine->m_ppSlots[nSlot];

    // The original has no guard here, and a run can reach it with an empty slot.
    if (pTarget == 0)
    {
        *pResult = 1;
        return;
    }

    pTarget->GetPhysicsBody();
    const Vec3* pForward = &pTarget->GetPhysicsBody()->velDir;

    EngineObject* pActor = pContext->GetActor();
    vDelta.x             = pTarget->GetPositionPtr()->x - pActor->GetPositionPtr()->x;

    pActor   = pContext->GetActor();
    vDelta.y = pTarget->GetPositionPtr()->y - pActor->GetPositionPtr()->y;

    pActor   = pContext->GetActor();
    vDelta.z = pTarget->GetPositionPtr()->z - pActor->GetPositionPtr()->z;

    flLength = (float)sqrt(vDelta.x * vDelta.x + vDelta.y * vDelta.y + vDelta.z * vDelta.z);

    const float flScale = 1.0f / flLength;

    vUnit.x = flScale * vDelta.x;
    vUnit.y = flScale * vDelta.y;
    vUnit.z = flScale * vDelta.z;

    const float flDot = vUnit.x * pForward->x + vUnit.y * pForward->y + vUnit.z * pForward->z;

    if ((1.0f - flDot * flDot) * flLength * flLength < g_flInterceptCorridorSq)
    {
        *pResult = 0;
        return;
    }

    *pResult = 1;
}

// Unwinds the last pitch through the staged sine, then applies the node's angle.

// The angle, in whole degrees.
#define ARG_DEGREES 1

#define DEG_TO_RAD 0.017453292f

// FUNCTION: HYPERX 0x00467410
void __cdecl PlanAction::ModifyPitch(Interactor* pContext, PlanNode* pNode, short*, int)
{
    EngineObject* pObject;

    if (pContext->GetNamedSlot() == -1)
    {
        BoundBubble* const pColObj = g_pGameEngine->GetColObjPtr(pContext->GetOtherSlot());

        pObject = pColObj->GetOwner();
    }
    else
    {
        pObject = pContext->GetActor();
    }

    float flAngle = -(float)asin(pObject->GetLocalTransformPtr()->m_rotX.flSin);

    TransformObject* pTransform = pObject->GetLocalTransformPtr();

    pTransform->m_rotX.flCos = (float)cos(flAngle);
    pTransform->m_rotX.flSin = (float)sin(flAngle);

    pTransform->RotateXStaged();

    flAngle = (float)(unsigned int)pNode->m_pArgs[ARG_DEGREES].n * DEG_TO_RAD;

    pTransform = pObject->GetLocalTransformPtr();

    pTransform->m_rotX.flCos = (float)cos(flAngle);
    pTransform->m_rotX.flSin = (float)sin(flAngle);

    pTransform->RotateXStaged();
}

// The two bungees differ only in range: fifty five for the first slot, forty five for the second.

#define ARG_FIRST_SLOT  1
#define ARG_SECOND_SLOT 2

#define DRAG_BUNGEE_GAIN         0.15f
#define DRAG_BUNGEE_REST_LENGTH  10.0f
#define DRAG_BUNGEE_FIRST_RANGE  55.0f
#define DRAG_BUNGEE_SECOND_RANGE 45.0f
#define DRAG_BUNGEE_DURATION     150

// FUNCTION: HYPERX 0x004674e0
void __cdecl PlanAction::HookDragBungeesOn(Interactor* pContext, PlanNode* pNode, short*, int)
{
    EngineObject* const pTarget = pContext->GetActor();

    PlanWord* const      pArgs     = pNode->m_pArgs;
    EngineObject** const apObjects = g_pGameEngine->m_ppSlots;

    EngineObject* const pFirst  = apObjects[(short)pArgs[ARG_FIRST_SLOT].n];
    EngineObject* const pSecond = apObjects[(short)pArgs[ARG_SECOND_SLOT].n];

    CBungeeToward* const pFirstBungee  = (CBungeeToward*)AllocAgendaItem(Agenda_CBungeeToward);
    CBungeeToward* const pSecondBungee = (CBungeeToward*)AllocAgendaItem(Agenda_CBungeeToward);

    PlanState* pBlock = pFirst->GetPlanState();
    pFirstBungee->Bind(pFirst, pBlock->m_pCurrent);

    pBlock = pSecond->GetPlanState();
    pSecondBungee->Bind(pSecond, pBlock->m_pCurrent);

    pFirstBungee->m_pTarget  = pTarget;
    pSecondBungee->m_pTarget = pTarget;

    pFirstBungee->m_flGain  = DRAG_BUNGEE_GAIN;
    pSecondBungee->m_flGain = DRAG_BUNGEE_GAIN;

    pFirstBungee->SetNearRange(g_flZeroPlan);

    pSecondBungee->SetNearRange(g_flZeroPlan);

    pFirstBungee->m_flRestLength  = DRAG_BUNGEE_REST_LENGTH;
    pSecondBungee->m_flRestLength = DRAG_BUNGEE_REST_LENGTH;

    pFirstBungee->m_flFarRange  = DRAG_BUNGEE_FIRST_RANGE;
    pSecondBungee->m_flFarRange = DRAG_BUNGEE_SECOND_RANGE;

    pFirstBungee->SetDotGate(g_flZeroPlan);

    pSecondBungee->SetDotGate(g_flZeroPlan);

    pFirstBungee->m_nCountdown  = DRAG_BUNGEE_DURATION;
    pSecondBungee->m_nCountdown = DRAG_BUNGEE_DURATION;

    pFirstBungee->Activate();
    pSecondBungee->Activate();
}

#define ARG_KIND_MASK 2

// No plan calls this; the field is the one every CTrackTraj range setup clears.
// FUNCTION: HYPERX 0x004675d0
void PlanAction::ResetAccTransient(Interactor* pContext, PlanNode*, short*, int)
{
    EngineObject* const pActor     = pContext->GetActor();
    PlanState* const    pPlanState = pActor->GetPlanState();

    static_cast<CTrackTraj*>(pPlanState->m_pCurrent->m_apSlots[0])->m_flBlend = g_flZeroPlan;
}

// Every input device is written as a KeyboardDeviceObj, so on the 0xd28-byte gamepad, joystick and mouse the
// mask lands past the object, as the original's writes at 0x0046763a and 0x00467649 do (allocated at 0x004863f0).
// FUNCTION: HYPERX 0x004675f0
void __cdecl PlanAction::ResetKeystateFlags(Interactor* pContext, PlanNode* pNode, short*, int)
{
    EngineObject* const pOwner = pContext->GetActor();

    if (pOwner->m_nKind == KIND_KEYBOARD || pOwner->m_nKind == KIND_GAMEPAD || pOwner->m_nKind == KIND_JOYSTICK || pOwner->m_nKind == KIND_MOUSE)
    {
        if (pNode->m_nArgCount == ARG_KIND_MASK)
        {
            EngineObject* pTarget;
            unsigned int  nMask;
            unsigned int  nActions;

            nMask = (unsigned int)pNode->m_pArgs[1].n;

            pTarget = pContext->GetActor();

            nActions = ((KeyboardDeviceObj*)pTarget)->m_nHeldMask;

            ((KeyboardDeviceObj*)pTarget)->m_nHeldMask = nActions & ~nMask;
        }
        else
        {
            ((KeyboardDeviceObj*)pContext->GetActor())->m_nHeldMask = 0;
        }
    }

    if (pOwner->m_nKind == KIND_DROMER)
    {
        pContext->GetActor()->GetPhysicsBody()->nHeldBindings = 0;
        pContext->GetActor()->GetPhysicsBody()->axis.z        = g_flZeroPlan;
        pContext->GetActor()->GetPhysicsBody()->axis.y        = g_flZeroPlan;
        pContext->GetActor()->GetPhysicsBody()->axis.x        = g_flZeroPlan;
        pContext->GetActor()->GetPhysicsBody()->flSteerScale  = g_flOnePlan;
    }
}

// Set, the rate runs from 1.05 to 1.45 across the damage range; clear, from one to six.

#define ARG_CURVE 1

// FUNCTION: HYPERX 0x004676e0
void PlanAction::IncrAnimSpeedWRating(Interactor* pContext, PlanNode* pNode, short*, int)
{
    float flDamage;
    float flScale;

    const int nCurve = pNode->m_pArgs[ARG_CURVE].n;

    PlayerObject* const pActor = pContext->GetPlayerActor();

    flDamage = pActor->m_stats.GetScalar(RATING_DAMAGE) * 0.01f;

    if (nCurve != 0)
    {
        flScale = flDamage * 0.4f + 1.05f;
    }
    else
    {
        flScale = flDamage * 5.0f + 1.0f;
    }

    pContext->GetActor()->m_pAnimPlayer->SetPlaybackRate(
        pContext->GetActor()->m_pAnimPlayer->GetPlaybackRate() * flScale);
}

// The winning and losing sides read different entries of the speedburst rating pair.

#define ARG_RATING_BASE 1
#undef ARG_PERIOD
#define ARG_PERIOD 2

// FUNCTION: HYPERX 0x00467760
void __cdecl PlanAction::SpeedBurstSituation(Interactor* pContext, PlanNode* pNode, short*, int)
{
    float flScale;

    if (pContext->GetPlayerActor()->m_stats.m_flSpeedRatio >= 0.25f)
    {
        int nRating = pNode->m_pArgs[ARG_RATING_BASE].n;

        if (GameEngine::IsTeamWinning((unsigned char)pContext->GetActor()->GetSlot()) != 0)
        {
            nRating *= 2;
        }
        else
        {
            nRating = nRating * 2 + 1;
        }

        if ((int)pContext->GetPlayerActor()->m_stats.GetScalar(nRating + RATING_SPEEDBURST0A) != 0)
        {
            CSpeedBurst* const pBurst = (CSpeedBurst*)AllocAgendaItem(Agenda_CSpeedBurst);

            if (pContext->GetPlayerActor()->m_stats.m_flSpeedRatio > 0.3)
            {
                flScale = g_flOnePlan;
            }
            else
            {
                flScale = g_flZeroPlan;
            }

            GameBehaviorObject* const pCurrentBehaviour = pContext->GetActor()->GetPlanState()->m_pCurrent;

            pBurst->Bind(pContext->GetActor(), pCurrentBehaviour);
            pBurst->Activate();

            pBurst->m_flTarget =
                (float)(80.0 / (unsigned int)pNode->m_pArgs[ARG_PERIOD].n * flScale + g_flOnePlan);

            *pBurst->m_pBoost = 1.05f;

            PhysicsBody* pFirst = pContext->GetActor()->GetPhysicsBody();

            Vec3* const pSpin  = &pFirst->axis;
            const float flSpin = pSpin->x + 0.25f;

            pSpin->x = flSpin;

            pFirst = pContext->GetActor()->GetPhysicsBody();

            pFirst->flFriction = 0.1f;

            pFirst = pContext->GetActor()->GetPhysicsBody();

            Vec3* const pVel = &pFirst->vel;

            const float flLen = (float)sqrt(pVel->x * pVel->x + pVel->y * pVel->y + pVel->z * pVel->z);

            if (flLen < 0.1)
            {
                pFirst                     = pContext->GetActor()->GetPhysicsBody();
                PhysicsBody* const pSecond = pContext->GetActor()->GetPhysicsBody();

                pSecond->vel = pFirst->velDir;
            }
        }
    }
}

// Eight hundred, the ceiling both squared-distance thresholds are clamped to.
// GLOBAL: HYPERX 0x004b378c
extern const float g_flRangeLimit;

#define RATINGS_RANGE 800.0f

#define ONE_IN_CHANCE 8

// One roll in eight inverts the answer, and a local match never attacks around a human team mate.
// FUNCTION: HYPERX 0x00467930
void __cdecl PlanCond::PassOrRunByRatings(Interactor* pContext, PlanNode*, short* pResult, int)
{
    float flSign;
    Vec3  vNear;
    Vec3  vFar;
    Vec3  vMateNear;
    Vec3  vMateFar;

    PlayerObject* const pOwner = pContext->GetPlayerActor();

    const short nSlot = pOwner->GetSlot();

    const int nMate = TeammateSlot(nSlot);

    EngineObject** const apObjects = g_pGameEngine->m_ppSlots;

    EngineObject* const pNear = apObjects[OPPOSING_FIRST(nSlot)];

    EngineObject* const pMate = apObjects[nMate];

    EngineObject* const pFar = apObjects[OPPOSING_SECOND(nSlot)];

    if (pOwner->m_wTargetGoalSlot == SLOT_NGOAL)
    {
        flSign = g_flOnePlan;
    }
    else
    {
        flSign = -g_flOnePlan;
    }

    const Vec3* const pNearFrom = pOwner->GetPositionPtr();
    const Vec3* const pNearTo   = pNear->GetPositionPtr();

    vNear.x = pNearTo->x - pNearFrom->x;
    vNear.y = pNearTo->y - pNearFrom->y;
    vNear.z = pNearTo->z - pNearFrom->z;

    const Vec3* const pFarFrom = pOwner->GetPositionPtr();
    const Vec3* const pFarTo   = pFar->GetPositionPtr();

    vFar.x = pFarTo->x - pFarFrom->x;
    vFar.y = pFarTo->y - pFarFrom->y;
    vFar.z = pFarTo->z - pFarFrom->z;

    const Vec3* const pMateNearFrom = pMate->GetPositionPtr();
    const Vec3* const pMateNearTo   = pNear->GetPositionPtr();

    vMateNear.x = pMateNearTo->x - pMateNearFrom->x;
    vMateNear.y = pMateNearTo->y - pMateNearFrom->y;
    vMateNear.z = pMateNearTo->z - pMateNearFrom->z;

    const Vec3* const pMateFarFrom = pMate->GetPositionPtr();
    const Vec3* const pMateFarTo   = pFar->GetPositionPtr();

    vMateFar.x = pMateFarTo->x - pMateFarFrom->x;
    vMateFar.y = pMateFarTo->y - pMateFarFrom->y;
    vMateFar.z = pMateFarTo->z - pMateFarFrom->z;

    PlayerObject* pBody = pContext->GetPlayerActor();

    float flLead = pBody->m_stats.GetScalar(RATING_PASS0);

    pBody = pContext->GetPlayerActor();

    float flOutside = pBody->m_stats.GetScalar(RATING_PASS1);

    pBody = pContext->GetPlayerActor();

    float flInside = pBody->m_stats.GetScalar(RATING_PASS2);

    if (flLead < 0.0f)
    {
        flLead = 0.0f;
    }

    if (flOutside > g_flRangeLimit)
    {
        flOutside = RATINGS_RANGE;
    }

    if (flInside > g_flRangeLimit)
    {
        flInside = RATINGS_RANGE;
    }

    const float flLimit = flSign * pOwner->GetPositionPtr()->y + flLead;

    if ((flSign * pMate->GetPositionPtr()->y <= flLimit && vNear.z * vNear.z + vNear.y * vNear.y + vNear.x * vNear.x >= flOutside && vFar.z * vFar.z + vFar.y * vFar.y + vFar.x * vFar.x >= flOutside) || vMateNear.z * vMateNear.z + vMateNear.y * vMateNear.y + vMateNear.x * vMateNear.x <= flInside || vMateFar.z * vMateFar.z + vMateFar.y * vMateFar.y + vMateFar.x * vMateFar.x <= flInside)
    {
        *pResult = 1;
    }
    else
    {
        *pResult = 0;
    }

    if (rand() % ONE_IN_CHANCE == 0)
    {
        *pResult = !*pResult;
    }

    if (g_pGameEngine->m_nTeammateMode == 0)
    {
        // STRING: HYPERX 0x004bb340
        if (_mbscmp((const unsigned char*)"Human",
                    (const unsigned char*)pMate->GetPlanState()->m_pszName) == 0)
        {
            *pResult = 0;
        }
    }
}

// Anything outside zero to three becomes zero, the widest a four-child DECIDE can be.

#define BRANCH_MAX 3

// FUNCTION: HYPERX 0x00467c30
void __cdecl PlanCond::LooseRokWhatToDo(Interactor* pContext, PlanNode*, short* pResult, int)
{
    PlayerObject* pOwner;
    const Vec3*   pRokAt;
    const Vec3*   pOppAt;
    Vec3          vOpp;
    float         flStat;

    pRokAt = g_pGameEngine->m_ppSlots[SLOT_ROK]->GetPositionPtr();

    const Vec3* pOwnerAt = pContext->GetActor()->GetPositionPtr();

    const short nSlot = pContext->GetActor()->GetSlot();

    pOppAt = g_pGameEngine->m_ppSlots[TeammateSlot(nSlot)]->GetPositionPtr();

    const float flOwnX = pRokAt->x - pOwnerAt->x;
    const float flOwnY = pRokAt->y - pOwnerAt->y;
    const float flOwnZ = pRokAt->z - pOwnerAt->z;

    vOpp.x = pRokAt->x - pOppAt->x;
    vOpp.y = pRokAt->y - pOppAt->y;
    vOpp.z = pRokAt->z - pOppAt->z;

    if (flOwnX * flOwnX + flOwnY * flOwnY + flOwnZ * flOwnZ < vOpp.x * vOpp.x + vOpp.y * vOpp.y + vOpp.z * vOpp.z)
    {
        if (GameEngine::IsTeamWinning((unsigned char)pContext->GetActor()->GetSlot()) != 0)
        {
            pOwner = pContext->GetPlayerActor();

            flStat = pOwner->m_stats.GetScalar(RATING_WIN_CLOSE_FREE_ROK);
        }
        else
        {
            pOwner = pContext->GetPlayerActor();

            flStat = pOwner->m_stats.GetScalar(RATING_LOSE_CLOSE_FREE_ROK);
        }
    }
    else
    {
        if (GameEngine::IsTeamWinning((unsigned char)pContext->GetActor()->GetSlot()) != 0)
        {
            pOwner = pContext->GetPlayerActor();

            flStat = pOwner->m_stats.GetScalar(RATING_WIN_FAR_FREE_ROK);
        }
        else
        {
            pOwner = pContext->GetPlayerActor();

            flStat = pOwner->m_stats.GetScalar(RATING_LOSE_FAR_FREE_ROK);
        }
    }

    const int nStat = (int)flStat;

    if (nStat < 0 || nStat > BRANCH_MAX)
    {
        *pResult = 0;
    }
    else
    {
        *pResult = (unsigned short)nStat;
    }

    PlayerObject* pOpponent = g_pGameEngine->GetPlayer(OPPOSING_FIRST(pContext->GetActor()->GetSlot()));

    const float flDamageA = pOpponent->m_stats.GetScalar(RATING_DAMAGE);

    pOpponent = g_pGameEngine->GetPlayer(OPPOSING_SECOND(pContext->GetActor()->GetSlot()));

    const float flDamageB = pOpponent->m_stats.GetScalar(RATING_DAMAGE);

    if (flDamageA <= DamageGate())
    {
        if (flDamageB <= DamageGate())
        {
            *pResult = 0;
        }
    }
}

#define PERCENT_SCALE 10.0f
#define PERCENT_RANGE 100

// The figure is halved before the roll, so a perfect actor is capped at half the die.
// FUNCTION: HYPERX 0x00467e80
void __cdecl PlanCond::JostledDropRok(Interactor* pContext, PlanNode*, short* pnResult, int)
{
    PlayerObject* const pActor = pContext->GetPlayerActor();

    const float flBase   = pActor->m_stats.GetWeightedStat(RATING_ROK_HANDLING) * PERCENT_SCALE;
    const float flDamage = pActor->m_stats.GetScalar(RATING_DAMAGE);

    const unsigned int nRoll = rand() % PERCENT_RANGE;

    const float flOdds = (flDamage * flBase) * 0.01f;

    if ((float)nRoll > (flOdds + flBase) * 0.5f)
    {
        *pnResult = 1;
        return;
    }

    *pnResult = 0;
}

// While the coach's first condition flag is set the threshold is four, so the answer is zero.

// The coach's thirteen condition answers, written once a frame by EngineObject::UpdateCoachAI.
// GLOBAL: HYPERX 0x004cb70c
extern int g_anCoachConditions[13];

// The delta the guard rejects, and the threshold the coach flag forces.
#define TRACK_DELTA_LIMIT 4

// The Multiplier rating for the team ahead or behind; a value past four either way is two.
#define TRACK_RATING_LIMIT   4
#define TRACK_RATING_DEFAULT 2

// FUNCTION: HYPERX 0x00467f10
void __cdecl PlanCond::GoThroughMuliplier(Interactor* pContext, PlanNode*, short* pResult, int)
{
    PlayerObject* pOwner;
    short         nSlot;
    short         nThreshold;

    RokObject* const pRok = g_pGameEngine->GetRok();

    if (pRok->m_bAtSpawnSlot != 0)
    {
        *pResult = 1;

        return;
    }

    short nDelta = pRok->m_wRokValue;

    AttachPak* const pCarrier = pRok->GetPhysicsBody()->pAttach;

    if (pCarrier != 0)
    {
        nSlot = pCarrier->nSlot;

        if (nSlot < PLAYER_OBJECT_COUNT)
        {
            if ((nSlot & 1) != 0)
            {
                nDelta = -nDelta;
            }

            if (nDelta >= TRACK_DELTA_LIMIT)
            {
                *pResult = 1;

                return;
            }

            EngineObject* const pActor = pContext->GetActor();

            if (GameEngine::IsTeamWinning((unsigned char)pActor->GetSlot()) != 0)
            {
                pOwner = pContext->GetPlayerActor();

                nThreshold = (short)(&pOwner->m_stats)->GetScalar(RATING_MULTIPLIER0);
            }
            else
            {
                pOwner = pContext->GetPlayerActor();

                nThreshold = (short)(&pOwner->m_stats)->GetScalar(RATING_MULTIPLIER1);
            }

            if (g_anCoachConditions[0] != 0)
            {
                nThreshold = TRACK_DELTA_LIMIT;
            }
            else if (nThreshold < -TRACK_RATING_LIMIT || nThreshold > TRACK_RATING_LIMIT)
            {
                nThreshold = TRACK_RATING_DEFAULT;
            }

            *pResult = nDelta >= nThreshold;

            return;
        }
    }

    *pResult = 1;
}

#define ROLL_STEPS 5

#define OPTION_ATTACK_MODE_ON 1

// The answer is one past an ID_ATK attack; nought deploys the Rok, mine or swarf the actor carries.
// FUNCTION: HYPERX 0x00468000
void __cdecl PlanCond::PickAttackMove(Interactor* pContext, PlanNode*, short* pResult, int)
{
    PhysicsBody* pWeaponBody;

    Vec3  vOffset;
    int   nSide;
    short nSlot;

    int nMove = 0;

    int nRoll = rand() % PERCENT_RANGE;

    PlayerObject* pActor = pContext->GetPlayerActor();

    int nAttackerSlot = pActor->GetSlot();

    if (nAttackerSlot > SLOT_DROMER3)
    {
        nAttackerSlot = g_pGameEngine->m_nViewedSlot;

        nSlot = (short)nAttackerSlot;

        pActor = g_pGameEngine->GetPlayer(nSlot);

        if (g_pGameEngine->m_nAttackMode == OPTION_ATTACK_MODE_ON)
        {
            *pResult = ATTACK_PUNCH + 1;

            return;
        }
    }

    *pResult = ATTACK_PUNCH + 1;

    EngineObject* pWeapon = g_pGameEngine->m_ppSlots[SLOT_ROK];

    if (pWeapon != 0 && ((RokObject*)pWeapon)->m_nKillState > -1)
    {
        pWeaponBody = pWeapon->GetPhysicsBody();

        if (pWeaponBody->pAttach != 0 && pWeaponBody->pAttach->nSlot < PLAYER_OBJECT_COUNT && pWeaponBody->pAttach->nSlot == nAttackerSlot)
        {
            *pResult = 0;

            return;
        }
    }

    pWeapon = g_pGameEngine->m_ppSlots[SLOT_MINE];

    if (pWeapon != 0)
    {
        pWeaponBody = pWeapon->GetPhysicsBody();

        if (pWeaponBody->pAttach != 0 && pWeaponBody->pAttach->nSlot < PLAYER_OBJECT_COUNT && pWeaponBody->pAttach->nSlot == nAttackerSlot)
        {
            *pResult = 0;

            return;
        }
    }

    pWeapon = g_pGameEngine->m_ppSlots[SLOT_SWARF];

    if (pWeapon != 0)
    {
        pWeaponBody = pWeapon->GetPhysicsBody();

        if (pWeaponBody->pAttach != 0 && pWeaponBody->pAttach->nSlot < PLAYER_OBJECT_COUNT && pWeaponBody->pAttach->nSlot == nAttackerSlot)
        {
            *pResult = 0;

            return;
        }
    }

    ObjectStats* const pStats = &pActor->m_stats;

    int nWeight = (int)pStats->GetScalar(RATING_FIGHTING0);

    if (nRoll > nWeight)
    {
        while (nRoll > nWeight)
        {
            nMove++;
            nRoll -= nWeight;

            nWeight = (int)pStats->GetScalar(nMove + RATING_FIGHTING0);

            if (nMove > ROLL_STEPS - 1)
            {
                *pResult = ATTACK_PUNCH + 1;

                return;
            }
        }
    }

    const Vec3* pSelf = pActor->GetPositionPtr();

    unsigned char bySlot = (unsigned char)pActor->GetSlot();

    const Vec3* pFar = g_pGameEngine->m_ppSlots[OPPOSING_FIRST(bySlot)]->GetPositionPtr();

    bySlot = (unsigned char)pActor->GetSlot();

    const Vec3* pNear = g_pGameEngine->m_ppSlots[OPPOSING_SECOND(bySlot)]->GetPositionPtr();

    const float flFarX = pSelf->x - pFar->x;
    const float flFarY = pSelf->y - pFar->y;
    const float flFarZ = pSelf->z - pFar->z;

    const float flNearX = pSelf->x - pNear->x;
    const float flNearY = pSelf->y - pNear->y;
    vOffset.z           = pSelf->z - pNear->z;

    const float flNearSq = flNearX * flNearX + flNearY * flNearY + vOffset.z * vOffset.z;

    const float flFarSq = flFarX * flFarX + flFarY * flFarY + flFarZ * flFarZ;

    if (flFarSq < flNearSq)
    {
        vOffset.x = pFar->x - pSelf->x;
        vOffset.y = pFar->y - pSelf->y;
    }
    else
    {
        vOffset.x = pNear->x - pSelf->x;
        vOffset.y = pNear->y - pSelf->y;
    }

    const Vec3* pVel = &pActor->GetPhysicsBody()->velDir;

    if (vOffset.y * pVel->x - vOffset.x * pVel->y > 0.0f)
    {
        nSide = 0;
    }
    else
    {
        nSide = 1;
    }

    if (nMove == 0)
    {
        *pResult = ATTACK_PUNCH + 1;

        return;
    }

    if (nMove == 1)
    {
        *pResult = ATTACK_HIP_CHECK + 1;

        return;
    }

    if (nMove == 2)
    {
        *pResult = (short)(nSide + ATTACK_CLOTHES_LINE_L + 1);

        return;
    }

    if (nMove == 3)
    {
        *pResult = (short)(nSide + ATTACK_SHOULDER_CHECK_L + 1);

        return;
    }

    *pResult = (short)(nSide + ATTACK_LEG_SWEEP_L + 1);
}

// Both opponents hurt turn an index of one into two; zero and two stand.

#define OFF_WING_JOB_MAX 2

// FUNCTION: HYPERX 0x004682d0
void __cdecl PlanCond::OffWingJob(Interactor* pContext, PlanNode*, short* pResult, int)
{
    PlayerObject* pOwner;
    int           nState;

    if (GameEngine::IsTeamWinning((unsigned char)pContext->GetActor()->GetSlot()) != 0)
    {
        pOwner = pContext->GetPlayerActor();

        nState = (int)(&pOwner->m_stats)->GetScalar(RATING_WIN_OFF_WING_JOB);
    }
    else
    {
        pOwner = pContext->GetPlayerActor();

        nState = (int)(&pOwner->m_stats)->GetScalar(RATING_LOSE_OFF_WING_JOB);
    }

    if (nState < 0 || nState > OFF_WING_JOB_MAX)
    {
        nState = 0;
    }

    *pResult = (short)nState;

    PlayerObject* pOpponent = g_pGameEngine->GetPlayer(OPPOSING_FIRST(pContext->GetActor()->GetSlot()));

    const float flDamageA = pOpponent->m_stats.GetScalar(RATING_DAMAGE);

    pOpponent = g_pGameEngine->GetPlayer(OPPOSING_SECOND(pContext->GetActor()->GetSlot()));

    const float flDamageB = pOpponent->m_stats.GetScalar(RATING_DAMAGE);

    if (*pResult == 1)
    {
        if (flDamageA <= DamageGate())
        {
            if (flDamageB <= DamageGate())
            {
                *pResult = 2;
            }
        }
    }
}

// Rolls over half the range while the owner is below fifty units a second.

// FUNCTION: HYPERX 0x00468440
void __cdecl PlanCond::PickFlyingMove(Interactor* pContext, PlanNode* pArgs, short* pResult, int)
{
    Vec3 vVelocity = pContext->GetActor()->m_pBody->vel;

    const int nRange = pArgs->m_nArgCount - 1;

    if ((float)(sqrt(Vec3::DotInline(&vVelocity, &vVelocity)) * g_pGameEngine->m_clock.flInvDelta) < 50.0f)
    {
        *pResult = (unsigned short)(rand() % (nRange / 2));
    }
    else
    {
        *pResult = (unsigned short)(rand() % nRange);
    }
}

// Picks one of the set bits of the seven-bit RATING_HALFPIPE_REPERTOIRE mask, with equal weight.

// FUNCTION: HYPERX 0x004684e0
void PlanCond::PickHalfpipeMove(Interactor* pContext, PlanNode*, short* pResult, int)
{
    int nBit   = 0;
    int nCount = 0;

    PlayerObject* const pObject = pContext->GetPlayerActor();

    int nMask = (int)pObject->m_stats.GetScalar(RATING_HALFPIPE_REPERTOIRE);

    if (nMask < 1 || nMask > 0x7f)
    {
        *pResult = (short)(rand() % 7);
        return;
    }

    if ((nMask & 0x01) != 0)
    {
        nCount++;
    }

    if ((nMask & 0x02) != 0)
    {
        nCount++;
    }

    if ((nMask & 0x04) != 0)
    {
        nCount++;
    }

    if ((nMask & 0x08) != 0)
    {
        nCount++;
    }

    if ((nMask & 0x10) != 0)
    {
        nCount++;
    }

    if ((nMask & 0x20) != 0)
    {
        nCount++;
    }

    if ((nMask & 0x40) != 0)
    {
        nCount++;
    }

    const int nRemainder = rand() % nCount;

    // Steps to the next set bit of the mask once for each of nought to the remainder.
    for (int nStep = -1; nStep < nRemainder; nStep++)
    {
        if (nBit == 0)
        {
            nBit = 1;
        }
        else
        {
            nBit <<= 1;
        }

        while ((nBit & nMask) == 0)
        {
            nBit <<= 1;
        }
    }

    if (nBit == 1)
    {
        *pResult = 0;
        return;
    }

    if (nBit == 2)
    {
        *pResult = 1;
        return;
    }

    if (nBit == 4)
    {
        *pResult = 2;
        return;
    }

    if (nBit == 8)
    {
        *pResult = 3;
        return;
    }

    if (nBit == 0x10)
    {
        *pResult = 4;
        return;
    }

    if (nBit == 0x20)
    {
        *pResult = 5;
        return;
    }

    *pResult = 6;
}

// FUNCTION: HYPERX 0x00468610
void PlanCond::AvoidSpinningTurnstile(Interactor* pContext, PlanNode*, short* pResult, int)
{
    PlayerObject* const pActor = pContext->GetPlayerActor();

    *pResult = (int)pActor->m_stats.GetScalar(RATING_TURNSTILE0) == 0;
}

// FUNCTION: HYPERX 0x00468640
void PlanCond::DecideNever(Interactor*, PlanNode*, short* pResult, int)
{
    *pResult = 0;
}

// The window an authored range has to fall in, and what replaces it when it does not.
#define RANGE_MIN     10
#define RANGE_MAX     350
#define RANGE_DEFAULT 150

// An authored range outside ten to 350 becomes 150; a local match never shoots past a human mate.
// FUNCTION: HYPERX 0x00468650
void __cdecl PlanCond::ShootSituation(Interactor* pContext, PlanNode*, short* pResult, int)
{
    EngineObject* pGoal;
    Vec3          vDiff;
    int           nRange;
    PlayerObject* pBody = pContext->GetPlayerActor();

    nRange = (int)(&pBody->m_stats)->GetScalar(RATING_CARRY_ROK_THROW_DIST);

    if (nRange < RANGE_MIN || nRange > RANGE_MAX)
    {
        nRange = RANGE_DEFAULT;
    }

    const Vec3* pSelfAt = pContext->GetActor()->GetPositionPtr();

    const short nOwnSlot = pContext->GetActor()->GetSlot();

    EngineObject** const apObjects = g_pGameEngine->m_ppSlots;

    if ((nOwnSlot & 1) != 0)
    {
        pGoal = apObjects[SLOT_NGOAL];
    }
    else
    {
        pGoal = apObjects[SLOT_SGOAL];
    }

    const Vec3* pGoalAt = pGoal->GetPositionPtr();

    vDiff.x = pGoalAt->x - pSelfAt->x;
    vDiff.y = pGoalAt->y - pSelfAt->y;
    vDiff.z = pGoalAt->z - pSelfAt->z;

    const float flDistance = vDiff.Magnitude();

    Vec3 vAxis = pContext->GetActor()->GetPhysicsBody()->velDir;

    float flRange = (float)nRange;

    Vec3 vDir = vDiff;

    if (flDistance <= flRange)
    {
        Math::Vec3Normalize(&vAxis);
        Math::Vec3Normalize(&vDir);

        if (Vec3::DotInline(&vDir, &vAxis) > 0.5f)
        {
            flRange *= 0.5f;

            const float flRoll = ((float)rand() * (1.0f / RAND_MAX)) * flRange;

            if (flRoll + flRange > flDistance)
            {
                *pResult = 0;
            }
            else
            {
                *pResult = 1;
            }
        }
        else
        {
            *pResult = 1;
        }
    }
    else
    {
        *pResult = 1;
    }

    const short nSlot = pContext->GetActor()->GetSlot();

    EngineObject* const pMate = g_pGameEngine->m_ppSlots[TeammateSlot(nSlot)];

    if (g_pGameEngine->m_nTeammateMode == 0)
    {
        // STRING: HYPERX 0x004bb340
        if (_mbscmp((const unsigned char*)"Human",
                    (const unsigned char*)pMate->GetPlanState()->m_pszName) == 0)
        {
            *pResult = 1;
        }
    }
}

// Every value it fetches is discarded and the answer is always zero; the calls are the point.
// FUNCTION: HYPERX 0x00468830
void __cdecl PlanCond::RegroupSituation(Interactor* pContext, PlanNode*, short* pResult, int)
{
    PlayerObject* pOwner;

    EngineObject* pActor = pContext->GetActor();

    if (GameEngine::IsTeamWinning((unsigned char)pActor->GetSlot()) != 0)
    {
        pOwner = pContext->GetPlayerActor();

        pOwner->m_stats.GetScalar(RATING_WIN_CARRY_ROK_REGROUP_DIST);
    }
    else
    {
        pOwner = pContext->GetPlayerActor();

        pOwner->m_stats.GetScalar(RATING_LOSE_CARRY_ROK_REGROUP_DIST);
    }

    pContext->GetActor()->GetPositionPtr();

    pActor = pContext->GetActor();

    if ((pActor->GetSlot() & 1) != 0)
    {
        g_pGameEngine->m_ppSlots[SLOT_SGOAL]->GetPositionPtr();

        *pResult = 0;

        return;
    }

    g_pGameEngine->m_ppSlots[SLOT_NGOAL]->GetPositionPtr();

    *pResult = 0;
}

#define AGGRESSION_RADIUS_MAX 400

// The radius is compared against a squared distance without being squared itself.
// FUNCTION: HYPERX 0x004688d0
void __cdecl PlanCond::AggressionOnOthers(Interactor* pContext, PlanNode*, short* pResult, int)
{
    Vec3*         pSecond;
    int           nLimit;
    PlayerObject* pOwner;
    Vec3          vB;

    EngineObject* pActor = pContext->GetActor();

    if (GameEngine::IsTeamWinning((unsigned char)pActor->GetSlot()) != 0)
    {
        pOwner = pContext->GetPlayerActor();

        nLimit = (int)(&pOwner->m_stats)->GetScalar(RATING_WIN_AGGRESSION);
    }
    else
    {
        pOwner = pContext->GetPlayerActor();

        nLimit = (int)(&pOwner->m_stats)->GetScalar(RATING_LOSE_AGGRESSION);
    }

    if (nLimit < 0 || nLimit > AGGRESSION_RADIUS_MAX)
    {
        nLimit = AGGRESSION_RADIUS_MAX;
    }

    Vec3* const pSelf = pContext->GetActor()->GetPositionPtr();

    pActor               = pContext->GetActor();
    unsigned char byTeam = (unsigned char)pActor->GetSlot();
    Vec3* const   pFirst = g_pGameEngine->m_ppSlots[OPPOSING_FIRST(byTeam)]->GetPositionPtr();

    pActor  = pContext->GetActor();
    byTeam  = (unsigned char)pActor->GetSlot();
    pSecond = g_pGameEngine->m_ppSlots[OPPOSING_SECOND(byTeam)]->GetPositionPtr();

    const float dxA = pSelf->x - pFirst->x;
    const float dyA = pSelf->y - pFirst->y;
    const float dzA = pSelf->z - pFirst->z;

    vB.x = pSelf->x - pSecond->x;
    vB.y = pSelf->y - pSecond->y;
    vB.z = pSelf->z - pSecond->z;

    const float flToFirst  = dxA * dxA + dyA * dyA + dzA * dzA;
    const float flToSecond = vB.x * vB.x + vB.y * vB.y + vB.z * vB.z;

    if (flToFirst < flToSecond)
    {
        if (flToFirst < nLimit)
        {
            *pResult = 0;

            return;
        }

        *pResult = 1;

        return;
    }

    if (flToSecond < nLimit)
    {
        *pResult = 0;

        return;
    }

    *pResult = 1;
}

// FUNCTION: HYPERX 0x00468a60
void PlanAction::SetShotPowerMeter(Interactor*, PlanNode* pArgs, short*, int)
{
    if (pArgs->m_nArgCount > 1 && pArgs->m_pArgs[1].n == 1)
    {
        g_pGameEngine->GetCamera(CAMERA_HEAD_SLOT)->m_bShotPowerShown = 1;
    }
    else
    {
        g_pGameEngine->GetCamera(CAMERA_HEAD_SLOT)->m_bShotPowerShown = 0;
    }
}

// FUNCTION: HYPERX 0x00468aa0
void __cdecl PlanCond::NearObjectBy(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg)
{
    const short nSlot = (short)nArg;

    const int nDistance = pArgs->m_pArgs[1].pNode->m_pArgs[0].n;

    Vec3* const pSelf  = pContext->GetActor()->GetPositionPtr();
    Vec3* const pOther = g_pGameEngine->m_ppSlots[nSlot]->GetPositionPtr();

    // The distance stays squared: the plan argument is already a square.
    const float dx = pSelf->x - pOther->x;
    const float dy = pSelf->y - pOther->y;
    const float dz = pSelf->z - pOther->z;

    if (dx * dx + dy * dy + dz * dz <= nDistance)
    {
        *pResult = 1;

        return;
    }

    *pResult = 2;
}

// FUNCTION: HYPERX 0x00468b30
void PlanCond::OkToGetRok(Interactor* pContext, PlanNode*, short* pResult, int nParam)
{
    pContext->GetTargetSlot();

    if (nParam == SLOT_WILDCARD)
    {
        *pResult = 1;

        return;
    }

    EngineObject* const pRok = g_pGameEngine->m_ppSlots[SLOT_ROK];

    if ((short)((RokObject*)pRok)->m_nCarrierSlot != (short)nParam)
    {
        *pResult = pRok->GetPhysicsBody()->pAttach == 0;

        return;
    }

    *pResult = 0;
}

// The Rok's carrier slot must name one of the four skaters; anything else answers yes.

// FUNCTION: HYPERX 0x00468b90
void PlanCond::CheckScoreOnOwnGoal(Interactor* pContext, PlanNode*, short* pResult, int)
{
    EngineObject* const pActor = pContext->GetActor();

    EngineObject** const ppObjects = g_pGameEngine->m_ppSlots;

    RokObject* const pRok = (RokObject*)ppObjects[SLOT_ROK];

    int nTracked = pRok->m_nCarrierSlot;

    if (nTracked >= 0 && nTracked <= 3)
    {
        const short nGoal = ((PlayerObject*)ppObjects[(short)nTracked])->m_wTargetGoalSlot;

        *pResult = nGoal != pActor->GetSlot();
    }
    else
    {
        *pResult = 1;
    }
}

// FUNCTION: HYPERX 0x00468bf0
void PlanCond::Interception(Interactor* pContext, PlanNode*, short* pResult, int)
{
    pContext->GetTargetSlot();

    RokObject* const pRok = g_pGameEngine->GetRok();

    *pResult = pRok->m_nCarrierSlot % 2 != pRok->m_nPrevCarrierSlot % 2;
}

// Two out of range or facing away; in range, two when the actor is the Rok's carrier slot, else one.

// The largest range the condition will test, and the dot the actor has to beat.
#define RANGE_CEILING 150.0f
#define FACING_DOT    -0.8f

// FUNCTION: HYPERX 0x00468c40
void __cdecl PlanCond::RokInRange(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg)
{
    const unsigned int nRange = nArg;

    const Vec3* pRok;
    float       flValue;
    Vec3        vOffset;

    const int nFlag = pArgs->m_pArgs[1].pNode->m_pArgs[0].n;

    float flRange = (float)nRange;

    const Vec3* pSelf = pContext->GetActor()->GetPositionPtr();

    pRok = g_pGameEngine->m_ppSlots[SLOT_ROK]->GetPositionPtr();

    vOffset.x = pSelf->x - pRok->x;
    vOffset.y = pSelf->y - pRok->y;
    vOffset.z = pSelf->z - pRok->z;

    if (nFlag != 0)
    {
        const float flSide = (float)sqrt(flRange) + g_flOnePlan;

        flRange = flSide * flSide;
    }

    EngineObject* const pActor = pContext->GetActor();

    const short nSlot = pActor->GetSlot();

    if (nSlot < PLAYER_SLOT_COUNT)
    {
        PlayerObject* const pOwner = pContext->GetPlayerActor();

        const float flHandling = pOwner->m_stats.GetScalar(RATING_ROK_HANDLING);

        flValue = pOwner->m_stats.GetScalar(RATING_DAMAGE);

        flValue *= flHandling;

        flValue = flValue * 0.01f + flHandling;

        flValue *= 0.5f;

        flValue -= 7.0f;

        flValue *= 0.05f;

        flValue += 1.0f;

        flValue *= flRange;
    }
    else
    {
        flValue = flRange;
    }

    if (RANGE_CEILING < flValue)
    {
        flValue = RANGE_CEILING;
    }

    if (Vec3::DotInline(&vOffset, &vOffset) <= flValue)
    {
        Vec3 vHeading;
        Vec3 vNormal;

        vNormal.x = pSelf->x - pRok->x;
        vNormal.y = pSelf->y - pRok->y;
        vNormal.z = pSelf->z - pRok->z;

        Math::Vec3Normalize(&vNormal);

        vHeading = pContext->GetActor()->m_pBody->vel;

        Math::Vec3Normalize(&vHeading);

        if (Vec3::DotInline(&vHeading, &vNormal) > FACING_DOT)
        {
            const int nTracked = g_pGameEngine->GetRok()->m_nCarrierSlot;

            *pResult = (short)((nTracked == pContext->GetActor()->GetSlot()) + 1);

            return;
        }

        *pResult = 2;

        return;
    }

    *pResult = 2;
}

#define GATED_SLOTS 4

#define RANGE_START 600.0f

// Goalies at slots four and five are never damage-gated, so they stay in the running.
// FUNCTION: HYPERX 0x00468e50
void __cdecl PlanCond::ClosestDromerToRok(Interactor*, PlanNode*, short* pResult, int)
{
    EngineObject* apActor[PLAYER_OBJECT_COUNT];
    Vec3          aOffset[PLAYER_OBJECT_COUNT];
    Vec3          vTo;
    float         flLimit;
    int           nSlot;

    *pResult = GATED_SLOTS;

    float flBest = RANGE_START;

    EngineObject** const ppObjects = g_pGameEngine->m_ppSlots;

    apActor[0] = ppObjects[0];
    apActor[1] = ppObjects[1];
    apActor[2] = ppObjects[2];
    apActor[3] = ppObjects[3];
    apActor[4] = ppObjects[4];
    apActor[5] = ppObjects[5];

    EngineObject* const pRok = ppObjects[SLOT_ROK];

    const Vec3* pSelf  = apActor[0]->GetPositionPtr();
    const Vec3* pRokAt = pRok->GetPositionPtr();

    vTo.x = pRokAt->x - pSelf->x;
    vTo.y = pRokAt->y - pSelf->y;
    vTo.z = pRokAt->z - pSelf->z;

    aOffset[0] = vTo;

    pSelf  = apActor[1]->GetPositionPtr();
    pRokAt = pRok->GetPositionPtr();

    vTo.x = pRokAt->x - pSelf->x;
    vTo.y = pRokAt->y - pSelf->y;
    vTo.z = pRokAt->z - pSelf->z;

    aOffset[1] = vTo;

    pSelf  = apActor[2]->GetPositionPtr();
    pRokAt = pRok->GetPositionPtr();

    vTo.x = pRokAt->x - pSelf->x;
    vTo.y = pRokAt->y - pSelf->y;
    vTo.z = pRokAt->z - pSelf->z;

    aOffset[2] = vTo;

    pSelf  = apActor[3]->GetPositionPtr();
    pRokAt = pRok->GetPositionPtr();

    vTo.x = pRokAt->x - pSelf->x;
    vTo.y = pRokAt->y - pSelf->y;
    vTo.z = pRokAt->z - pSelf->z;

    aOffset[3] = vTo;

    pSelf  = apActor[4]->GetPositionPtr();
    pRokAt = pRok->GetPositionPtr();

    vTo.x = pRokAt->x - pSelf->x;
    vTo.y = pRokAt->y - pSelf->y;
    vTo.z = pRokAt->z - pSelf->z;

    aOffset[4] = vTo;

    pSelf  = apActor[5]->GetPositionPtr();
    pRokAt = pRok->GetPositionPtr();

    vTo.x = pRokAt->x - pSelf->x;
    vTo.y = pRokAt->y - pSelf->y;
    vTo.z = pRokAt->z - pSelf->z;

    aOffset[5] = vTo;

    for (nSlot = 0; nSlot < PLAYER_OBJECT_COUNT; nSlot++)
    {
        if (nSlot < GATED_SLOTS)
        {
            flLimit = DamageGate();

            if (flLimit >= ((PlayerObject*)apActor[nSlot])->m_stats.GetScalar(RATING_DAMAGE))
            {
                continue;
            }
        }

        const float flZ = aOffset[nSlot].z;
        const float flY = aOffset[nSlot].y;
        const float flX = aOffset[nSlot].x;

        const float flDistance = (flX * flX + flZ * flZ) + flY * flY;

        if (flDistance < flBest)
        {
            *pResult = (unsigned short)nSlot;

            flBest = flDistance;
        }
    }
}

// FUNCTION: HYPERX 0x00469110
void PlanCond::SoundBank(Interactor*, PlanNode*, short* pResult, int)
{
    *pResult = PlanUtil::CheckSequenceStep((unsigned int)-1) == 0;
}

// FUNCTION: HYPERX 0x00469130
void PlanCond::GetTeamParity(Interactor* pContext, PlanNode*, short* pResult, int)
{
    EngineObject* const pActor = pContext->GetActor();

    *pResult = (signed char)(pActor->GetSlot() & 1);
}

// FUNCTION: HYPERX 0x00469150
void PlanCond::WinningOrLosing(Interactor* pContext, PlanNode*, short* pResult, int)
{
    EngineObject* const pActor = pContext->GetActor();

    *pResult = (short)(GameEngine::IsTeamWinning(
                           (unsigned char)pActor->GetSlot()) == 0);
}

// A network session always answers two.

// FUNCTION: HYPERX 0x00469180
void __cdecl PlanCond::IsDromerKnockedDown(Interactor*, PlanNode* pArgs, short* pResult, int nDefault)
{
    int nSlot = pArgs->m_pArgs[1].pNode->m_pArgs[0].n;

    if (nSlot == PLAN_TOKEN_MESSAGE_DATA)
    {
        nSlot = nDefault;
    }

    PlayerObject* const pObject = g_pGameEngine->GetPlayer((short)nSlot);

    // m_stats at 0xd20 assumes a PlayerObject; RokObject puts its own field at the same offset.
    if (pObject->m_nKnockedDownState == 0 && DamageGate() < pObject->m_stats.GetScalar(RATING_DAMAGE))
    {
        *pResult = 2;
    }
    else
    {
        *pResult = 1;
    }

    if (g_bSessionActive != 0)
    {
        *pResult = 2;
    }
}

// FUNCTION: HYPERX 0x00469240
void __cdecl PlanCond::IsDromerInjured(Interactor* pContext, PlanNode* pArgs, short* pResult, int nPrevSlot)
{
    const PlanWord arg = pArgs->m_pArgs[1].pNode->m_pArgs[0];
    int            nSlot;
    float          flLimit;

    if (arg.IsPronoun())
    {
        nSlot = g_apPlanStatFns[arg.wPronoun](pContext->GetActor(), pArgs);
    }
    else
    {
        nSlot = arg.n;
    }

    if (nSlot == PLAN_TOKEN_MESSAGE_DATA)
    {
        nSlot = nPrevSlot;
    }

    PlayerObject* const pObject = g_pGameEngine->GetPlayer((short)nSlot);

    flLimit = DamageGate();

    if (pObject->m_stats.GetScalar(RATING_DAMAGE) <= flLimit)
    {
        *pResult = 1;
        return;
    }

    *pResult = 2;
}

// FUNCTION: HYPERX 0x00469310
void __cdecl PlanCond::CheckRenderFlag(Interactor* pContext, PlanNode* pArgs, short* pResult, int nPrevSlot)
{
    const PlanWord arg = pArgs->m_pArgs[1].pNode->m_pArgs[0];
    int            nSlot;

    if (arg.IsPronoun())
    {
        nSlot = g_apPlanStatFns[arg.wPronoun](pContext->GetActor(), pArgs);
    }
    else
    {
        nSlot = arg.n;
    }

    if (nSlot == PLAN_TOKEN_MESSAGE_DATA)
    {
        nSlot = nPrevSlot;
    }

    if ((g_pGameEngine->m_ppSlots[(short)nSlot]->m_nFlags & ENGINEOBJECT_HIDDEN) != 0)
    {
        *pResult = 1;
    }
    else
    {
        *pResult = 2;
    }
}

// 2.2, about a unit and a half either side of a slot's line.
// GLOBAL: HYPERX 0x004c4568
extern float g_flInterceptCorridorSq;
float        g_flInterceptCorridorSq = 2.2f;
