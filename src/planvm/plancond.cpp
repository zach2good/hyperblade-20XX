#include "common/compat.h"
#include <cstdint>
#include <algorithm>
#include "math/mathops.h"
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
#include <cmath>
#include <cstring>
#include <cstdlib>
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
#include "engine/particlesystem.h"
#include "physics/collisionplane.h"
#include "physics/collisioncheck.h"
#include "engine/object.h"
#include "engine/objectphase.h"
#include "planvm/decisionlog.h"
#include <SDL3/SDL.h>

constexpr auto kEntitySlotLimit = 0x66;

constexpr auto kJudgeStateLow = 0;

constexpr auto kJudgeStateHigh = 8;

constexpr auto kArgCountWithXY = 3;

constexpr auto kArgCountWithZ = 4;

#define ARG_OBJECT 1
#define ARG_FIRST  1

// One, the plan files' own copy. src/globals.h's g_flOne is at a different address.
// GLOBAL: HYPERX 0x004b36ec
extern const float g_flOnePlan;

// The velocity is fifty plus a roll of fifty, scaled by the frame delta, at a fixed climb.
constexpr auto kScatterRoll = 50;
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
void PlanCond::Decide_Attachment(Interactor* pContext, PlanNode*, short* pResult, int)
{
    EngineObject* pRange;
    EngineObject* pOther;
    float         flDot;
    Vec3          vDelta;
    Vec3          vPoint;

    const auto  nSlot  = pContext->GetOtherSlot();
    auto* const pActor = pContext->GetActor();

    pRange = g_pGameEngine->m_ppSlots[static_cast<short>(CollisionObj::GetModelId(nSlot))];

    if (pContext->GetActor()->GetPhysicsBody()->pAttach != nullptr)
    {
        pOther = g_pGameEngine->m_ppSlots[pContext->GetActor()->GetPhysicsBody()->pAttach->nSlot];

        if (*pResult < 0)
        {
            const Vec3* const pForwardOther = &pOther->GetPhysicsBody()->velDir;
            const Vec3* const pForwardRange = &pRange->GetPhysicsBody()->velDir;

            // Not GetColObjPtr: the client stores the dot between the load and its test (0x0040168a to 0x0040169c).
            auto* const pColObj = g_pGameEngine->m_ppColObjects[nSlot];

            flDot = pForwardRange->x * pForwardOther->x + pForwardRange->y * pForwardOther->y + pForwardRange->z * pForwardOther->z;

            if (pColObj == nullptr)
            {
                // STRING: HYPERX 0x004bb2b0
                g_gameApp.Fatal("ERROR: GetColObjPtr: NULL Pointer\n");
            }

            pColObj->GetWorldPosition(&vPoint);

            vDelta.x = pActor->GetPositionPtr()->x - vPoint.x;
            vDelta.y = pActor->GetPositionPtr()->y - vPoint.y;
            vDelta.z = pActor->GetPositionPtr()->z - vPoint.z;

            if (static_cast<float>(sqrt(static_cast<double>(vDelta.x * vDelta.x + vDelta.y * vDelta.y + vDelta.z * vDelta.z))) < INTERACT_RANGE)
            {
                if (((pRange->GetSlot() ^ pOther->GetSlot()) & 1) != 0)
                {
                    auto* const pBodyOther = pOther->GetPhysicsBody();
                    auto* const pBodyRange = pRange->GetPhysicsBody();

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
    auto* const pSub = g_pGameEngine->GetCamera(kCameraHeadSlot);

    if (pSub != nullptr)
    {
        // The gap word at the end of CameraObject; this is its only reader.
        *pResult = pSub->m_bRearView != 0;
    }
}

// FUNCTION: HYPERX 0x00403b70
void PlanCond::DecideGoalieLaunch(Interactor* pContext, PlanNode*, short* pResult, int)
{
    const Vec3* pGoalCentre;
    float       flTime;

    auto* const pActor = pContext->GetActor();

    auto* const pRok = g_pGameEngine->m_ppSlots[kSlotRok];

    pActor->GetSlot();
    const Vec3* pPosition = pActor->GetPositionPtr();
    const auto  nTeam     = pActor->GetSlot();

    if (nTeam % 2 == 0)
    {
        pGoalCentre = g_apPositionMarkers[POS_N_GOAL_CENTER];
    }
    else
    {
        pGoalCentre = g_apPositionMarkers[kPosSGoalCenter];
    }

    auto              vTriple  = pActor->GetPhysicsBody()->vel;
    const Vec3* const pGoalPos = pRok->GetPositionPtr();

    const auto flDx = pGoalCentre->x - pGoalPos->x;
    const auto flDy = pGoalCentre->y - pGoalPos->y;
    const auto flDz = pGoalCentre->z - pGoalPos->z;

    flTime = static_cast<float>(sqrt(static_cast<double>(flDx * flDx + flDy * flDy + flDz * flDz))) / static_cast<float>(sqrt(static_cast<double>(vTriple.x * vTriple.x + vTriple.y * vTriple.y + vTriple.z * vTriple.z))) * g_pGameEngine->m_clock.flDelta;

    vTriple.x = pGoalCentre->x - pPosition->x;
    vTriple.y = pGoalCentre->y - pPosition->y;
    vTriple.z = pGoalCentre->z - pPosition->z;

    auto* const pOwner = pContext->GetPlayerActor();

    const auto flAccuracy = pOwner->m_stats.GetScalar(kRatingShotAccuracy);

    if (flAccuracy * APPROACH_ACCURACY_WEIGHT + flTime > APPROACH_TIME_GATE)
    {
        *pResult = 0;
        return;
    }

    if (static_cast<float>(sqrt(static_cast<double>(vTriple.x * vTriple.x + vTriple.y * vTriple.y + vTriple.z * vTriple.z))) > APPROACH_RANGE_GATE)
    {
        *pResult = 1;
        return;
    }

    *pResult = 2;
}

// Starts at one and is only cleared; OffWing, Defense and Sideln are rejected by name.

// FUNCTION: HYPERX 0x00404590
void PlanCond::OkToLaunchRok(Interactor* pContext, PlanNode*, short* pOutValid, int)
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

    const auto nTarget = pContext->GetTargetSlot();

    auto* const pszName = g_pGameEngine->m_ppSlots[nTarget]->GetPlanState()->m_pszName;

    if (pszName == nullptr
        // STRING: HYPERX 0x004bb38c
        || SDL_strcasecmp(pszName, "OffWing") == 0
        // STRING: HYPERX 0x004bb384
        || SDL_strcasecmp(pszName, "Defense") == 0
        // STRING: HYPERX 0x004bb37c
        || SDL_strcasecmp(pszName, "Sideln") == 0)
    {
        *pOutValid = 0;
    }

    auto* const pHeld = pContext->GetActor()->GetPhysicsBody()->pAttach;

    if (pHeld != nullptr && nTarget != pHeld->nSlot)
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
    const auto nIndex = static_cast<short>(pNode->m_pArgs[1].n);

    pActor->GetPlanState()->m_apNamedInteractors[nIndex] = pInteractor;
}

constexpr auto kArgCountDirect  = 2;
constexpr auto kArgCountIndexed = 3;

// A count of three indexes the named interactors only under a kPlanArgInteractorName high half.
// FUNCTION: HYPERX 0x0045e320
void PlanCond::SetActiveState(EngineObject* pActor, PlanNode* pArgs, GameBehaviorObject*, Interactor* pInteractor)
{
    const auto nArgCount = pArgs->m_nArgCount;

    if (nArgCount == kArgCountDirect)
    {
        pInteractor->SetActiveState(pArgs->m_pArgs[1].n);
        return;
    }

    if (nArgCount == kArgCountIndexed)
    {
        if (!pArgs->m_pArgs[1].IsInteractorName())
        {
            return;
        }

        const auto nIndex = static_cast<short>(pArgs->m_pArgs[1].n);

        auto* const pList = pActor->GetPlanState();

        if (nIndex < pList->m_nNamedInteractors)
        {
            auto* const pEntry = pList->m_apNamedInteractors[nIndex];

            if (pEntry != nullptr)
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

    auto* const pActor = pContext->GetActor();

    *pResult = static_cast<short>(pActor->GetSlot() & 1) != g_wActiveScoreLine;
}

// FUNCTION: HYPERX 0x0045e430
void PlanAction::LaunchGunDirection(Interactor* pContext, PlanNode*, short*, int)
{
    float flSign;
    float flDelta;

    auto* const pForward = &pContext->GetActor()->GetPhysicsBody()->velDir;

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

    const auto flSpeed = (static_cast<float>(rand() % kScatterRoll) + SCATTER_FLOOR) * flDelta;

    auto* const pBody = pContext->GetActor()->GetPhysicsBody();

    Vec3::ScaleTo(pForward, flSpeed, &pBody->vel);
}

// A failed build returns the agenda item to its pool, so it leaves nothing behind.

constexpr auto kArgKind = 1;
#define ARG_ANIM 2
constexpr auto kArgStart = 3;
#define ARG_DURATION 4

// FUNCTION: HYPERX 0x0045e4d0
void PlanAction::SetPaletteEffect(Interactor* pContext, PlanNode* pNode, short*, int)
{
    auto* const pAgendaItem = AllocAgendaItem(Agenda_CPaletteEffect);

    pAgendaItem->Bind(pContext->GetActor(), nullptr);

    auto* const pArgs = pNode->m_pArgs;

    if (((CPaletteEffect*)pAgendaItem)
            ->CreateVisualEffect(static_cast<EffectKind>(pArgs[kArgKind].n), pArgs[ARG_ANIM].n, pArgs[kArgStart].pe, pArgs[ARG_DURATION].n) != 0)
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

    const auto nArgCount = pArgs->m_nArgCount;

    if (nArgCount == 1)
    {
        pTarget = g_pGameEngine->m_ppSlots[pContext->GetTargetSlot()];

        pActor = (DeviceObject*)pContext->GetActor();

        pActor->m_pOwner = pTarget;
    }
    else if (nArgCount == 2)
    {
        pTarget = g_pGameEngine->m_ppSlots[static_cast<short>(pArgs->m_pArgs[1].n)];

        pActor = (DeviceObject*)pContext->GetActor();

        pActor->m_pOwner = pTarget;
    }
}

// FUNCTION: HYPERX 0x0045e5a0
void PlanAction::SignalKeystate(Interactor*, PlanNode* pArgs, short*, int)
{
    const PlanWord* const pBlock = pArgs->m_pArgs;

    g_pGameEngine->GetInputDevice(static_cast<short>(pBlock[1].n))->OnButtonDown(static_cast<short>(pBlock[2].n));
}

// A target of _MESSAGE_DATA_ uses the interpreter's value, and _BROADCAST_ reaches everyone.
// FUNCTION: HYPERX 0x0045e5d0
void PlanAction::ReallySendMsg(Interactor* pContext, PlanNode* pArgs, short*, int nArg)
{
    unsigned int  nTarget;
    EngineObject* pActor;

    const PlanWord* const pBlock = pArgs->m_pArgs;
    const auto            nEvent = pBlock[2].n;

    if (pBlock[1].IsPronoun())
    {
        nTarget = g_apPlanStatFns[pBlock[1].wPronoun](pContext->GetActor(), pArgs);
    }
    else
    {
        nTarget = pBlock[1].n;
    }

    if (static_cast<int>(nTarget) == PLAN_TOKEN_MESSAGE_DATA)
    {
        nTarget = nArg;
    }

    if (pArgs->m_nArgCount == 4 && pArgs->m_pArgs[3].n != PLAN_TOKEN_MESSAGE_DATA)
    {
        nArg = pArgs->m_pArgs[3].n;
    }

    if (nTarget == kPlanTokenBroadcast)
    {
        pActor = pContext->GetActor();

        g_pGameEngine->BroadcastEvent(nEvent, nArg, pActor->GetSlot());

        return;
    }

    auto* const pTarget = g_pGameEngine->m_ppSlots[static_cast<short>(nTarget)];
    pActor              = pContext->GetActor();

    pTarget->DispatchSceneMessage(nEvent, nArg, kDecisionLogFresh, pActor->GetSlot(), 0);
}

// FUNCTION: HYPERX 0x0045e6a0
void PlanAction::SendMsg(Interactor* pContext, PlanNode* pArgs, short*, int nParam)
{
    unsigned int  nTarget;
    EngineObject* pActor;

    const PlanWord* const pBlock = pArgs->m_pArgs;
    const auto            nEvent = pBlock[2].n;

    if (pBlock[1].IsPronoun())
    {
        nTarget = g_apPlanStatFns[pBlock[1].wPronoun](pContext->GetActor(), pArgs);
    }
    else
    {
        nTarget = pBlock[1].n;
    }

    if (static_cast<int>(nTarget) == PLAN_TOKEN_MESSAGE_DATA)
    {
        nTarget = nParam;
    }

    if (pArgs->m_nArgCount == 4 && pArgs->m_pArgs[3].n != PLAN_TOKEN_MESSAGE_DATA)
    {
        nParam = pArgs->m_pArgs[3].n;
    }

    if (nTarget == kPlanTokenBroadcast)
    {
        pActor = pContext->GetActor();

        g_pGameEngine->BroadcastEvent(nEvent, nParam, pActor->GetSlot());

        return;
    }

    auto* const pTarget = g_pGameEngine->m_ppSlots[static_cast<short>(nTarget)];

    if (pTarget != nullptr)
    {
        pActor = pContext->GetActor();

        pTarget->PostSceneMessage(nEvent, nParam, pActor->GetSlot(), 0);
    }
}

#define POSSESSION_TIME_SCALE 100.0f

// FUNCTION: HYPERX 0x0045e770
void PlanAction::SyncPossession(Interactor* pActor, PlanNode*, short*, int nArg)
{
    const PossessionSyncWord packed = { static_cast<unsigned int>(nArg) };

    TeamBlock*    pTeam;
    PlayerObject* pObject;

    if (pActor->GetActor()->GetPhase() == ObjectPhase::kLocal)
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

    const auto nRecord         = packed.GetRow();
    const auto nSide           = packed.GetTeam();
    const auto nShotsOnGoal    = packed.GetShotsOnGoal();
    const auto nPossessionTime = packed.GetPossessionTime();

    if (nRecord == pTeam->aSeat[0])
    {
        pObject = g_pGameEngine->GetPlayer(static_cast<short>(nSide));

        pObject->m_matchStats.SetAt(kMatchStatTimeOfPoss, static_cast<float>(nPossessionTime) * POSSESSION_TIME_SCALE);
        pObject->m_matchStats.SetAt(kMatchStatShotsOnGoal, static_cast<float>(nShotsOnGoal));
    }
    else if (nRecord == pTeam->aSeat[1])
    {
        pObject = g_pGameEngine->GetPlayer(static_cast<short>(nSide + 2));

        pObject->m_matchStats.SetAt(kMatchStatTimeOfPoss, static_cast<float>(nPossessionTime) * POSSESSION_TIME_SCALE);
        pObject->m_matchStats.SetAt(kMatchStatShotsOnGoal, static_cast<float>(nShotsOnGoal));
    }
    else
    {
        const auto flValue = static_cast<float>(nPossessionTime);

        pTeam->aStatRows[nRecord].aStat[kMatchStatTimeOfPoss]  = flValue * POSSESSION_TIME_SCALE;
        pTeam->aStatRows[nRecord].aStat[kMatchStatShotsOnGoal] = static_cast<float>(nShotsOnGoal);
    }
}

// The pause must be asked for three frames running before the mark moves and is broadcast.

// How many frames running have asked for a pause state the engine is not in.
// GLOBAL: HYPERX 0x004cfa50
extern int g_nPauseRequestStreak;

constexpr auto kPauseMarkNone = (-1);

constexpr auto kPauseStreakNeeded = 3;

// The host packs the period's elapsed time in tenths of a second.
constexpr auto kSyncElapsedUnitMs = 100;

// Matches the host's clock by replaying the goal plans' broadcasts: a shot starts it, a goal stops it.
// FUNCTION: HYPERX 0x0045e890
void PlanAction::SyncSystem(Interactor* pContext, PlanNode*, short*, int nArg)
{
    const SystemSyncWord packed = { static_cast<unsigned int>(nArg) };

    int nTotal;

    auto* const pActor = pContext->GetActor();

    if (pActor->GetPhase() == ObjectPhase::kLocal)
    {
        return;
    }

    auto* const pClock = &g_pGameEngine->m_clock;
    const auto  nMark  = pClock->nPauseMark;

    if (nMark != kPauseMarkNone)
    {
        if (packed.GetPauseMark() != 0)
        {
            if (nMark != 1)
            {
                ++g_nPauseRequestStreak;

                if (g_nPauseRequestStreak >= kPauseStreakNeeded)
                {
                    pClock->nPauseMark = 1;

                    g_pGameEngine->BroadcastEvent(kMsgShoot, 0, pActor->GetSlot());

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

                if (g_nPauseRequestStreak >= kPauseStreakNeeded)
                {
                    pClock->nPauseMark = 0;

                    g_pGameEngine->BroadcastEvent(kMsgGoalScored, 0, pActor->GetSlot());

                    g_nPauseRequestStreak = 0;
                }
            }
            else
            {
                g_nPauseRequestStreak = 0;
            }
        }
    }

    nTotal                 = pClock->nPauseStartMs - packed.GetElapsed() * kSyncElapsedUnitMs;
    pClock->nPeriodStartMs = nTotal;

    g_pGameEngine->SetHomeScore(packed.GetHomeScore());
    g_pGameEngine->SetAwayScore(packed.GetAwayScore());
}

constexpr auto kArgCountWithSlot = 2;

// FUNCTION: HYPERX 0x0045e980
void PlanAction::SyncDamage(Interactor* pContext, PlanNode*, short*, int nValue)
{
    auto* const pActor = pContext->GetPlayerActor();

    if (pActor->GetPhase() != ObjectPhase::kLocal)
    {
        pActor->m_stats.m_aStats[RATING_SLOT(kRatingDamage)] = static_cast<float>(nValue & 0xff);
    }
}

// In a network session this machine must drive both the actor and the target.
// FUNCTION: HYPERX 0x0045e9c0
void PlanAction::TransferViewOwnership(Interactor* pContext, PlanNode* pArgs, short*, int nSlot)
{
    int nIndex;

    auto*     pActor   = pContext->GetActor();
    const int nOwnSlot = pActor->GetSlot();

    if (pArgs->m_nArgCount == kArgCountWithSlot)
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

    pActor        = pContext->GetActor();
    auto* pTarget = g_pGameEngine->m_ppSlots[static_cast<short>(nIndex)];

    if (pActor != nullptr && pTarget != nullptr && (g_bSessionActive == 0 || (pActor->GetPhase() == ObjectPhase::kLocal && pTarget->GetPhase() == ObjectPhase::kLocal)))
    {
        g_pGameEngine->m_nViewedSlot = static_cast<short>(nIndex);

        g_pGameEngine->m_apTracked[0] = g_pGameEngine->m_ppSlots[static_cast<short>(nIndex)];
        g_pGameEngine->m_apTracked[1] = g_pGameEngine->m_ppSlots[static_cast<short>(nIndex)];

        if (nIndex == nOwnSlot)
        {
            g_pGameEngine->m_bFollowingOther = 0;

            return;
        }

        g_pGameEngine->m_bFollowingOther = 1;
    }
}

constexpr auto kArgCountWithValue = 3;

// In a network session the actor must be the viewed slot or driven by this machine.
// FUNCTION: HYPERX 0x0045ea90
void PlanAction::SendMessageToMyCamera(Interactor* pContext, PlanNode* pArgs, short*, int nParam)
{
    EngineObject* pActor;
    int           nValue;

    const auto nEvent = pArgs->m_pArgs[1].n;

    if (pArgs->m_nArgCount == kArgCountWithValue && pArgs->m_pArgs[2].n != PLAN_TOKEN_MESSAGE_DATA)
    {
        nValue = pArgs->m_pArgs[2].n;
    }
    else
    {
        nValue = nParam;
    }

    auto* const pCamera = g_pGameEngine->m_ppSlots[kCameraMainSlot];

    if (pCamera == nullptr)
    {
        return;
    }

    if (g_bSessionActive != 0)
    {
        pActor             = pContext->GetActor();
        const auto nViewed = g_pGameEngine->m_nViewedSlot;

        if (pActor->GetSlot() != nViewed)
        {
            pActor = pContext->GetActor();

            if (pActor->GetPhase() != ObjectPhase::kLocal)
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
void PlanAction::Set_SpeedBurst(Interactor* pContext, PlanNode* pNode, short*, int)
{
    float flScale;

    if (pNode->m_pArgs[ARG_PERIOD].n != 0)
    {
        auto* const pBurst = (CSpeedBurst*)AllocAgendaItem(Agenda_CSpeedBurst);

        if (pContext->GetPlayerActor()->m_stats.m_flSpeedRatio > 0.3)
        {
            flScale = g_flOnePlan;
        }
        else
        {
            flScale = g_flZeroPlan;
        }

        auto* const pCurrentBehaviour = pContext->GetActor()->GetPlanState()->m_pCurrent;

        pBurst->Bind(pContext->GetActor(), pCurrentBehaviour);
        pBurst->Activate();

        pBurst->m_flTarget =
            static_cast<float>(80.0 / static_cast<unsigned int>(pNode->m_pArgs[ARG_PERIOD].n) * flScale + g_flOnePlan);

        *pBurst->m_pBoost = 1.05f;

        auto* pFirst = pContext->GetActor()->GetPhysicsBody();

        auto* const pSpin  = &pFirst->axis;
        const auto  flSpin = pSpin->x + 0.25f;

        pSpin->x = flSpin;

        pFirst = pContext->GetActor()->GetPhysicsBody();

        pFirst->flFriction = 0.1f;

        pFirst = pContext->GetActor()->GetPhysicsBody();

        auto* const pVel = &pFirst->vel;

        const auto flLen = static_cast<float>(sqrt(static_cast<double>(pVel->x * pVel->x + pVel->y * pVel->y + pVel->z * pVel->z)));

        if (flLen < 0.1)
        {
            pFirst              = pContext->GetActor()->GetPhysicsBody();
            auto* const pSecond = pContext->GetActor()->GetPhysicsBody();

            pSecond->vel = pFirst->velDir;
        }
    }
    else
    {
        auto* const pBlock = pContext->GetActor()->GetPlanState();

        pBlock->m_pCurrent->RemoveAgendaItem(Agenda_CSpeedBurst);
    }
}

constexpr auto kArgColObject = 1;
#define ARG_STAT 2

constexpr auto kArgCountActor = 2;
constexpr auto kArgCountSlot  = 3;

// Tells whichever of the mine, the swarf and the Rok the actor carries to deploy.
// FUNCTION: HYPERX 0x0045ecc0
void PlanAction::DeployWeapon(Interactor* pContext, PlanNode*, short*, int)
{
    EngineObject* pActor;
    AttachPak*    pSlot;
    short         nSlot;

    if (pContext->GetNamedSlot() == -1)
    {
        auto* const pCollision = g_pGameEngine->GetColObjPtr(pContext->GetModelSlot());

        pActor = pCollision->GetOwner();
    }
    else
    {
        pActor = pContext->GetActor();
    }

    auto* pTracked = g_pGameEngine->m_ppSlots[kSlotMine];

    if (pTracked != nullptr)
    {
        pSlot = pTracked->GetPhysicsBody()->pAttach;

        if (pSlot != nullptr)
        {
            nSlot = pSlot->nSlot;

            if (nSlot < kPlayerObjectCount && nSlot == pActor->GetSlot())
            {
                pTracked->PostSceneEvent(kMsgDeployWeapon, 0);
            }
        }
    }

    pTracked = g_pGameEngine->m_ppSlots[kSlotSwarf];

    if (pTracked != nullptr)
    {
        pSlot = pTracked->GetPhysicsBody()->pAttach;

        if (pSlot != nullptr)
        {
            nSlot = pSlot->nSlot;

            if (nSlot < kPlayerObjectCount && nSlot == pActor->GetSlot())
            {
                pTracked->PostSceneEvent(kMsgDeployWeapon, 0);
            }
        }
    }

    pTracked = g_pGameEngine->m_ppSlots[kSlotRok];

    if (pTracked != nullptr)
    {
        pSlot = pTracked->GetPhysicsBody()->pAttach;

        if (pSlot != nullptr)
        {
            nSlot = pSlot->nSlot;

            if (nSlot < kPlayerObjectCount && nSlot == pActor->GetSlot())
            {
                pTracked->PostSceneEvent(kMsgDeployWeapon, 0);
            }
        }
    }
}

// Refuses a slot outside the players or the actor's own, writing minus one.
// FUNCTION: HYPERX 0x0045edd0
void PlanAction::SetAttackee(Interactor* pContext, PlanNode* pArgs, short*, int)
{
    auto nSlot = static_cast<short>(pArgs->m_pArgs[1].n);

    if (nSlot < 0 || nSlot > kGoalieSlotB)
    {
        nSlot = kSlotNone;
    }

    auto* const pActor = pContext->GetPlayerActor();

    if (pActor->GetSlot() == nSlot)
    {
        nSlot = kSlotNone;
    }

    pActor->m_wAttackeeSlot = nSlot;
}

// FUNCTION: HYPERX 0x0045ee20
void PlanAction::SetKnockedDownState(Interactor* pContext, PlanNode* pArgs, short*, int nArg)
{
    const auto nSlot = static_cast<short>(nArg);

    auto* const pActor = pContext->GetPlayerActor();

    if (pArgs->m_nArgCount == kArgCountSlot)
    {
        int nIndex = nSlot;

        if (pArgs->m_pArgs[2].n != PLAN_TOKEN_MESSAGE_DATA)
        {
            nIndex = static_cast<short>(pArgs->m_pArgs[2].n);
        }

        auto* const pTarget = g_pGameEngine->GetPlayer(nIndex);

        if (pTarget != nullptr)
        {
            pTarget->m_nKnockedDownState = pArgs->m_pArgs[1].n;

            return;
        }
    }
    else if (pArgs->m_nArgCount == kArgCountActor)
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

    short nSlot = kSlotArmory;

    auto nDistance = pContext->GetNamedSlot();

    if (nDistance == -1)
    {
        pArgs     = pNode->m_pArgs;
        pSource   = g_pGameEngine->m_ppSlots[static_cast<short>(pArgs[1].n)];
        nDistance = static_cast<short>(pArgs[2].n);

        if (pNode->m_nArgCount == 4)
        {
            nSlot = static_cast<short>(pArgs[3].n);
        }
    }
    else
    {
        pSource   = pContext->GetActor();
        pArgs     = pNode->m_pArgs;
        nDistance = static_cast<short>(pArgs[1].n);

        if (pNode->m_nArgCount == 3)
        {
            nSlot = static_cast<short>(pArgs[2].n);
        }
    }

    f.vResult.y = static_cast<float>(nDistance);

    if (pSource != nullptr)
    {
        auto* const pTarget = g_pGameEngine->m_ppSlots[nSlot];

        if (pTarget != nullptr)
        {
            f.vOffset.x = (pSource->m_transformC.m_a[0][2] + pSource->m_transformC.m_a[0][0]) * g_flZeroPlan + f.vResult.y * pSource->m_transformC.m_a[0][1];
            f.vOffset.y = (pSource->m_transformC.m_a[1][2] + pSource->m_transformC.m_a[1][0]) * g_flZeroPlan + f.vResult.y * pSource->m_transformC.m_a[1][1];
            f.vOffset.z = (pSource->m_transformC.m_a[2][2] + pSource->m_transformC.m_a[2][0]) * g_flZeroPlan + f.vResult.y * pSource->m_transformC.m_a[2][1];

            auto* const pBasePos = pTarget->GetPositionPtr();

            f.vResult.x = f.vOffset.x + pBasePos->x;
            f.vResult.y = f.vOffset.y + pBasePos->y;
            f.vResult.z = f.vOffset.z + pBasePos->z;

            pSource->m_pBody->pos = f.vResult;
        }
    }
}

// The ellipsoid's out-of-bounds height answer of one is not checked, in the original too.

// FUNCTION: HYPERX 0x0045efc0
void PlanAction::SetLocation(Interactor* pContext, PlanNode* pNode, short*, int nMarkerIndex)
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
    else if (pNode->m_nArgCount >= kArgCountWithXY)
    {
        pContext->GetActor()->GetPhysicsBody()->pos.x = pContext->GetActor()->GetPhysicsBody()->prevPos.x = pContext->GetActor()->GetPositionPtr()->x = PlanArg::ResolveFloat(pContext, pNode, 1);

        pContext->GetActor()->GetPhysicsBody()->pos.y = pContext->GetActor()->GetPhysicsBody()->prevPos.y = pContext->GetActor()->GetPositionPtr()->y = PlanArg::ResolveFloat(pContext, pNode, 2);

        if (pNode->m_nArgCount == kArgCountWithZ)
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

    pBody->surfNormal = *pNormal;

    pContext->GetActor()->GetPhysicsBody()->vel.z = g_flZeroPlan;
    pContext->GetActor()->GetPhysicsBody()->vel.y = g_flZeroPlan;
    pContext->GetActor()->GetPhysicsBody()->vel.x = g_flZeroPlan;

    if (pContext->GetActor()->GetSlot() < kPlayerObjectCount)
    {
        cSavedVisible = pContext->GetActor()->IsVisible();
        pContext->GetActor()->SetVisible(1);
        pContext->GetActor()->ApplyRenderSlotPose();
        pContext->GetActor()->SetVisible(cSavedVisible);
    }
}

// The copy through the owner's position accessor writes the position onto itself.

// A node of four arguments carries three literal integers.
constexpr auto kArgCountWithLiterals = 4;

// Below this length the heading is left alone.
#define MIN_VELOCITY_LENGTH 0.001f

// FUNCTION: HYPERX 0x0045f2a0
void PlanAction::SetVelocity(Interactor* pContext, PlanNode* pNode, short*, int)
{
    PlanWord* pnArgs;
    Vec3*     pOwnPosition;
    Vec3      vVelocity;

    if (pNode->m_nArgCount == kArgCountWithLiterals)
    {
        pnArgs = pNode->m_pArgs;

        vVelocity.x = static_cast<float>(pnArgs[1].n);
        vVelocity.y = static_cast<float>(pnArgs[2].n);
        vVelocity.z = static_cast<float>(pnArgs[3].n);
    }
    else
    {
        pnArgs = pNode->m_pArgs;

        vVelocity.x = g_apTrackOffsets[pnArgs[1].n]->x;
        vVelocity.y = g_apTrackOffsets[pnArgs[1].n]->y;
        vVelocity.z = g_apTrackOffsets[pnArgs[1].n]->z;
    }

    auto* const pPosition = &pContext->GetActor()->GetPhysicsBody()->pos;

    pContext->GetActor()->GetPhysicsBody()->TakeSurfaceNormal(pPosition);

    auto* const pVelocity = &pContext->GetActor()->GetPhysicsBody()->vel;

    pVelocity->x = vVelocity.x;
    pVelocity->y = vVelocity.y;
    pVelocity->z = vVelocity.z;

    auto* const pPrevious = &pContext->GetActor()->GetPhysicsBody()->prevPos;

    pPrevious->x = pPosition->x - vVelocity.x;
    pPrevious->y = pPosition->y - vVelocity.y;
    pPrevious->z = pPosition->z - vVelocity.z;

    pOwnPosition = pContext->GetActor()->GetPositionPtr();

    *pOwnPosition = *pPosition;

    pContext->GetActor()->GetPhysicsBody()->smoothVel.x = vVelocity.x;

    pContext->GetActor()->GetPhysicsBody()->smoothVel.y = vVelocity.y;

    pContext->GetActor()->GetPhysicsBody()->smoothVel.z = vVelocity.z;

    if (static_cast<float>(sqrt(static_cast<double>(vVelocity.x * vVelocity.x + vVelocity.y * vVelocity.y + vVelocity.z * vVelocity.z))) > MIN_VELOCITY_LENGTH)
    {
        auto* const pBody = pContext->GetActor()->GetPhysicsBody();

        Vec3::ScaleTo(&vVelocity, g_flOnePlan / static_cast<float>(sqrt(static_cast<double>(vVelocity.x * vVelocity.x + vVelocity.y * vVelocity.y + vVelocity.z * vVelocity.z))), &pBody->velDir);
    }
}

#define FACING_MIN_DOT 0.1f
#define TACKLE_RANGE   25.0
#define TACKLE_MIN_DOT 0.2f

constexpr auto kTackleMessageArg = 2;

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

    auto* const pMateBody  = pMate->GetPhysicsBody();
    auto* const pOwnerBody = pContext->GetActor()->GetPhysicsBody();

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
void PlanAction::Engage_Opponent(Interactor* pContext, PlanNode*, short*, int)
{
    AttachPak*            pnCarrier;
    short                 nCarrier = 0;
    bool                  bCarrier;
    float                 flDotGate;
    Vec3                  vOffset;
    CSetPositionRelative* pSetPosition;

    const auto nFirst = static_cast<short>((pContext->GetActor()->GetSlot() + 1) % kPlayerSlotCount);

    auto* const pFirst = g_pGameEngine->GetSlot(nFirst);

    const auto nSecond = static_cast<short>((pContext->GetActor()->GetSlot() + 3) % kPlayerSlotCount);

    auto* pSecond = g_pGameEngine->GetSlot(nSecond);

    pnCarrier = g_pGameEngine->GetSlot(kSlotRok)->GetPhysicsBody()->pAttach;

    if (pnCarrier != nullptr && pnCarrier->nSlot < kPlayerObjectCount)
    {
        nCarrier = pnCarrier->nSlot;
        bCarrier = true;
    }
    else
    {
        bCarrier = false;
    }

    const auto bLoose = !bCarrier;

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

    if (pSecond == nullptr)
    {
        return;
    }

    pSetPosition = (CSetPositionRelative*)AllocAgendaItem(Agenda_CSetPositionRelative);

    vOffset.x = -7.0f;
    vOffset.y = g_flThreePlan;
    vOffset.z = g_flZeroPlan;

    auto* pBindNode = pContext->GetActor()->GetPlanState()->m_pCurrent;

    pSetPosition->Bind(pContext->GetActor(), pBindNode);

    pSetPosition->SetSourceObjectByIndex(pSecond->GetSlot());

    pSetPosition->m_wMode = 1;

    pSetPosition->SetPositionWithTerrainHeight(&g_vZeroMarker);

    pSetPosition->SetOffset(vOffset);

    pSetPosition->Activate();

    auto* const pBungee = (CBungeeToward*)AllocAgendaItem(Agenda_CBungeeToward);

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

    pSecond->PostEvent(kMsgYerAttacked, kTackleMessageArg, pContext->GetActor()->GetSlot());
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

    auto* const pOpponentBody = pOpponent->GetPhysicsBody();
    auto* const pOwnerBody    = pContext->GetActor()->GetPhysicsBody();

    return Vec3::DotInline(&pOwnerBody->velDir, &pOpponentBody->velDir) > 0.2f;
}

} // namespace PlanCond

// Answers zero for the plus-one opponent, one for plus-three and two for neither.
// FUNCTION: HYPERX 0x0045f740
void PlanCond::EngageWhichOpponent(Interactor* pContext, PlanNode*, short* pResult, int)
{
    PlayerObject* pTarget;
    Vec3          vOffset;

    const auto sFirst = static_cast<short>((pContext->GetActor()->GetSlot() + 1) % kPlayerSlotCount);

    auto* const pFirst = g_pGameEngine->GetPlayer(sFirst);

    const auto sSecond = static_cast<short>((pContext->GetActor()->GetSlot() + 3) % kPlayerSlotCount);

    auto* const pSecond = g_pGameEngine->GetPlayer(sSecond);

    const auto flDamageA = pFirst->m_stats.GetScalar(kRatingDamage);
    const auto flDamageB = pSecond->m_stats.GetScalar(kRatingDamage);

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
        pTarget = nullptr;
    }

    if (pTarget != nullptr)
    {
        auto* const pSetPosition = (CSetPositionRelative*)AllocAgendaItem(Agenda_CSetPositionRelative);

        auto* const pCurrent = pContext->GetActor()->GetPlanState()->m_pCurrent;

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
    auto* const pState = g_pGameEngine->m_ppSlots[kSlotRok]->GetPhysicsBody()->pAttach;

    if (pState != nullptr)
    {
        const auto nHolderSlot = pState->nSlot;

        if (nHolderSlot < kPlayerObjectCount)
        {
            auto* const pActor = pContext->GetActor();

            if (((pActor->GetSlot() ^ nHolderSlot) & 1) == 0)
            {
                *pResult = 0;

                return;
            }
        }
    }

    *pResult = 1;
}

constexpr auto kAnnouncerGagMs = 0x3a98;

// Inside the gag window it answers one past the highest draw.
// FUNCTION: HYPERX 0x0045fac0
void PlanCond::GagTheAnnouncer(Interactor*, PlanNode* pArgs, short* pnOut, int)
{
    const int nNow = g_pGameEngine->m_clock.nTickMs;

    if (static_cast<unsigned int>(nNow - g_nAnnouncerLineMs) < kAnnouncerGagMs)
    {
        *pnOut = static_cast<short>(pArgs->m_nArgCount - 2);

        return;
    }

    *pnOut = static_cast<short>(static_cast<long long>(rand()) * (pArgs->m_nArgCount - 2) / (static_cast<long long>(RAND_MAX) + 1));

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
void PlanCond::AttackerIsInRear(Interactor* pContext, PlanNode*, short* pResult, int)
{
    EngineObject* pFirst;
    EngineObject* pSecond;
    Vec3          vGap;

    pFirst = g_pGameEngine->m_ppSlots[static_cast<short>(CollisionObj::GetModelId(
        static_cast<int>(pContext->GetModelSlot())))];

    const auto vFirst = *pFirst->GetPositionPtr();

    pSecond = g_pGameEngine->m_ppSlots[static_cast<short>(CollisionObj::GetModelId(
        static_cast<int>(pContext->GetOtherSlot())))];

    const auto vSecond = *pSecond->GetPositionPtr();

    vGap.x = vFirst.x - vSecond.x;
    vGap.y = vFirst.y - vSecond.y;
    vGap.z = vFirst.z - vSecond.z;

    auto vOffset = vGap;

    Math::Vec3Normalize(&vOffset);

    const Vec3* const pDirB = &pSecond->GetPhysicsBody()->velDir;

    if (vOffset.x * pDirB->x + vOffset.y * pDirB->y + vOffset.z * pDirB->z < -0.1f)
    {
        auto* const pBlockA = pFirst->GetPhysicsBody();
        auto* const pBlockB = pSecond->GetPhysicsBody();

        if (Vec3::DotInline(&pBlockB->velDir, &pBlockA->velDir) > 0.25 && static_cast<float>(sqrt(static_cast<double>(vGap.x * vGap.x + vGap.y * vGap.y + vGap.z * vGap.z))) < 150.0f)
        {
            *pResult = 0;

            return;
        }
    }

    *pResult = 1;
}

// The selector that arms the throttle instead of answering a state.
constexpr auto kSelectorArm = 0x18;

constexpr auto kArmWindowMs   = 300;
constexpr auto kStateWindowMs = 800;

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

    if (nSelector == kSelectorArm)
    {
        g_nRokCameraArmedTick = g_pGameEngine->m_clock.nTickMs;

        pTarget = g_pGameEngine->m_ppSlots[kSlotRok]->GetPhysicsBody()->pAttach;

        if (pTarget != nullptr && (nSlot = pTarget->nSlot) < kPlayerObjectCount)
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
        pTarget = g_pGameEngine->m_ppSlots[kSlotRok]->GetPhysicsBody()->pAttach;

        if (pTarget != nullptr && (nSlot = pTarget->nSlot) < kPlayerObjectCount)
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

                    auto* const pCandidate = g_pGameEngine->GetPlayer(TeammateSlot(nSlot));

                    if (pCandidate->m_stats.GetScalar(kRatingDamage) <= flLimit)
                    {
                        *pResult = 4;
                    }
                    else
                    {
                        const auto flFacing = static_cast<float>(cos(static_cast<double>(PlanUtil::ComputeConeHalfAngle(
                            nSlot, static_cast<short>(TeammateSlot(nSlot))))));

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

                if (nSlot == static_cast<short>(nOther & 3))
                {
                    if (g_pGameEngine->m_clock.nTickMs - g_nRokCameraArmedTick > kArmWindowMs)
                    {
                        const auto flFacing = static_cast<float>(cos(static_cast<double>(PlanUtil::ComputeConeHalfAngle(
                            nSlot, g_pGameEngine->m_nViewedSlot))));

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
                    else if (g_pGameEngine->m_clock.nTickMs - g_nRokCameraStateTick > kStateWindowMs)
                    {
                        *pResult = static_cast<short>((g_pGameEngine->m_bFollowingOther != 0) + 6);
                    }
                    else
                    {
                        *pResult = 7;
                    }
                }
                else if (g_pGameEngine->m_clock.nTickMs - g_nRokCameraArmedTick < kArmWindowMs && g_pGameEngine->m_clock.nTickMs - g_nRokCameraStateTick > kStateWindowMs)
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
        else if (g_pGameEngine->m_clock.nTickMs - g_nRokCameraArmedTick < kArmWindowMs)
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

    if (pOwner == nullptr)
    {
        return;
    }

    nKind = g_pGameEngine->m_nViewedSlot;

    auto* const pOtherOwner = g_pGameEngine->m_ppSlots[TeammateSlot(nKind)]->GetPhysicsBody()->pPlayer;

    if (pOtherOwner == nullptr)
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

constexpr auto kRelationshipKindDirection = 0x16;

// Kind 0x16 normalises a direction nothing reads, in the original too.
// FUNCTION: HYPERX 0x00460140
void PlanCond::AnalyzePadB(Interactor*, PlanNode*, short* pResult, int nKind)
{
    EngineObject* pTarget;
    AttachPak*    pSlot;
    short         nSlot;
    Vec3          vDirection;

    if (nKind == kRelationshipKindDirection)
    {
        auto* const pRok = g_pGameEngine->m_ppSlots[kSlotRok];
        pSlot            = pRok->GetPhysicsBody()->pAttach;

        if (pSlot != nullptr && (nSlot = pSlot->nSlot) < kPlayerObjectCount)
        {
            auto* const pFollowed = g_pGameEngine->GetPlayer(g_pGameEngine->m_nViewedSlot);

            if (nSlot == pFollowed->GetSlot())
            {
                pTarget = g_pGameEngine->m_ppSlots[pFollowed->m_wTargetGoalSlot];

                auto* const pFrom = pRok->GetPositionPtr();
                auto* const pTo   = pTarget->GetPositionPtr();

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

    pSlot = g_pGameEngine->m_ppSlots[kSlotRok]->GetPhysicsBody()->pAttach;

    if (pSlot != nullptr && (nSlot = pSlot->nSlot) < kPlayerObjectCount && nSlot == g_pGameEngine->m_nViewedSlot)
    {
        *pResult = 2;

        return;
    }

    *pResult = 3;
}

// Five hundred, as a squared distance, a little over twenty two: both opponents have to be beyond it.
#define FAR_RANGE_SQUARED 500.0f

// FUNCTION: HYPERX 0x00460240
void PlanCond::InRangeToAttack(Interactor* pContext, PlanNode*, short* pResult, int)
{
    Vec3 vNear;
    Vec3 vFar;

    auto bySlot = static_cast<unsigned char>(pContext->GetActor()->GetSlot());

    const Vec3* const pNearAt = g_pGameEngine->m_ppSlots[OPPOSING_FIRST(bySlot)]->GetPositionPtr();

    bySlot = static_cast<unsigned char>(pContext->GetActor()->GetSlot());

    const Vec3* const pFarAt = g_pGameEngine->m_ppSlots[OPPOSING_SECOND(bySlot)]->GetPositionPtr();

    const Vec3* const pSelfAt = pContext->GetActor()->GetPositionPtr();

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
void PlanCond::Decide_BlockOrGoOut(Interactor* pContext, PlanNode*, short* pResult, int)
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

    auto* pActor = pContext->GetActor();
    auto  bySlot = static_cast<unsigned char>(pActor->GetSlot());
    pFirst       = g_pGameEngine->m_ppSlots[OPPOSING_FIRST(bySlot)]->GetPositionPtr();

    pActor  = pContext->GetActor();
    bySlot  = static_cast<unsigned char>(pActor->GetSlot());
    pSecond = g_pGameEngine->m_ppSlots[OPPOSING_SECOND(bySlot)]->GetPositionPtr();

    pActor = pContext->GetActor();

    const auto nSlot = pActor->GetSlot();

    pObject = g_pGameEngine->m_ppSlots[TeammateSlot(nSlot)]->GetPositionPtr();

    const Vec3* const pSelf = pContext->GetActor()->GetPositionPtr();

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

    const auto flHypotFirst = static_cast<float>(sqrt(static_cast<double>(vFirst.x * vFirst.x + vFirst.y * vFirst.y + vFirst.z * vFirst.z)));
    flToSecond              = static_cast<float>(sqrt(static_cast<double>(vSecond.x * vSecond.x + vSecond.y * vSecond.y + vSecond.z * vSecond.z)));

    const auto bFirstCloser = flHypotFirst < flToSecond;

    flToObject = static_cast<float>(sqrt(static_cast<double>(vObjectSelf.x * vObjectSelf.x + vObjectSelf.y * vObjectSelf.y + vObjectSelf.z * vObjectSelf.z)));

    if (bFirstCloser)
    {
        if (flHypotFirst <= 100.0 && static_cast<float>(sqrt(static_cast<double>(vFirstSelf.x * vFirstSelf.x + vFirstSelf.y * vFirstSelf.y + vFirstSelf.z * vFirstSelf.z))) + flToObject <= flHypotFirst * PATH_SLACK)
        {
            *pResult = 0;

            return;
        }

        *pResult = 1;

        return;
    }

    if (flToSecond <= 100.0 && static_cast<float>(sqrt(static_cast<double>(vSecondSelf.x * vSecondSelf.x + vSecondSelf.y * vSecondSelf.y + vSecondSelf.z * vSecondSelf.z))) + flToObject <= flToSecond * PATH_SLACK)
    {
        *pResult = 0;

        return;
    }

    *pResult = 1;
}

// FUNCTION: HYPERX 0x004605a0
void PlanCond::SpeedBurstIsRelaxed(Interactor* pContext, PlanNode*, short* pResult, int)
{
    auto* const pActor = pContext->GetActor();

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
void PlanCond::WhichOppnIsClosestToRok(Interactor* pContext, PlanNode*, short* pResult, int)
{
    const Vec3* pRokAt;
    Vec3        vFar;
    float       flDamageNear;

    pRokAt = g_pGameEngine->m_ppSlots[kSlotRok]->GetPositionPtr();

    auto bySlot = static_cast<unsigned char>(pContext->GetActor()->GetSlot());

    auto* pOpponent = g_pGameEngine->GetPlayer(OPPOSING_FIRST(bySlot));

    const Vec3* const pNearAt = pOpponent->GetPositionPtr();

    bySlot = static_cast<unsigned char>(pContext->GetActor()->GetSlot());

    pOpponent = g_pGameEngine->GetPlayer(OPPOSING_SECOND(bySlot));

    const Vec3* const pFarAt = pOpponent->GetPositionPtr();

    const auto flNearX = pRokAt->x - pNearAt->x;
    const auto flNearY = pRokAt->y - pNearAt->y;
    const auto flNearZ = pRokAt->z - pNearAt->z;

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

    flDamageNear = pOpponent->m_stats.GetScalar(kRatingDamage);

    pOpponent = g_pGameEngine->GetPlayer(OPPOSING_SECOND(pContext->GetActor()->GetSlot()));

    const auto flDamageFar = pOpponent->m_stats.GetScalar(kRatingDamage);

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
constexpr auto kNoPassTarget = 4;

// FUNCTION: HYPERX 0x004608a0
void PlanCond::BallGotBehindMe(Interactor* pContext, PlanNode*, short* pResult, int)
{
    const Vec3* pGoalCentre;
    Vec3        vSpawn;
    Vec3        vRok;
    Vec3        vActor;

    auto* const pActor = pContext->GetActor();

    pActor->GetSlot();

    auto* const pRok = g_pGameEngine->m_ppSlots[kSlotRok];

    const auto nTeam = pActor->GetSlot();

    if (nTeam % 2 == 0)
    {
        pGoalCentre = g_apPositionMarkers[POS_N_GOAL_CENTER];
    }
    else
    {
        pGoalCentre = g_apPositionMarkers[kPosSGoalCenter];
    }

    vSpawn.x = pGoalCentre->x;
    vSpawn.y = pGoalCentre->y;
    vSpawn.z = pGoalCentre->z;

    vActor = *pActor->GetPositionPtr();

    vRok = *pRok->GetPositionPtr();

    const auto flDx = vSpawn.x - vRok.x;
    const auto flDy = vSpawn.y - vRok.y;
    const auto flDz = vSpawn.z - vRok.z;

    Vec3 vToSpawn;

    vToSpawn.x = vSpawn.x - vActor.x;
    vToSpawn.y = vSpawn.y - vActor.y;
    vToSpawn.z = vSpawn.z - vActor.z;

    if (static_cast<float>(sqrt(static_cast<double>(flDx * flDx + flDy * flDy + flDz * flDz))) > static_cast<float>(sqrt(static_cast<double>(vToSpawn.x * vToSpawn.x + vToSpawn.y * vToSpawn.y + vToSpawn.z * vToSpawn.z))))
    {
        *pResult = 0;

        return;
    }

    *pResult = 1;
}

// A team mate in the Sideln plan counts as hurt.
// FUNCTION: HYPERX 0x004609d0
void PlanCond::GoaliePassingTarget(Interactor* pContext, PlanNode*, short* pResult, int)
{
    auto* const pOwner = pContext->GetActor();

    const auto nSlot = pOwner->GetSlot() % 2;

    EngineObject** const apObjects = g_pGameEngine->m_ppSlots;

    auto* const pNear = apObjects[static_cast<short>(nSlot)];

    const auto nOther = nSlot + 2;

    auto* const pFar = apObjects[static_cast<short>(nOther)];

    const auto vOwn = *pOwner->GetPositionPtr();

    const auto vNear = *pNear->GetPositionPtr();

    const auto vFar = *pFar->GetPositionPtr();

    Vec3 vToNear;

    vToNear.x = vNear.x - vOwn.x;
    vToNear.y = vNear.y - vOwn.y;
    vToNear.z = vNear.z - vOwn.z;

    Vec3 vToFar;

    vToFar.x = vFar.x - vOwn.x;
    vToFar.y = vFar.y - vOwn.y;
    vToFar.z = vFar.z - vOwn.z;

    const auto flDistNear = static_cast<float>(sqrt(static_cast<double>((vToNear.z * vToNear.z + vToNear.y * vToNear.y) + vToNear.x * vToNear.x)));

    const auto flDistFar = static_cast<float>(sqrt(static_cast<double>((vToFar.z * vToFar.z + vToFar.y * vToFar.y) + vToFar.x * vToFar.x)));

    auto flDamageNear = ((PlayerObject*)pNear)->m_stats.GetScalar(kRatingDamage);

    auto flDamageFar = ((PlayerObject*)pFar)->m_stats.GetScalar(kRatingDamage);

    const char* const pszNear = pNear->GetPlanState()->m_pszName;

    // STRING: HYPERX 0x004bb37c
    if (strcmp(pszNear, "Sideln") == 0)
    {
        flDamageNear = 0.0f;
    }

    const char* const pszFar = pFar->GetPlanState()->m_pszName;

    // STRING: HYPERX 0x004bb37c
    if (strcmp(pszFar, "Sideln") == 0)
    {
        flDamageFar = 0.0f;
    }

    if (flDamageNear <= DamageGate())
    {
        if (flDamageFar <= DamageGate())
        {
            *pResult = kNoPassTarget;

            return;
        }
    }
    else
    {
        if (flDamageFar <= DamageGate() || flDistNear >= flDistFar)
        {
            *pResult = static_cast<short>(nSlot);

            return;
        }
    }

    *pResult = static_cast<short>(nOther);
}

// Answers zero or one for the opponent to attack and two for neither, DECIDE's child index.

// FUNCTION: HYPERX 0x00460c80
void PlanCond::WhichOppnToAttack(Interactor* pContext, PlanNode*, short* pResult, int)
{
    auto* pOpponent = g_pGameEngine->GetPlayer(OPPOSING_FIRST(pContext->GetActor()->GetSlot()));

    const auto flDamageA = pOpponent->m_stats.GetScalar(kRatingDamage);

    pOpponent = g_pGameEngine->GetPlayer(OPPOSING_SECOND(pContext->GetActor()->GetSlot()));

    const auto flDamageB = pOpponent->m_stats.GetScalar(kRatingDamage);

    pOpponent = g_pGameEngine->GetPlayer(OPPOSING_FIRST(pContext->GetActor()->GetSlot()));

    auto flDistanceA = pContext->GetActor()->GetDistIfFacingDefault(pOpponent);

    pOpponent = g_pGameEngine->GetPlayer(OPPOSING_SECOND(pContext->GetActor()->GetSlot()));

    auto flDistanceB = pContext->GetActor()->GetDistIfFacingDefault(pOpponent);

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
void PlanCond::WhichOppnIsClosestToMe(Interactor* pContext, PlanNode*, short* pResult, int)
{
    Vec3 vOffsetA;
    Vec3 vOffsetB;

    auto* const pOwner = pContext->GetActor();

    auto* pOpponent = g_pGameEngine->GetPlayer(OPPOSING_FIRST(pContext->GetActor()->GetSlot()));

    const auto flDamageA = pOpponent->m_stats.GetScalar(kRatingDamage);

    pOpponent = g_pGameEngine->GetPlayer(OPPOSING_SECOND(pContext->GetActor()->GetSlot()));

    const auto flDamageB = pOpponent->m_stats.GetScalar(kRatingDamage);

    pOpponent = g_pGameEngine->GetPlayer(OPPOSING_FIRST(pContext->GetActor()->GetSlot()));

    auto flDistanceA = pContext->GetActor()->GetDistIfFacingDefault(pOpponent);

    pOpponent = g_pGameEngine->GetPlayer(OPPOSING_SECOND(pContext->GetActor()->GetSlot()));

    auto flDistanceB = pContext->GetActor()->GetDistIfFacingDefault(pOpponent);

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
        auto bySlot = static_cast<unsigned char>(pContext->GetActor()->GetSlot());

        pOpponent = g_pGameEngine->GetPlayer(OPPOSING_FIRST(bySlot));

        const Vec3* const pPosA = pOpponent->GetPositionPtr();

        bySlot = static_cast<unsigned char>(pContext->GetActor()->GetSlot());

        pOpponent = g_pGameEngine->GetPlayer(OPPOSING_SECOND(bySlot));

        const Vec3* const pPosB = pOpponent->GetPositionPtr();

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
            flDistanceA = static_cast<float>(sqrt(static_cast<double>(vOffsetA.z * vOffsetA.z + vOffsetA.y * vOffsetA.y + vOffsetA.x * vOffsetA.x)));
        }

        if (flDamageB > DamageGate())
        {
            flDistanceB = static_cast<float>(sqrt(static_cast<double>(vOffsetB.z * vOffsetB.z + vOffsetB.y * vOffsetB.y + vOffsetB.x * vOffsetB.x)));
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
void PlanCond::ClosestTargetToProjectile(Interactor* pContext, PlanNode*, short* pResult, int)
{
    AttachPak* pnTargetSlot;
    float      flSpeed;
    float      flLimit;
    float      flNear;
    float      flFar;
    short      nSlot;

    auto* const pVelocity = &pContext->GetActor()->GetPhysicsBody()->vel;

    flSpeed = pContext->GetActor()->GetPhysicsBody()->flSpeed;

    pnTargetSlot = pContext->GetActor()->GetPhysicsBody()->pAttach;

    if (pnTargetSlot == nullptr || (nSlot = pnTargetSlot->nSlot) >= kPlayerObjectCount)
    {
        // TODO: The original takes the result pointer itself as the slot; was *pResult meant?
        nSlot = static_cast<short>((std::intptr_t)pResult);
    }

    auto* const pActor = g_pGameEngine->GetSlot(nSlot);

    const auto nNear = static_cast<short>(~nSlot & 1);
    const auto nFar  = static_cast<short>((~nSlot & 1) | 2);

    if (flSpeed > MIN_SPEED_FOR_FACING)
    {
        auto* const pBlock = pContext->GetActor()->GetPhysicsBody();

        Vec3::ScaleTo(pVelocity, g_flOnePlan / flSpeed, &pBlock->velDir);
    }

    auto* pFacing = pActor;

    flNear = pFacing->GetDistIfFacing(g_pGameEngine->GetSlot(nNear), FACING_MIN_DOT);

    pFacing = pActor;

    flFar = pFacing->GetDistIfFacing(g_pGameEngine->GetSlot(nFar), FACING_MIN_DOT);

    flLimit = DamageGate();

    auto* pCandidate = g_pGameEngine->GetPlayer(nNear);

    if (pCandidate->m_stats.GetScalar(kRatingDamage) <= flLimit)
    {
        flNear = g_flZeroPlan;
    }

    flLimit = DamageGate();

    pCandidate = g_pGameEngine->GetPlayer(nFar);

    if (pCandidate->m_stats.GetScalar(kRatingDamage) <= flLimit)
    {
        flFar = g_flZeroPlan;
    }

    if (flNear == 0.0f && flFar == 0.0f)
    {
        *pResult = kGoalieSlotA;

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
void PlanCond::ClosestUncoveredOppn(Interactor* pContext, PlanNode*, short* pResult, int)
{
    EngineObject* pTarget;
    AttachPak*    pCarrierSlot;
    EngineObject* pOther;
    Vec3          vFar;
    float         flDistNear;

    const auto sSlot = pContext->GetActor()->GetSlot();

    pTarget = g_pGameEngine->m_ppSlots[TeammateSlot(sSlot)];

    auto* pOpponent = g_pGameEngine->GetPlayer(OPPOSING_FIRST(pContext->GetActor()->GetSlot()));

    const auto flDamageNear = pOpponent->m_stats.GetScalar(kRatingDamage);

    pOpponent = g_pGameEngine->GetPlayer(OPPOSING_SECOND(pContext->GetActor()->GetSlot()));

    const auto flDamageFar = pOpponent->m_stats.GetScalar(kRatingDamage);

    if (flDamageNear <= DamageGate() && flDamageFar <= DamageGate())
    {
        *pResult = 2;
    }
    else
    {
        pCarrierSlot = g_pGameEngine->m_ppSlots[kSlotRok]->GetPhysicsBody()->pAttach;

        const int bNoCarrier = pCarrierSlot == nullptr || pCarrierSlot->nSlot >= kPlayerObjectCount;

        if (bNoCarrier != 0)
        {
            *pResult = 2;
        }
        else
        {
            auto* const pBehaviour = pTarget->GetPlanState()->m_pCurrent;

            if (pBehaviour->m_wKind == kBehGlideBetweenFacing && ((pOther = ((BehObj_GlideBetweenFacing*)pBehaviour)->m_pAimObject) == nullptr || pOther->GetSlot() <= kPlayerSlotCount - 1))
            {
                const auto sOther = pOther->GetSlot();

                const auto sMasked = static_cast<short>(TeammateSlot(sOther));

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
                auto bySlot = static_cast<unsigned char>(pContext->GetActor()->GetSlot());

                pOpponent = g_pGameEngine->GetPlayer(OPPOSING_FIRST(bySlot));

                const Vec3* const pNearAt = pOpponent->GetPositionPtr();

                bySlot = static_cast<unsigned char>(pContext->GetActor()->GetSlot());

                pOpponent = g_pGameEngine->GetPlayer(OPPOSING_SECOND(bySlot));

                const Vec3* const pFarAt = pOpponent->GetPositionPtr();

                const Vec3* const pOwnAt = pContext->GetActor()->GetPositionPtr();

                const auto flNearX = pNearAt->x - pOwnAt->x;
                const auto flNearY = pNearAt->y - pOwnAt->y;
                const auto flNearZ = pNearAt->z - pOwnAt->z;

                vFar.x = pFarAt->x - pOwnAt->x;
                vFar.y = pFarAt->y - pOwnAt->y;
                vFar.z = pFarAt->z - pOwnAt->z;

                flDistNear           = static_cast<float>(sqrt(static_cast<double>((flNearZ * flNearZ + flNearY * flNearY) + flNearX * flNearX)));
                const auto flDistFar = static_cast<float>(sqrt(static_cast<double>((vFar.z * vFar.z + vFar.y * vFar.y) + vFar.x * vFar.x)));

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
void PlanCond::NearestSuspenseMult(Interactor* pContext, PlanNode*, short* pResult, int)
{
    auto* const       pActor    = pContext->GetActor();
    const Vec3* const pPosition = pActor->GetPositionPtr();

    auto flBest = 200000000.0f;
    auto nBest  = kModelIDNone;

    for (int nSlot = kSlotMultiplier; nSlot <= kSlotMultiplierLast; nSlot++)
    {
        auto* const pMultiplier = g_pGameEngine->m_ppSlots[static_cast<short>(nSlot)];

        if (pMultiplier != nullptr)
        {
            const auto* const pszName = pMultiplier->GetPlanState()->m_pCurrent->m_pszName;

            // STRING: HYPERX 0x004bcac8
            if (strcmp(pszName, "Spin") != 0
                // STRING: HYPERX 0x004bcabc
                && strcmp(pszName, "ScoreSpin") != 0)
            {
                const Vec3* const pOther = pMultiplier->GetPositionPtr();

                const auto flDx = pOther->x - pPosition->x;
                const auto flDy = pOther->y - pPosition->y;
                const auto flDz = pOther->z - pPosition->z;

                const auto flRange = static_cast<float>(sqrt(static_cast<double>(flDx * flDx + flDy * flDy + flDz * flDz)));

                if (flRange < flBest)
                {
                    flBest = flRange;
                    nBest  = nSlot;
                }
            }
        }
    }

    if (nBest != kModelIDNone)
    {
        *pResult = static_cast<short>(nBest - (kSlotMultiplier - 1));
    }
    else
    {
        *pResult = 0;
    }
}

// FUNCTION: HYPERX 0x00461a50
void PlanCond::OffenseOrWRok(Interactor* pContext, PlanNode*, short* pResult, int)
{
    short nSlot;
    int   nWide;

    auto* const pSlot = g_pGameEngine->m_ppSlots[kSlotRok]->GetPhysicsBody()->pAttach;

    const auto bNoCarrier = pSlot == nullptr || pSlot->nSlot >= kPlayerObjectCount;

    if (bNoCarrier)
    {
        *pResult = 2;

        return;
    }

    nSlot = pSlot->nSlot;

    auto* pActor = pContext->GetActor();

    if (nSlot == pActor->GetSlot())
    {
        *pResult = 1;

        return;
    }

    pActor      = pContext->GetActor();
    auto nOther = pActor->GetSlot();

    if (nOther < 4)
    {
        nWide = nOther + 2;
    }
    else
    {
        nWide = nOther;
    }

    nOther = static_cast<short>(nWide & 3);
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
    auto* const pSlot = g_pGameEngine->m_ppSlots[kSlotRok]->GetPhysicsBody()->pAttach;

    if (pSlot != nullptr)
    {
        const auto nSlot = pSlot->nSlot;

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
    auto* const pSlot = g_pGameEngine->m_ppSlots[kSlotRok]->GetPhysicsBody()->pAttach;

    if (pSlot != nullptr)
    {
        const auto nSlot = pSlot->nSlot;

        if (nSlot < 6)
        {
            *pResult = static_cast<signed char>(nSlot & 1);

            return;
        }
    }

    *pResult = 2;
}

// Zero when the rok is loose, else one to four by which skater holds it relative to the actor; a goalie holder leaves the answer alone.

// FUNCTION: HYPERX 0x00461b60
void PlanCond::WhoHasRok(Interactor* pContext, PlanNode*, short* pResult, int)
{
    short nSlot;

    auto* const pActor = pContext->GetActor();
    auto* const pSlot  = g_pGameEngine->m_ppSlots[kSlotRok]->GetPhysicsBody()->pAttach;

    if (pSlot != nullptr)
    {
        nSlot = pSlot->nSlot;

        if (nSlot < kPlayerObjectCount)
        {
            const int nTeamWide = pActor->GetSlot();
            auto      nTeam     = static_cast<short>(nTeamWide);
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

            if (nSlot == static_cast<short>(nOther & 3))
            {
                *pResult = 2;

                return;
            }

            nTeam = static_cast<short>(~nTeam & 1);

            if (nSlot == nTeam)
            {
                *pResult = 3;

                return;
            }

            if (nSlot == static_cast<short>(nTeam | 2))
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
    if (g_pGameEngine->m_ppSlots[kSlotRok]->GetPhysicsBody()->pAttach != nullptr)
    {
        *pResult = 2;

        return;
    }

    auto* const pActor = pContext->GetPlayerActor();

    // Only PlayerObject has m_wTargetGoalSlot.
    *pResult = pActor->m_wTargetGoalSlot == 0;
}

// The low nibble of the flag launches the pass; clear, it only announces it.
constexpr auto kLaunchMask = 0x0f;

constexpr auto kLatePassStep = 4;

#define BUNGEE_GAIN     1.85f
#define BUNGEE_DOT_GATE 0.7f

// The word at 0x004d2b82, named and marked in src/planvm/planactionrecoil.cpp.
extern short g_nCheckImpulseFacing;

// FUNCTION: HYPERX 0x00461c40
void PlanCond::BungeeVictimLorR(Interactor* pContext, PlanNode*, short* pResult, int nFlags)
{
    EngineObject*  pTarget;
    CBungeeToward* pBungee;
    float          flScore;

    if ((nFlags & kLaunchMask) != 0)
    {
        pTarget = Player::FindBungeeVictim(pContext->GetActor(), &flScore);

        *pResult = 0;

        if (pTarget != nullptr)
        {
            pBungee = (CBungeeToward*)AllocAgendaItem(Agenda_CBungeeToward);

            auto* const pBindNode = pContext->GetActor()->GetPlanState()->m_pCurrent;

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

        const int nLate = (g_nCheckImpulseFacing >= kLatePassStep);

        if (pTarget != nullptr)
        {
            g_pGameEngine->m_ppSlots[kSlotSoundMan]->PostSceneEvent(kMsgYerAttacked, 0);
        }

        *pResult = static_cast<short>(nLate + 1);
    }
}

// The stat handler is asked twice with the same index and its two answers are used differently.

// FUNCTION: HYPERX 0x00461d20
void PlanAction::IncrementStat(Interactor* pContext, PlanNode* pArgs, short*, int)
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
void PlanAction::StartStatTimer(Interactor* pContext, PlanNode* pArgs, short*, int)
{
    auto* const pBlock = pArgs->m_pArgs;

    if (pBlock[1].IsPronoun())
    {
        const short nSlot = g_apPlanStatFns[pBlock[1].wPronoun](
            pContext->GetActor(), pArgs);
        auto* const pEntity = g_pGameEngine->GetPlayer(nSlot);

        if (nSlot >= 0 && nSlot < kEntitySlotLimit && pEntity != nullptr)
        {
            pEntity->m_matchStats.StartTimer(pArgs->m_pArgs[2].n);
        }
    }
}

// FUNCTION: HYPERX 0x00461e30
void PlanAction::EndStatTimer(Interactor* pContext, PlanNode* pArgs, short*, int)
{
    auto* const pBlock = pArgs->m_pArgs;

    if (pBlock[1].IsPronoun())
    {
        const short nSlot = g_apPlanStatFns[pBlock[1].wPronoun](
            pContext->GetActor(), pArgs);
        auto* const pEntity = g_pGameEngine->GetPlayer(nSlot);

        if (nSlot >= 0 && nSlot < kEntitySlotLimit && pEntity != nullptr)
        {
            pEntity->m_matchStats.StopTimer(pArgs->m_pArgs[2].n);
        }
    }
}

// FUNCTION: HYPERX 0x00461ea0
void PlanAction::ScaleBubToRating(Interactor* pContext, PlanNode* pNode, short*, int)
{
    auto* const pColObj = g_pGameEngine->GetColObjPtr(static_cast<short>(pNode->m_pArgs[kArgColObject].n));

    auto* const pActor = pContext->GetPlayerActor();

    const auto flStat = pActor->m_stats.GetScalar(pNode->m_pArgs[ARG_STAT].n);

    const auto flLow = PlanArg::ResolveFloat(pContext, pNode, 3);

    pColObj->m_flRadius = (PlanArg::ResolveFloat(pContext, pNode, 4) - flLow) * (flStat * g_flPlanStatScale) + flLow;
}

// Slot one on the tracked object answers a short, read here as a slot and elsewhere as a team.

// FUNCTION: HYPERX 0x00461f30
void PlanAction::SetTargetWithData(Interactor* pContext, PlanNode*, short*, int nArg)
{
    const auto nIndex = static_cast<short>(nArg);

    Vec3 vToTracked;
    Vec3 vToCandidate;

    auto*       pActor     = pContext->GetActor();
    auto*       pPlanState = pActor->GetPlanState();
    auto*       pSteer     = (CSteerToTarget*)pPlanState->m_pCurrent->m_apSlots[0];
    auto* const pTracked   = pSteer->m_pTarget;

    auto* const pCandidate = g_pGameEngine->m_ppSlots[nIndex];

    if (pTracked != nullptr)
    {
        pActor = pContext->GetActor();

        auto* const pSelfPos      = pActor->GetPositionPtr();
        auto* const pTrackedPos   = pTracked->GetPositionPtr();
        auto* const pCandidatePos = pCandidate->GetPositionPtr();

        vToTracked.x = pSelfPos->x - pTrackedPos->x;
        vToTracked.y = pSelfPos->y - pTrackedPos->y;
        vToTracked.z = pSelfPos->z - pTrackedPos->z;

        vToCandidate.x = pSelfPos->x - pCandidatePos->x;
        vToCandidate.y = pSelfPos->y - pCandidatePos->y;
        vToCandidate.z = pSelfPos->z - pCandidatePos->z;

        const auto nTracked = pTracked->GetSlot();

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
void PlanAction::BungeeTowardTarget(Interactor* pContext, PlanNode*, short*, int)
{
    EngineObject* pTracked;

    pTracked = ((CBungeeToward*)pContext->GetActor()->GetPlanState()->m_pCurrent->m_apSlots[0])->m_pTarget;

    if (pTracked != nullptr)
    {
        auto* const pTrack = (CBungeeToward*)AllocAgendaItem(Agenda_CBungeeToward);

        auto* const pBindNode = pContext->GetActor()->GetPlanState()->m_pCurrent;

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
void PlanCond::Decide_PassOrRun(Interactor* pContext, PlanNode*, short* pResult, int)
{
    float flSign;
    short nTracked;

    const auto nSlot = pContext->GetActor()->GetSlot();

    const auto nMate = static_cast<short>(TeammateSlot(nSlot));

    EngineObject** const apObjects = g_pGameEngine->m_ppSlots;

    auto* const pActor = (PlayerObject*)apObjects[nSlot];

    if (pActor->m_wTargetGoalSlot == kSlotNgoal)
    {
        flSign = g_flOnePlan;
    }
    else
    {
        flSign = -g_flOnePlan;
    }

    auto* const pBlock = apObjects[kSlotRok]->GetPhysicsBody();

    auto* const pnTracked = pBlock->pAttach;

    if (pnTracked != nullptr && (nTracked = pnTracked->nSlot) < kPlayerObjectCount && nTracked == nSlot)
    {
        auto* const pMate = g_pGameEngine->m_ppSlots[nMate];

        const auto flLimit = flSign * pActor->GetPositionPtr()->y + 20.0f;

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
void PlanCond::Decide_HPSpin_Rail_orNot(Interactor* pContext, PlanNode*, short* pResult, int)
{
    AttachPak* pnTracked;
    short      nTracked;

    auto* const pBlock = pContext->GetActor()->GetPhysicsBody();

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

    pnTracked = g_pGameEngine->m_ppSlots[kSlotRok]->GetPhysicsBody()->pAttach;

    if (pnTracked != nullptr && (nTracked = pnTracked->nSlot) < kPlayerObjectCount && nTracked == pContext->GetActor()->GetSlot())
    {
        const auto flEnergy = pContext->GetPlayerActor()->m_stats.m_aStats[0];

        auto* const pDest = pContext->GetActor();

        auto flRaised = flEnergy;

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
void PlanAction::KickOffRim(Interactor* pContext, PlanNode*, short*, int)
{
    Vec3 vDirection;

    auto* const pActor    = pContext->GetActor();
    auto* const pBody     = pActor->GetPhysicsBody();
    const auto  vPosition = *pActor->GetPositionPtr();

    vDirection.x = -vPosition.x;
    vDirection.y = -vPosition.y;
    vDirection.z = 1.0f;

    Math::Vec3Normalize(&vDirection);

    vDirection.z += 1.0f;

    const auto flStep = g_pGameEngine->m_clock.flDelta * 15.0f;

    pBody->vel.x = flStep * vDirection.x + pBody->vel.x;
    pBody->vel.y = flStep * vDirection.y + pBody->vel.y;
    pBody->vel.z = flStep * vDirection.z + pBody->vel.z;
}

// The two axis tests answer two and one, and the answer is their sum.

// FUNCTION: HYPERX 0x00462360
void PlanCond::PickACell(Interactor* pContext, PlanNode*, short* pResult, int)
{
    auto* const pActor    = pContext->GetActor();
    auto* const pPosition = pActor->GetPositionPtr();

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
    *pResult = static_cast<short>(static_cast<long long>(rand()) * (pArgs->m_nArgCount - 1) / (static_cast<long long>(RAND_MAX) + 1));
}

// Turns the head camera's shot power into a child index, inverted and clamped.

// FUNCTION: HYPERX 0x004623f0
void PlanCond::ShotPower(Interactor*, PlanNode* pArgs, short* pResult, int)
{
    const auto nScaled = g_pGameEngine->GetCamera(kCameraHeadSlot)->m_nShotPowerLevel * (pArgs->m_nArgCount - 1);

    const auto nPosition = static_cast<int>(static_cast<double>(nScaled) * g_dPeriodScale);

    *pResult = static_cast<short>(1 - nPosition);

    if (*pResult > pArgs->m_nArgCount - 2)
    {
        *pResult = static_cast<short>(pArgs->m_nArgCount - 2);
    }
}

constexpr auto kSpawnGridHalf  = 28;
constexpr auto kSpawnGridPitch = 56;

constexpr auto kSpawnSlotLimit = 3;
constexpr auto kSpawnAxes      = 4;

constexpr auto kSpawnResultOut        = 1;
constexpr auto kSpawnResultTeamMoving = 2;

// The third axis difference is always zero: both operands are the same shared zero.
// FUNCTION: HYPERX 0x00462450
void PlanCond::IfInFaceOff(Interactor*, PlanNode*, short* pResult, int)
{
    Vec3  vSquare;
    float flLimit;
    int   nSlot;

    *pResult = 0;

    for (nSlot = 0; static_cast<short>(nSlot) <= kSpawnSlotLimit; nSlot++)
    {
        auto* const pObject = g_pGameEngine->GetPlayer(static_cast<short>(nSlot));

        vSquare.x = static_cast<float>(kSpawnGridHalf - static_cast<short>(nSlot) / 2 * kSpawnGridPitch);
        vSquare.y = static_cast<float>(kSpawnGridHalf - static_cast<short>(nSlot) % 2 * kSpawnGridPitch);
        vSquare.z = g_flZeroPlan;

        const auto vPosition = *pObject->GetPositionPtr();

        const auto flDx = vPosition.x - vSquare.x;
        const auto flDy = vPosition.y - vSquare.y;
        const auto flDz = g_flZeroPlan - vSquare.z;

        const auto flSq = flDx * flDx;

        if ((flSq + flDy * flDy) + flDz * flDz > g_flSpawnEscapeDistance)
        {
            flLimit = DamageGate();

            if (pObject->m_stats.GetScalar(kRatingDamage) > flLimit)
            {
                *pResult = kSpawnResultOut;
                return;
            }

            auto nMoving = 0;
            auto nAxis   = 0;
            auto nLeft   = kSpawnAxes;

            do
            {
                if (g_pGameEngine->IsFitSubstitute(static_cast<short>(nSlot), nAxis) != 0)
                {
                    nMoving++;
                }

                nAxis++;
                nLeft--;
            } while (nLeft != 0);

            if (nMoving != 0)
            {
                *pResult = kSpawnResultOut;
                return;
            }
        }
    }

    if (g_pGameEngine->IsHomeTeamInjured() != 0 || g_pGameEngine->IsAwayTeamInjured() != 0)
    {
        *pResult = kSpawnResultTeamMoving;
    }
}

constexpr auto kArenaTrackUp   = 7;
constexpr auto kArenaTrackDown = 8;

// FUNCTION: HYPERX 0x00462620
void PlanAction::EndPeriodNow(Interactor*, PlanNode*, short*, int)
{
    g_pGameEngine->m_bEndPeriodRequested = 1;
}

// Three when both teams say yes, one for the first only, two for the second only, else zero.

// FUNCTION: HYPERX 0x00462630
void PlanCond::TeamIsDead(Interactor*, PlanNode*, short* pResult, int)
{
    const auto bHome = g_pGameEngine->IsHomeTeamDead();
    const auto bAway = g_pGameEngine->IsAwayTeamDead();

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
    const auto bHome = g_pGameEngine->IsHomeTeamInjured();
    const auto bAway = g_pGameEngine->IsAwayTeamInjured();

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
    auto* const pActor = pContext->GetActor();

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
        *pResult = static_cast<short>(g_pGameEngine->GetHomeScore() <= g_pGameEngine->GetAwayScore());
    }
}

// Two is the answer when the target and the actor are the same object.

// FUNCTION: HYPERX 0x00462780
void PlanCond::IsSenderTeammate(Interactor* pContext, PlanNode*, short* pResult, int)
{
    // Not TeammateSlot: the client adds two without an else arm (0x00462794 to 0x004627a8).
    const auto nOwnSlot = pContext->GetActor()->GetSlot();

    int nSlot = nOwnSlot;

    if (nOwnSlot < kPlayerSlotCount)
    {
        nSlot += 2;
    }

    nSlot &= kPlayerSlotMask;

    *pResult = pContext->GetTargetSlot() != static_cast<short>(nSlot);

    auto* const pActor = pContext->GetActor();

    if (pContext->GetTargetSlot() == pActor->GetSlot())
    {
        *pResult = 2;
    }
}

// FUNCTION: HYPERX 0x004627f0
void PlanCond::AmMainDromer(Interactor* pContext, PlanNode*, short* pResult, int)
{
    auto* const pActor = pContext->GetActor();

    *pResult = pActor->GetSlot() >= 2;
}

// FUNCTION: HYPERX 0x00462820
void PlanCond::MyID(Interactor* pContext, PlanNode*, short* pResult, int)
{
    auto* pActor = pContext->GetActor();

    if (pActor->GetSlot() < kPlayerObjectCount)
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

    pObject            = g_pGameEngine->m_ppSlots[g_pGameEngine->m_nViewedSlot];
    auto* const pState = pObject->GetPhysicsBody();

    *pResult = (static_cast<char>(pState->nHeldBindings) & HELD_ROTATE) == 0;
}

// FUNCTION: HYPERX 0x00462910
void PlanCond::AmViewOwner(Interactor* pContext, PlanNode*, short* pResult, int)
{
    // Nought when the actor is either tracked object.
    *pResult = pContext->GetActor()->GetSlot() != g_pGameEngine->m_apTracked[0]->GetSlot() && pContext->GetActor()->GetSlot() != g_pGameEngine->m_apTracked[1]->GetSlot();
}

// An actor on the Input plan stands in for the viewed slot when finding its team mate.
// FUNCTION: HYPERX 0x00462990
void PlanCond::TeammateIsViewOwner(Interactor* pContext, PlanNode*, short* pResult, int)
{
    auto* const pActor = pContext->GetActor();

    int nTeam = pActor->GetSlot();

    const char* const pszName = pActor->GetPlanState()->m_pszName;

    // STRING: HYPERX 0x004c45f4
    if (strcmp("Input", pszName) == 0)
    {
        nTeam = g_pGameEngine->m_nViewedSlot;
    }

    *pResult = static_cast<unsigned short>(
        // STRING: HYPERX 0x004bb340
        strcmp("Human",
               g_pGameEngine->m_ppSlots[TeammateSlot(static_cast<short>(nTeam))]
                   ->GetPlanState()
                   ->m_pszName) != 0);
}

// Answers one for a bot team mate that is fit and wants no substitute, else nought.
// FUNCTION: HYPERX 0x00462a10
void PlanCond::TeammateIsHumanOrInjured(Interactor* pContext, PlanNode*, short* pResult, int)
{
    short nIsBot;

    auto* const pActor = pContext->GetActor();

    int nTeam = pActor->GetSlot();

    const char* const pszName = pActor->GetPlanState()->m_pszName;

    // STRING: HYPERX 0x004c45f4
    if (strcmp("Input", pszName) == 0)
    {
        nTeam = g_pGameEngine->m_nViewedSlot;
    }

    const auto nSlot = static_cast<short>(TeammateSlot(static_cast<short>(nTeam)));

    nIsBot = static_cast<short>(
        // STRING: HYPERX 0x004bb340
        strcmp("Human",
               g_pGameEngine->m_ppSlots[nSlot]->GetPlanState()->m_pszName) != 0);

    *pResult = nIsBot;

    if (nIsBot == 1)
    {
        auto* const pBody = g_pGameEngine->GetPlayer(nSlot);

        const auto flDamage = pBody->m_stats.GetScalar(kRatingDamage);

        if (flDamage <= DamageGate() || g_pGameEngine->GetPlayer(nSlot)->m_wDesiredSubstitute != -1)
        {
            *pResult = 0;
        }
    }
}

// FUNCTION: HYPERX 0x00462b30
void PlanCond::OkToSwitchToTeammate(Interactor* pContext, PlanNode*, short* pResult, int)
{
    const auto nTeam = pContext->GetActor()->GetSlot();

    const auto nSlot = static_cast<short>(TeammateSlot(nTeam));

    *pResult = static_cast<short>(
        // STRING: HYPERX 0x004bb340
        strcmp("Human",
               g_pGameEngine->m_ppSlots[nSlot]->GetPlanState()->m_pszName) != 0);

    auto* const pOther = g_pGameEngine->GetPlayer(nSlot);

    auto* const pActor = pContext->GetPlayerActor();

    if (*pResult == 1)
    {
        const auto flDamage = pOther->m_stats.GetScalar(kRatingDamage);

        if (flDamage <= DamageGate() || pActor->m_wClockStoppedSemaphore != 0 || pOther->m_wClockStoppedSemaphore != 0 || pOther->m_wDesiredSubstitute != -1)
        {
            *pResult = 0;
        }
    }
}

// FUNCTION: HYPERX 0x00462c40
void PlanCond::OkToPassToTeammate(Interactor* pContext, PlanNode*, short* pResult, int)
{
    const auto nTeam = pContext->GetActor()->GetSlot();

    const auto nSlot = static_cast<short>(TeammateSlot(nTeam));

    auto* const pActor = pContext->GetPlayerActor();

    auto* const pOther = g_pGameEngine->GetPlayer(nSlot);

    *pResult = 1;

    auto* const pBody = g_pGameEngine->GetPlayer(nSlot);

    const auto flDamage = pBody->m_stats.GetScalar(kRatingDamage);

    if (flDamage <= DamageGate() || pActor->m_wClockStoppedSemaphore != 0 || pOther->m_wClockStoppedSemaphore != 0 || pOther->m_wDesiredSubstitute != -1)
    {
        *pResult = 0;
    }
}

// Five below the damage gate and four for the actor's m_wClockStoppedSemaphore, before the rok is asked.

// The slot two along, wrapped: zero pairs with two and one with three.
constexpr auto kSlotOppositeStep  = 2;
constexpr auto kSlotOppositeWrap  = 3;
constexpr auto kSlotOppositeBelow = 4;

// The half of the four the tail answers with.
#define SLOT_FAR_HALF 2

constexpr auto kRokTargeted = 2;

constexpr auto kStateHeld  = 4;
constexpr auto kStateGated = 5;

// FUNCTION: HYPERX 0x00462d20
void PlanCond::DecidePosition(Interactor* pContext, PlanNode*, short* pResult, int)
{
    AttachPak* pnTargetSlot;
    int        nOpposite;
    float      flLimit;

    auto* const pActor = pContext->GetPlayerActor();

    const auto flDamage = pActor->m_stats.GetScalar(kRatingDamage);

    flLimit = DamageGate();

    if (flDamage <= flLimit)
    {
        *pResult = kStateGated;

        return;
    }

    if (pActor->m_wClockStoppedSemaphore != 0)
    {
        *pResult = kStateHeld;

        return;
    }

    pnTargetSlot = g_pGameEngine->m_ppSlots[kSlotRok]->GetPhysicsBody()->pAttach;

    if (pnTargetSlot != nullptr)
    {
        const auto nTarget = pnTargetSlot->nSlot;

        if (nTarget < kPlayerObjectCount)
        {
            const auto nOwn = pContext->GetActor()->GetSlot();

            if (nTarget != nOwn)
            {
                const auto nOther = pContext->GetActor()->GetSlot();

                if (nOther < kSlotOppositeBelow)
                {
                    nOpposite = nOther + kSlotOppositeStep;
                }
                else
                {
                    nOpposite = nOther;
                }

                if (nTarget != static_cast<short>(nOpposite & kSlotOppositeWrap))
                {
                    *pResult = static_cast<unsigned short>(pContext->GetActor()->GetSlot() >= SLOT_FAR_HALF);

                    return;
                }
            }

            *pResult = static_cast<unsigned short>((nTarget != pContext->GetActor()->GetSlot()) + kRokTargeted);

            return;
        }
    }

    *pResult = static_cast<unsigned short>(pContext->GetActor()->GetSlot() >= SLOT_FAR_HALF);
}

// FUNCTION: HYPERX 0x00462e50
void PlanCond::CollisionDetail(Interactor* pContext, PlanNode*, short* pResult, int)
{
    // The first object is fetched only to be checked: nothing reads it afterwards.
    g_pGameEngine->GetColObjPtr(pContext->GetOtherSlot());

    auto* const pCollision = g_pGameEngine->GetColObjPtr(pContext->GetModelSlot());

    auto* const pBody = pCollision->GetOwner()->GetPhysicsBody();

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
    auto* const pSlot = g_pGameEngine->m_ppSlots[kSlotRok]->GetPhysicsBody()->pAttach;

    if (pSlot != nullptr)
    {
        auto nSlot = pSlot->nSlot;

        if (nSlot < 6 && nSlot == g_pGameEngine->m_nViewedSlot)
        {
            *pResult = 0;

            return;
        }
    }

    *pResult = 1;
}

constexpr auto kSlotWrap   = 0x03;
constexpr auto kSlotPaired = 0x04;

// FUNCTION: HYPERX 0x00462f20
void PlanCond::ViewOwnersMateInFront(Interactor*, PlanNode*, short* pOutFacing, int)
{
    Vec3* pThere;
    short nFacing;
    int   nIndex;
    Vec3  vToThere;

    auto* const pTracked = g_pGameEngine->m_ppSlots[g_pGameEngine->m_nViewedSlot];

    auto* const pHere = pTracked->GetPositionPtr();

    const auto nSlot = g_pGameEngine->m_nViewedSlot;

    if (nSlot < kSlotPaired)
    {
        nIndex = nSlot + 2;
    }
    else
    {
        nIndex = nSlot;
    }

    nIndex &= kSlotWrap;

    pThere = g_pGameEngine->m_ppSlots[nIndex]->GetPositionPtr();

    vToThere.x = pThere->x - pHere->x;
    vToThere.y = pThere->y - pHere->y;
    vToThere.z = pThere->z - pHere->z;

    const Vec3* const pForward = &pTracked->GetPhysicsBody()->velDir;

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
void PlanCond::IHaveRok(Interactor* pContext, PlanNode*, short* pResult, int nFlag)
{
    const AttachPak* pCarrier;
    short            nCarrier;

    if (pContext->GetOtherSlot() < 0)
    {
        // The flagged query answers two and three where the plain one answers nought and one.
        if (nFlag != 0)
        {
            pCarrier = g_pGameEngine->m_ppSlots[kSlotRok]->GetPhysicsBody()->pAttach;

            if (pCarrier != nullptr && (nCarrier = pCarrier->nSlot) < kPlayerObjectCount && nCarrier == pContext->GetActor()->GetSlot())
            {
                *pResult = 2;

                return;
            }

            *pResult = 3;

            return;
        }

        pCarrier = g_pGameEngine->m_ppSlots[kSlotRok]->GetPhysicsBody()->pAttach;

        if (pCarrier != nullptr && (nCarrier = pCarrier->nSlot) < kPlayerObjectCount && nCarrier == pContext->GetActor()->GetSlot())
        {
            *pResult = 0;

            return;
        }

        *pResult = 1;

        return;
    }

    pCarrier = g_pGameEngine->m_ppSlots[kSlotRok]->GetPhysicsBody()->pAttach;

    if (pCarrier != nullptr && (nCarrier = pCarrier->nSlot) < kPlayerObjectCount && nCarrier == CollisionObj::GetModelId(pContext->GetModelSlot()))
    {
        *pResult = 0;

        return;
    }

    *pResult = 1;
}

// Inverted: zero when the carrier matches; the tag test cannot fire, as the slot is narrowed first.
// FUNCTION: HYPERX 0x004630d0
void PlanCond::DromerHasRok(Interactor* pContext, PlanNode* pArgs, short* pResult, int nPrevSlot)
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
    if (static_cast<unsigned short>(static_cast<unsigned int>(static_cast<short>(nSlot)) >> 16) == kPlanArgPronoun)
    {
        nSlot = (reinterpret_cast<PlanStatFnWide*>(g_apPlanStatFns))[nSlot & 0xffff](pContext->GetActor(), pArgs);
    }

    auto* const pCarrier = g_pGameEngine->m_ppSlots[kSlotRok]->GetPhysicsBody()->pAttach;

    if (pCarrier != nullptr && pCarrier->nSlot < kPlayerObjectCount && pCarrier->nSlot == static_cast<short>(nSlot))
    {
        *pResult = 0;
        return;
    }

    *pResult = 1;
}

// FUNCTION: HYPERX 0x00463170
void PlanCond::SetResultDirect(Interactor*, PlanNode*, short* pResult, int nValue)
{
    *pResult = static_cast<short>(nValue);
}

// FUNCTION: HYPERX 0x00463180
void PlanCond::IsRokInMyZone(Interactor* pContext, PlanNode*, short* pResult, int)
{
    int nAnswer;

    if ((pContext->GetActor()->GetSlot() & 1) != 0)
    {
        nAnswer = g_pGameEngine->m_ppSlots[kSlotRok]->GetPositionPtr()->y > g_flZeroPlan;
    }
    else
    {
        nAnswer = g_pGameEngine->m_ppSlots[kSlotRok]->GetPositionPtr()->y < g_flZeroPlan;
    }

    *pResult = static_cast<short>(nAnswer);
}

// FUNCTION: HYPERX 0x004631f0
void PlanAction::SetRenderFlag(Interactor* pContext, PlanNode* pArgs, short*, int)
{
    EngineObject* pActor;

    if (pArgs->m_pArgs[1].n != 0)
    {
        pActor = pContext->GetActor();

        pActor->m_nFlags &= ~kEngineObjectHidden;
    }
    else
    {
        pActor = pContext->GetActor();

        pActor->m_nFlags |= kEngineObjectHidden;
    }
}

// The upper arm announces once more going up than the lower one does coming down.

// FUNCTION: HYPERX 0x00463300
void PlanAction::IncRokValue(Interactor*, PlanNode* pArgs, short*, int nArg)
{
    const auto nFlag = static_cast<unsigned char>(nArg);

    short nRequest;

    EngineObject** const pSlots = g_pGameEngine->m_ppSlots;
    auto* const          pRok   = (RokObject*)pSlots[kSlotRok];
    auto                 nMode  = pRok->m_wRokValue;

    if (nMode == 1)
    {
        nMode = 0;
    }

    if (pArgs->m_pArgs[1].n == PLAN_TOKEN_MESSAGE_DATA)
    {
        nRequest = static_cast<short>(((nFlag & 1) > 0) + 7);
    }
    else
    {
        nRequest = static_cast<short>(pArgs->m_pArgs[1].n);
    }

    if (nRequest == kArenaTrackUp)
    {
        nMode += 2;

        if (nMode <= 4)
        {
            pSlots[kSlotSoundMan]->PostSceneEvent(kMsgDoubleScore, 0);
        }

        if (nMode >= 4)
        {
            nMode = 4;
        }
    }
    else if (nRequest == kArenaTrackDown)
    {
        nMode -= 2;

        if (nMode >= -4)
        {
            pSlots[kSlotSoundMan]->PostSceneEvent(kMsgDoubleScore, 1);
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

// The original narrows the argument to a short before testing it for the message-data token, so the
// test never matches and the event's own value is never taken.
// FUNCTION: HYPERX 0x004633c0
void PlanAction::SetRokKillState(Interactor*, PlanNode* pArgs, short*, int)
{
    const auto  nValue = static_cast<short>(pArgs->m_pArgs[1].n);
    auto* const pRok   = g_pGameEngine->GetRok();

    if (nValue == -1)
    {
        pRok->m_nKillState = -1;

        pRok->SetRenderSlotFlags();

        g_pGameEngine->m_ppSlots[kSlotSoundMan]->PostSceneEvent(kMsgKillBallChange, 0);

        return;
    }

    pRok->m_nKillState = static_cast<unsigned char>(~static_cast<unsigned char>(nValue)) & 1;

    pRok->SetRenderSlotFlags();

    g_pGameEngine->m_ppSlots[kSlotSoundMan]->PostSceneEvent(kMsgKillBallChange, 1);

    pRok->SetEntitySlot(pRok->m_nCarrierSlot);
}

// Any kind but the randomizers indexes by the low word of the argument block's own address.

// The frame the first saved row is pinned to.
constexpr auto kColorPinnedFrame = 0x78;

// One word a colour row, set to one so the first save wins; marked in src/globals.cpp.
extern short g_awColorLatched[2];

// Two rows of twenty saved colours. src/globals.cpp carries the marker.
extern int g_aaColorSaved[2][20];

// FUNCTION: HYPERX 0x00463450
void PlanAction::GrayscaleRandomizer(Interactor* pContext, PlanNode* pArgs, short*, int)
{
    EngineObject* pObject;
    auto          nValue = 0;
    PaletteRange  aRange;

    const auto nEnable = static_cast<short>(pArgs->m_pArgs[1].n);

    if (pContext->GetNamedSlot() == -1)
    {
        auto* const pCol = g_pGameEngine->GetColObjPtr(pContext->GetOtherSlot());

        pObject = pCol->m_pOwner;
    }
    else
    {
        pObject = pContext->GetActor();
    }

    // The original leaves the index unset for any other kind; zero picks the first row.
    if (pObject->m_nKind == KIND_GREEN_RANDOMIZER)
    {
        nValue = 1;
    }

    if (pObject->m_nKind == KIND_RED_RANDOMIZER)
    {
        nValue = 0;
    }

    const auto nIndex = static_cast<short>(nValue);

    if (g_awColorLatched[nIndex] != 0)
    {
        for (int nMat = 0; nMat < pObject->m_pModelBlock->m_nMatCount; nMat++)
        {
            g_aaColorSaved[nIndex][nMat] = pObject->m_pModelBlock->m_pMaterials[nMat].nColour;

            if (nIndex == 0)
            {
                g_aaColorSaved[0][1] = kColorPinnedFrame;
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
constexpr auto kArgGuard = 1;
#define ARG_X 2
#define ARG_Y 3
constexpr auto kArgZ                 = 4;
constexpr auto kArgCountWithPosition = 5;

// FUNCTION: HYPERX 0x004635e0
void PlanAction::SetLocalReal(Interactor* pContext, PlanNode* pArgs, short*, int)
{
    const PlanWord* const pBlock  = pArgs->m_pArgs;
    const auto            flValue = pBlock[2].fl;
    const auto            nIndex  = static_cast<short>(pBlock[1].n);

    auto* const pActor  = pContext->GetActor();
    auto* const pLocals = pActor->GetPlanState();

    pLocals->m_pflLocalReals[nIndex] = flValue;
}

// The CTrackTraj belongs to the plan's object; the position goes to the camera in slot nine.

// FUNCTION: HYPERX 0x00463620
void PlanAction::SetInDrag(Interactor* pContext, PlanNode* pNode, short*, int)
{
    if (pNode->m_pArgs[kArgGuard].n != 0)
    {
        auto* const pActor = pContext->GetActor();

        auto* const pPlanState = pActor->GetPlanState();

        static_cast<CTrackTraj*>(pPlanState->m_pCurrent->m_apSlots[0])->InitComplexRange();

        if (pNode->m_nArgCount == kArgCountWithPosition)
        {
            auto* pCamera = g_pGameEngine->m_ppSlots[kCameraMainSlot];

            auto* const pPos = pCamera->GetPositionPtr();

            pCamera = g_pGameEngine->m_ppSlots[kCameraMainSlot];

            auto* const pBody = &pCamera->GetPhysicsBody()->pos;

            pPos->x = pBody->x = static_cast<float>(static_cast<unsigned int>(pNode->m_pArgs[ARG_X].n));

            pPos->y = pBody->y = static_cast<float>(static_cast<unsigned int>(pNode->m_pArgs[ARG_Y].n));

            pPos->z = pBody->z = static_cast<float>(static_cast<unsigned int>(pNode->m_pArgs[kArgZ].n));
        }
    }
}

// Argument one is a mode that doubles as the range's low end; two is its high end.
constexpr auto kArgRimMode        = 1;
constexpr auto kArgHigh           = 2;
constexpr auto kArgCountWithRange = 3;

// With both arguments a mode of nought picks the directed range and any other the complex one.
// FUNCTION: HYPERX 0x004636e0
void PlanAction::SetOnRim(Interactor*, PlanNode* pNode, short*, int)
{
    EngineObject* pActor;
    PlanState*    pPlanState;

    if (pNode->m_nArgCount == kArgCountWithRange)
    {
        if (pNode->m_pArgs[kArgRimMode].n == 0)
        {
            pActor = g_pGameEngine->m_ppSlots[kCameraMainSlot];

            pPlanState = pActor->GetPlanState();

            static_cast<CTrackTraj*>(pPlanState->m_pCurrent->m_apSlots[0])->SetDirectedRange(static_cast<float>(static_cast<unsigned int>(pNode->m_pArgs[kArgRimMode].n)), static_cast<float>(static_cast<unsigned int>(pNode->m_pArgs[kArgHigh].n)));

            return;
        }

        pActor = g_pGameEngine->m_ppSlots[kCameraMainSlot];

        pPlanState = pActor->GetPlanState();

        static_cast<CTrackTraj*>(pPlanState->m_pCurrent->m_apSlots[0])->InitComplexRange();

        return;
    }

    pActor = g_pGameEngine->m_ppSlots[kCameraMainSlot];

    pPlanState = pActor->GetPlanState();

    static_cast<CTrackTraj*>(pPlanState->m_pCurrent->m_apSlots[0])->InitRange();
}

constexpr auto kArgValue = 1;

constexpr auto kSteerScaleThreshold = 0x19;

// The camera's plan sets the blend fraction; any other object takes a hundredth as its friction.
// FUNCTION: HYPERX 0x00463790
void PlanAction::SetFriction(Interactor* pContext, PlanNode* pNode, short*, int)
{
    auto* pActor = pContext->GetActor();

    if (pActor->GetSlot() != kCameraMainSlot)
    {
        pActor = pContext->GetActor();

        const auto flParam = static_cast<float>(static_cast<unsigned int>(pNode->m_pArgs[kArgValue].n)) * 0.01f;

        pActor->GetPhysicsBody()->flFriction = flParam;

        if (static_cast<unsigned int>(pNode->m_pArgs[kArgValue].n) > kSteerScaleThreshold)
        {
            pActor = pContext->GetActor();

            pActor->GetPhysicsBody()->flSteerScale = g_flOnePlan;
        }
    }
    else
    {
        auto* const pPlanState = g_pGameEngine->m_ppSlots[kCameraMainSlot]->GetPlanState();

        static_cast<CTrackTraj*>(pPlanState->m_pCurrent->m_apSlots[0])
            ->SetBlendT(pNode->m_pArgs[kArgValue].n);
    }
}

constexpr auto kArgCountWithoutSlot = 3;

// The argument block's own fields: which rating, how much, and which slot.
#undef ARG_STAT
#define ARG_STAT 1
constexpr auto kArgAmount = 2;
#undef ARG_SLOT
#define ARG_SLOT 3

// The top of the range every amount is clamped to; the bottom is nought.
#define STAT_CEILING 10.0f

// FUNCTION: HYPERX 0x00463840
void PlanAction::SetMyGravity(Interactor* pContext, PlanNode* pArgs, short*, int)
{
    auto* const pActor = pContext->GetActor();

    const auto flValue = static_cast<float>(static_cast<unsigned int>(pArgs->m_pArgs[1].n));

    pActor->GetPhysicsBody()->flGravAccel = flValue;
}

// FUNCTION: HYPERX 0x00463880
void PlanAction::ModifyAvailableReds(Interactor*, PlanNode* pArgs, short*, int)
{
    int nReds;

    const auto nJudge = static_cast<int>(g_pGameEngine->m_ppSlots[kSlotRedRandomizer]->GetPhysicsBody()->axis.x);

    nReds = (std::max)(kJudgeStateLow,
                       (std::min)(kJudgeStateHigh, static_cast<int>(static_cast<short>(nJudge)) + static_cast<int>(static_cast<short>(pArgs->m_pArgs[1].n))));

    g_pGameEngine->m_ppSlots[kSlotRedRandomizer]->GetPhysicsBody()->axis.x = static_cast<float>(nReds);
}

// The two enable ratings are flags; every other rating is added and clamped to nought and ten.

// FUNCTION: HYPERX 0x00463900
void PlanAction::ModifyRating(Interactor* pContext, PlanNode* pArgs, short*, int nArg)
{
    const auto nSlot = static_cast<short>(nArg);

    PlayerObject* pObject;
    int           nIndex;

    const auto nStat = pArgs->m_pArgs[ARG_STAT].n;

    if (pContext->GetNamedSlot() == -1)
    {
        if (pArgs->m_nArgCount == kArgCountWithoutSlot)
        {
            auto* const pColObj = g_pGameEngine->GetColObjPtr(pContext->GetModelSlot());

            pObject = (PlayerObject*)pColObj->m_pOwner;
        }
        else
        {
            nIndex = static_cast<short>(pArgs->m_pArgs[ARG_SLOT].n);

            pObject = g_pGameEngine->GetPlayer(nIndex);
        }
    }
    else
    {
        nIndex = nSlot;

        if (pArgs->m_pArgs[ARG_SLOT].n != PLAN_TOKEN_MESSAGE_DATA)
        {
            nIndex = static_cast<short>(pArgs->m_pArgs[ARG_SLOT].n);
        }

        pObject = g_pGameEngine->GetPlayer(nIndex);
    }

    const auto nAmount = static_cast<short>(pArgs->m_pArgs[kArgAmount].n);

    const auto flScalar = pObject->m_stats.GetScalar(nStat);

    switch (nStat)
    {
        case kRatingLongShotEnable:
        case kRatingLongPassEnable:
            if (nAmount <= 0)
            {
                pObject->m_stats.m_aStats[RATING_SLOT(nStat)] = 0.0f;

                return;
            }

            pObject->m_stats.m_aStats[RATING_SLOT(nStat)] = 1.0f;

            return;
    }

    auto flValue = static_cast<float>(nAmount) + flScalar;

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
void PlanAction::SwapRandomizers(Interactor* pContext, PlanNode* pNode, short*, int nPassed)
{
    const auto nDefaultSlot = static_cast<short>(nPassed);

    EngineObject* pActor;
    EngineObject* pNamed;
    short         nSlot;

    const auto vRandomizerAt = *g_pGameEngine->m_ppSlots[kSlotRedRandomizer]->GetPositionPtr();

    auto* const pRandomizer = g_pGameEngine->m_ppSlots[kSlotRedRandomizer];

    if (pContext->GetNamedSlot() == -1)
    {
        nSlot = pContext->GetOtherSlot();

        auto* const pCollision = g_pGameEngine->GetColObjPtr(nSlot);

        pActor = pCollision->GetOwner();

        auto* const pBlock = pNode->m_pArgs;

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
                nSlot = static_cast<short>(pBlock[1].n);
            }

            pNamed = g_pGameEngine->m_ppSlots[nSlot];
        }
    }
    else
    {
        pActor = pContext->GetActor();

        auto* const pBlock = pNode->m_pArgs;

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
                nSlot = static_cast<short>(pBlock[1].n);
            }

            pNamed = g_pGameEngine->m_ppSlots[nSlot];
        }
    }

    if (pRandomizer == nullptr || pNamed == nullptr)
    {
        return;
    }

    if (pRandomizer != pActor && pNamed != pActor)
    {
        return;
    }

    const Vec3* pAt = pActor->GetPositionPtr();

    pRandomizer->m_pBody->pos = *pAt;

    auto* pFrom = pActor->GetPositionPtr();

    pRandomizer->GetPhysicsBody()->SetFlatOnTerrain(pFrom);

    pRandomizer->m_nFlags &= ~kEngineObjectHidden;

    pAt = pNamed->GetPositionPtr();

    pActor->m_pBody->pos = *pAt;

    pFrom = pNamed->GetPositionPtr();

    pActor->GetPhysicsBody()->SetFlatOnTerrain(pFrom);

    pActor->m_nFlags &= ~kEngineObjectHidden;

    pNamed->m_pBody->pos = vRandomizerAt;

    pNamed->m_nFlags |= kEngineObjectHidden;
}

constexpr auto kArgTracked = 2;

constexpr auto kArgCountWithTracked = 3;

// A positive damage rating clears the killer slot to minus one.
// FUNCTION: HYPERX 0x00463c20
void PlanAction::SetKilledDromer(Interactor* pContext, PlanNode* pArgs, short*, int nMessageData)
{
    int nObject;
    int nTracked;

    auto* pBlock = pArgs->m_pArgs;

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

    if (pArgs->m_nArgCount == kArgCountWithTracked)
    {
        pBlock = pArgs->m_pArgs;

        if (pBlock[kArgTracked].IsPronoun())
        {
            nTracked = g_apPlanStatFns[pBlock[kArgTracked].wPronoun](
                pContext->GetActor(), pArgs);
        }
        else
        {
            nTracked = pBlock[kArgTracked].n;
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

    auto* const pObject = g_pGameEngine->GetPlayer(static_cast<short>(nObject));

    if (pObject->m_stats.GetScalar(kRatingDamage) <= 0.0f)
    {
        pObject->m_wKilledBySlot = static_cast<short>(nTracked);

        return;
    }

    pObject->m_wKilledBySlot = -1;
}

constexpr auto kArgCountWrite = 2;

// What a read answers when m_wDesiredSubstitute is minus one, no substitute: one past the skaters.
constexpr auto kSubstituteNoneAnswer = 4;

// FUNCTION: HYPERX 0x00463d20
void PlanAction::ResetCameraTrack(Interactor* pContext, PlanNode*, short*, int)
{
    auto* const pCamera = pContext->GetCameraActor();

    pCamera->SetLastTrackedObject(nullptr);
}

// FUNCTION: HYPERX 0x00463d40
void PlanAction::ClearBehaviorAnimation(Interactor* pContext, PlanNode*, short*, int)
{
    auto* pActor = pContext->GetActor();

    pActor->GetPlanState()->m_pCurrent->m_pAnimation = nullptr;

    pActor = pContext->GetActor();

    pActor->GetPlanState()->m_pCurrent->m_pPersistentAnimation = nullptr;
}

// FUNCTION: HYPERX 0x00463d80
void PlanAction::FlyingSemaphore(Interactor* pContext, PlanNode* pArgs, short* pResult, int)
{
    auto* const pActor = pContext->GetPlayerActor();

    if (pArgs->m_nArgCount == 2)
    {
        pActor->m_wFlyingSemaphore = static_cast<short>(pArgs->m_pArgs[1].n);
    }
    else
    {
        *pResult = pActor->m_wFlyingSemaphore != 0;
    }
}

// FUNCTION: HYPERX 0x00463dc0
void PlanAction::AnimationSemaphore(Interactor* pContext, PlanNode* pArgs, short* pResult, int)
{
    auto* const pActor = pContext->GetPlayerActor();

    if (pArgs->m_nArgCount == 2)
    {
        pActor->m_wAnimationSemaphore = static_cast<short>(pArgs->m_pArgs[1].n);
    }
    else
    {
        *pResult = pActor->m_wAnimationSemaphore != 0;
    }
}

// FUNCTION: HYPERX 0x00463e00
void PlanAction::ClockStoppedSemaphore(Interactor* pContext, PlanNode* pArgs, short* pResult, int)
{
    auto* const pActor = pContext->GetPlayerActor();

    if (pArgs->m_nArgCount == 2)
    {
        pActor->m_wClockStoppedSemaphore = static_cast<short>(pArgs->m_pArgs[1].n);
    }
    else
    {
        *pResult = pActor->m_wClockStoppedSemaphore != 0;
    }
}

// A count of two writes and any other reads.

// FUNCTION: HYPERX 0x00463e40
void PlanAction::DesiredSubstitute(Interactor* pContext, PlanNode* pArgs, short* pResult, int nPassed)
{
    const auto nSlot = static_cast<unsigned int>(nPassed);

    unsigned int nArg;

    auto* const pActor = pContext->GetPlayerActor();

    const PlanWord* const pBlock = pArgs->m_pArgs;

    if (pBlock[1].IsPronoun())
    {
        nArg = g_apPlanStatFns[pBlock[1].wPronoun](pContext->GetActor(), pArgs);
    }
    else
    {
        nArg = pBlock[1].n;
    }

    if (static_cast<int>(nArg) == PLAN_TOKEN_MESSAGE_DATA)
    {
        nArg = nSlot;
    }

    if (pArgs->m_nArgCount == kArgCountWrite)
    {
        pActor->m_wDesiredSubstitute = static_cast<short>(nArg);

        return;
    }

    const auto nValue = pActor->m_wDesiredSubstitute;

    *pResult = nValue;

    if (nValue == -1)
    {
        *pResult = kSubstituteNoneAnswer;
    }
}

// FUNCTION: HYPERX 0x00463ed0
void PlanCond::SubstitutionSemaphore(Interactor* pContext, PlanNode*, short* pResult, int)
{
    auto* const pActor = pContext->GetPlayerActor();

    *pResult = pActor->m_wDesiredSubstitute != -1;
}

// Of the first and last fit substitutes the healthier wins, with the near half favoured.

// Answered when no substitute is fit.
constexpr auto kSlotAnswerNone = 4;

// FUNCTION: HYPERX 0x00463ef0
void PlanCond::PickValidSubstitute(Interactor* pContext, PlanNode*, short* pResult, int)
{
    short            sIndex;
    int              nWhich;
    const TeamBlock* pTeam;
    float            flBoost;

    pContext->GetActor();

    const int nSlot = pContext->GetActor()->GetSlot();

    short sCandidate  = kSlotNone;
    short sChallenger = kSlotNone;

    for (sIndex = 0, nWhich = 0; sIndex < kPlayerSlotCount; sIndex++, nWhich++)
    {
        if (g_pGameEngine->IsFitSubstitute(nSlot, nWhich) != 0)
        {
            if (sCandidate == kSlotNone)
            {
                sCandidate = sIndex;
            }
            else
            {
                sChallenger = sIndex;
            }
        }
    }

    if (sCandidate == kSlotNone)
    {
        *pResult = kSlotAnswerNone;
        return;
    }

    if (sChallenger == kSlotNone)
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

    auto flFirst  = pTeam->aRows[nFirst].aStat[kRatingDamage];
    auto flSecond = pTeam->aRows[nSecond].aStat[kRatingDamage];

    if (pTeam->aReserveSince[nFirst] != 0)
    {
        const auto flGate = DamageGate();

        flFirst = static_cast<float>(g_pGameEngine->m_clock.nTickMs - pTeam->aReserveSince[nFirst]) / (13500.0f / flGate) + flFirst;
    }

    if (pTeam->aReserveSince[nSecond] != 0)
    {
        flSecond = static_cast<float>(g_pGameEngine->m_clock.nTickMs - pTeam->aReserveSince[nSecond]) / (13500.0f / DamageGate()) + flSecond;
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

#define CREDIT_OTHER()                                                                                \
    if (nTeam % 2 == nOther % 2)                                                                      \
    {                                                                                                 \
        return;                                                                                       \
    }                                                                                                 \
                                                                                                      \
    g_pGameEngine->GetPlayer((short)nOther)->m_matchStats.AccumulateStat(kMatchStatInjuries, nOther); \
    return;

#define ACCUMULATE_HIT(nLowStat)                                                       \
    if (nSlot == kSlotMine)                                                            \
    {                                                                                  \
        nOther = g_pGameEngine->m_ppSlots[kSlotMine]->m_nEntitySlot;                   \
        CREDIT_OTHER()                                                                 \
    }                                                                                  \
    else if (nSlot == kSlotRok)                                                        \
    {                                                                                  \
        nOther = g_pGameEngine->m_ppSlots[kSlotRok]->m_nEntitySlot;                    \
        CREDIT_OTHER()                                                                 \
    }                                                                                  \
    else if (nSlot >= kSlotTurnstile && nSlot <= kSlotTurnstileLast)                   \
    {                                                                                  \
        nOther = g_pGameEngine->m_ppSlots[nSlot]->m_nEntitySlot;                       \
        CREDIT_OTHER()                                                                 \
    }                                                                                  \
    else                                                                               \
    {                                                                                  \
        if (nSlot >= kSlotLaserHurdle && nSlot <= kSlotLaserHurdleLast)                \
        {                                                                              \
            return;                                                                    \
        }                                                                              \
                                                                                       \
        if (nSlot < 0)                                                                 \
        {                                                                              \
            return;                                                                    \
        }                                                                              \
                                                                                       \
        if (nSlot > kSlotDromer3)                                                      \
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
    auto* const pPlayer = pContext->GetPlayerActor();
    auto* const pActor  = pContext->GetActor();

    const int nSlot = pActor->GetSlot();

    if (pArgs->m_nArgCount == 2)
    {
        const auto nSub = pPlayer->m_wDesiredSubstitute;

        if (nSub == -1)
        {
            return;
        }

        g_pGameEngine->SubstitutePlayer(nSlot, nSub);

        return;
    }

    *pResult = static_cast<short>(g_pGameEngine->IsFitSubstitute(nSlot, nParam) != 0);
}

// Whether the team mate may substitute in; the byte answer is normalised to one or zero.

// FUNCTION: HYPERX 0x004641b0
void PlanCond::OkToSubstituteTeammate(Interactor* pContext, PlanNode*, short* pResult, int nParam)
{
    pContext->GetActor();

    auto* const pActor = pContext->GetActor();

    const auto nSlot = static_cast<unsigned int>(pActor->GetSlot());

    *pResult = static_cast<unsigned short>(g_pGameEngine->IsFitSubstitute(TeammateSlot(static_cast<short>(nSlot)), nParam) != 0);
}

// FUNCTION: HYPERX 0x00464200
void PlanAction::TrackTurnstile(Interactor* pContext, PlanNode* pArgs, short*, int nParam)
{
    auto nValue = pArgs->m_pArgs[1].n;

    if (nValue == PLAN_TOKEN_MESSAGE_DATA)
    {
        nValue = nParam;
    }

    auto* const pActor = pContext->GetActor();

    pActor->SetEntitySlot(nValue);
}

// Nothing is credited when hitter and target are on the same side, the two parities.

// FUNCTION: HYPERX 0x00464230
void PlanAction::CreditDamage(Interactor* pContext, PlanNode* pArgs, short*, int)
{
    int nOther;

    auto* const pObject = g_pGameEngine->GetPlayer(static_cast<short>(pArgs->m_pArgs[1].n));

    const auto nTeam = pObject->GetSlot();

    const auto flDamage = pObject->m_stats.GetScalar(kRatingDamage);

    const auto nSlot = pContext->GetTargetSlot();

    if (flDamage <= DamageGate())
    {
        if (flDamage > 0.0f)
        {
            ACCUMULATE_HIT(kMatchStatInjuries)
        }
    }
    else if (flDamage > 0.0f)
    {
        return;
    }

    ACCUMULATE_HIT(kMatchStatFatalities)
}

constexpr auto kArgCountColSlot = 2;

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

    const auto flSpeed = pArgs->m_pArgs[1].fl;

    g_flMaxSpeed = flSpeed;
}

// Nought or less is scaled by difficulty; the damage is added and clamped to nought to a hundred.

// FUNCTION: HYPERX 0x004644c0
void PlanAction::ModifyDamage(Interactor* pContext, PlanNode* pArgs, short*, int nParam)
{
    PlayerObject*   pObject;
    const PlanWord* pRecord;
    BoundBubble*    pCollision;
    int             nWhich;
    int             nValue;
    int             nKillerSlot;
    float           flScale;
    float           flClamped;

    if (pContext->GetNamedSlot() == kSlotNone)
    {
        if (pArgs->m_nArgCount == kArgCountColSlot)
        {
            pCollision = g_pGameEngine->GetColObjPtr(pContext->GetModelSlot());

            pObject = (PlayerObject*)pCollision->GetOwner();
            nValue  = pArgs->m_pArgs[1].n;
        }
        else
        {
            pRecord = pArgs->m_pArgs;
            pObject = g_pGameEngine->GetPlayer(static_cast<short>(pRecord[1].n));
            nValue  = pRecord[2].n;
        }
    }
    else
    {
        pRecord = pArgs->m_pArgs;

        if (pRecord[1].n == PLAN_TOKEN_MESSAGE_DATA)
        {
            nWhich = static_cast<short>(nParam);
        }
        else
        {
            nWhich = static_cast<short>(pRecord[1].n);
        }

        nValue = pRecord[2].n;

        pObject = g_pGameEngine->GetPlayer(nWhich);
    }

    pObject->GetSlot();

    auto* const pStats = &pObject->m_stats;

    const auto flDamage = pStats->GetScalar(kRatingDamage);

    if (nValue == PLAN_TOKEN_MESSAGE_DATA)
    {
        nKillerSlot = pContext->GetTargetSlot();
        nValue      = nParam;
    }
    else
    {
        nKillerSlot = pContext->GetActor()->GetSlot();
    }

    if ((flDamage > DamageGate() || static_cast<float>(nValue) > g_flZeroPlan) && pObject->m_wClockStoppedSemaphore == 0)
    {
        if (g_pGameEngine->GetDifficultyLevel() == DifficultyLevel::kEasy)
        {
            flScale = SCALE_EASY;
        }
        else if (g_pGameEngine->GetDifficultyLevel() == DifficultyLevel::kMedium)
        {
            flScale = SCALE_MEDIUM;
        }
        else if (g_pGameEngine->GetDifficultyLevel() == DifficultyLevel::kHard)
        {
            flScale = SCALE_HARD;
        }
        else
        {
            flScale = SCALE_HARD;
        }

        if (g_bSessionActive != 0)
        {
            auto* const pszName = pObject->GetPlanState()->m_pszName;

            // STRING: HYPERX 0x004bb340
            if (strcmp("Human", pszName) != 0)
            {
                flScale = SCALE_NETWORK_BOT;
            }
        }

        if (nValue <= 0)
        {
            nValue = static_cast<int>(static_cast<float>(nValue) * flScale);
        }

        const auto flTotal = static_cast<float>(nValue) + flDamage;

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

        pStats->m_aStats[RATING_SLOT(kRatingDamage)] = flClamped;

        if (flTotal <= 0.0f)
        {
            pObject->m_wKilledBySlot = static_cast<short>(nKillerSlot);
        }
        else
        {
            pObject->m_wKilledBySlot = kSlotNone;
        }
    }
}

constexpr auto kArgGate       = 1;
constexpr auto kArgHundredths = 2;

constexpr auto kSpeedCacheCount = 4;

// The clamp, as two doubles.
#define SPEED_RATING_MIN 0.99
#define SPEED_RATING_MAX 10.99

// A hundredth, the scale the raw word is in, and the offset added to the scaled reading.
#define HUNDREDTHS 0.01f
#define SPEED_BIAS 0.00999f

// Minus one as its bits; the cache entry is tested as a dword.
constexpr auto kSpeedUnsetBits = 0xbf800000;

// The cache of the four skaters' speeds and its dirty word, which ships set.
// GLOBAL: HYPERX 0x004c4540
extern float g_aOpposSpeedCache[kSpeedCacheCount];
// GLOBAL: HYPERX 0x004c4550
extern short g_nOpposSpeedCacheStale;

// FUNCTION: HYPERX 0x00464720
void PlanAction::IncrementScore(Interactor*, PlanNode* pArgs, short*, int)
{
    if (pArgs->m_pArgs[1].n != 0)
    {
        InfoBar::IncrementScore(kScoreLineTeam1);
    }
    else
    {
        InfoBar::IncrementScore(kScoreLineTeam2);
    }
}

// The twin of ModifySpeedRating, writing a pair: the complement's low bit and that slot plus two.

// FUNCTION: HYPERX 0x00464750
void PlanAction::ModifyOpposTeamFriction(Interactor* pContext, PlanNode* pNode, short*, int nArg)
{
    const auto nSlot = static_cast<short>(nArg);

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
        nGate       = static_cast<short>(pArgs[kArgGate].n);
        nHundredths = static_cast<short>(pArgs[kArgHundredths].n);

        flSpeed = static_cast<float>(nHundredths) * HUNDREDTHS + SPEED_BIAS;

        if (pNode->m_nArgCount == kArgCountWithoutSlot)
        {
            auto* const pColObj = g_pGameEngine->GetColObjPtr(pContext->GetModelSlot());

            nTarget = pColObj->GetOwner()->GetSlot();
        }
        else
        {
            nTarget = static_cast<short>(pArgs[ARG_SLOT].n);
        }
    }
    else
    {
        pArgs       = pNode->m_pArgs;
        nGate       = static_cast<short>(pArgs[kArgGate].n);
        nHundredths = static_cast<short>(pArgs[kArgHundredths].n);

        flSpeed = static_cast<float>(nHundredths) * HUNDREDTHS + SPEED_BIAS;

        if (pArgs[ARG_SLOT].n == PLAN_TOKEN_MESSAGE_DATA)
        {
            nTarget = nSlot;
        }
        else
        {
            nTarget = static_cast<short>(pArgs[ARG_SLOT].n);
        }
    }

    if (g_nOpposSpeedCacheStale != 0)
    {
        for (int i = 0; i < kSpeedCacheCount; i++)
        {
            pObject = g_pGameEngine->GetPlayer(static_cast<short>(i));

            g_aOpposSpeedCache[i] = pObject->m_stats.GetScalar(kRatingSpeed);
        }

        g_nOpposSpeedCacheStale = 0;
    }

    const auto flBoosted = flSpeed + 1.0f;

    dReading = static_cast<float>(static_cast<short>(static_cast<long>(flBoosted * g_aOpposSpeedCache[nTarget] + 0.5f))) + 0.99f;

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

    auto nFlip = (static_cast<unsigned char>(~nTarget)) & 1;

    if (nGate != 0)
    {
        if (FloatBits(g_aOpposSpeedCache[static_cast<short>(nFlip)]) != kSpeedUnsetBits)
        {
            pObject = g_pGameEngine->GetPlayer(static_cast<short>(nFlip));

            pObject->m_stats.SetStat(RATING_SLOT(kRatingSpeed), g_aOpposSpeedCache[nTarget]);

            nFlip += 2;

            pObject = g_pGameEngine->GetPlayer(static_cast<short>(nFlip));

            pObject->m_stats.SetStat(RATING_SLOT(kRatingSpeed), g_aOpposSpeedCache[nTarget]);
        }

        g_nOpposSpeedCacheStale = 1;
    }
    else
    {
        pObject = g_pGameEngine->GetPlayer(static_cast<short>(nFlip));

        pObject->m_stats.m_aStats[RATING_SLOT(kRatingSpeed)] = static_cast<float>(dReading);

        nFlip += 2;

        pObject = g_pGameEngine->GetPlayer(static_cast<short>(nFlip));

        pObject->m_stats.m_aStats[RATING_SLOT(kRatingSpeed)] = static_cast<float>(dReading);
    }
}

// The cache of the four skaters' speeds and its dirty word, which ships set.
// GLOBAL: HYPERX 0x004c4554
extern float g_aSpeedCache[kSpeedCacheCount];
// GLOBAL: HYPERX 0x004c4564
extern short g_nSpeedCacheStale;

// The reading is (speed + 1) times the cached speed, rounded to a short, plus 0.99.
// FUNCTION: HYPERX 0x00464940
void PlanAction::ModifySpeedRating(Interactor* pContext, PlanNode* pNode, short*, int nArg)
{
    const auto nSlot = static_cast<short>(nArg);

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
        nGate       = static_cast<short>(pArgs[kArgGate].n);
        nHundredths = static_cast<short>(pArgs[kArgHundredths].n);

        flSpeed = static_cast<float>(nHundredths) * HUNDREDTHS;

        if (pNode->m_nArgCount == kArgCountWithoutSlot)
        {
            auto* const pColObj = g_pGameEngine->GetColObjPtr(pContext->GetModelSlot());

            nTarget = pColObj->GetOwner()->GetSlot();
        }
        else
        {
            nTarget = static_cast<short>(pArgs[ARG_SLOT].n);
        }
    }
    else
    {
        pArgs       = pNode->m_pArgs;
        nGate       = static_cast<short>(pArgs[kArgGate].n);
        nHundredths = static_cast<short>(pArgs[kArgHundredths].n);

        flSpeed = static_cast<float>(nHundredths) * HUNDREDTHS;

        if (pArgs[ARG_SLOT].n == PLAN_TOKEN_MESSAGE_DATA)
        {
            nTarget = nSlot;
        }
        else
        {
            nTarget = static_cast<short>(pArgs[ARG_SLOT].n);
        }
    }

    if (g_nSpeedCacheStale != 0)
    {
        for (int i = 0; i < kSpeedCacheCount; i++)
        {
            pObject = g_pGameEngine->GetPlayer(static_cast<short>(i));

            g_aSpeedCache[i] = pObject->m_stats.GetScalar(kRatingSpeed);
        }

        g_nSpeedCacheStale = 0;
    }

    const auto flBoosted = flSpeed + 1.0f;

    dReading = static_cast<float>(static_cast<short>(static_cast<long>(flBoosted * g_aSpeedCache[nTarget] + 0.5f))) + 0.99f;

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

    const auto flReading = static_cast<float>(dReading);

    if (nGate != 0)
    {
        if (g_aSpeedCache[0] != -1.0f)
        {
            pObject = g_pGameEngine->GetPlayer(nTarget);

            if (pObject->m_stats.GetScalar(kRatingSpeed) == flReading)
            {
                pObject = g_pGameEngine->GetPlayer(nTarget);

                pObject->m_stats.SetStat(RATING_SLOT(kRatingSpeed), g_aSpeedCache[nTarget]);
            }

            g_nSpeedCacheStale = 1;
        }
    }
    else
    {
        pObject = g_pGameEngine->GetPlayer(nTarget);

        pObject->m_stats.SetStat(RATING_SLOT(kRatingSpeed), flReading);
    }
}

constexpr auto kArgRange      = 1;
constexpr auto kArgFacingGate = 2;
constexpr auto kArgDotGate    = 3;
#undef ARG_SLOT
#define ARG_SLOT 4
#undef ARG_DURATION
#define ARG_DURATION 5
constexpr auto kArgLead = 6;

#define DEFAULT_RANGE       100.0f
#define DEFAULT_FACING_GATE 0.1f
#define DEFAULT_DOT_GATE    0.2f
constexpr auto kDefaultDuration = 20;

// A fiftieth, as a double.
#define GATE_SCALE 0.02

constexpr auto kLaunchMessageArg = 2;

// FUNCTION: HYPERX 0x00464b10
void PlanAction::SetImmunity(Interactor* pContext, PlanNode* pArgs, short*, int nArg)
{
    const auto nSlot = static_cast<short>(nArg);

    PlanWord* pBlock;
    int       nIndex;
    float     flValue;

    if (pContext->GetNamedSlot() == -1)
    {
        if (pArgs->m_nArgCount == 2)
        {
            auto* const pCollision = g_pGameEngine->GetColObjPtr(pContext->GetModelSlot());

            pBlock = pArgs->m_pArgs;

            ((PlayerObject*)pCollision->GetOwner())->m_stats.m_aStats[RATING_SLOT(kRatingImmunity)] =
                static_cast<float>(static_cast<unsigned int>(pBlock[1].n));

            return;
        }

        pBlock  = pArgs->m_pArgs;
        nIndex  = static_cast<short>(pBlock[1].n);
        flValue = static_cast<float>(static_cast<unsigned int>(pBlock[2].n));

        g_pGameEngine->GetPlayer(nIndex)->m_stats.m_aStats[RATING_SLOT(kRatingImmunity)] = flValue;

        return;
    }

    pBlock  = pArgs->m_pArgs;
    flValue = static_cast<float>(static_cast<unsigned int>(pBlock[2].n));

    if (pBlock[1].n == PLAN_TOKEN_MESSAGE_DATA)
    {
        nIndex = nSlot;
    }
    else
    {
        nIndex = static_cast<short>(pBlock[1].n);
    }

    g_pGameEngine->GetPlayer(nIndex)->m_stats.m_aStats[RATING_SLOT(kRatingImmunity)] = flValue;
}

// FUNCTION: HYPERX 0x00464c00
void PlanCond::GetImmunity(Interactor* pContext, PlanNode*, short* pResult, int)
{
    auto* const pActor = pContext->GetPlayerActor();

    if (pActor->m_stats.GetScalar(kRatingImmunity) == g_flZeroPlan)
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
void PlanAction::ThrowBungee(Interactor* pContext, PlanNode* pNode, short*, int)
{
    EngineObject* pWinner;

    const auto nFirst = static_cast<short>((pContext->GetActor()->GetSlot() + 1) % kPlayerSlotCount);

    auto* const pFirst = g_pGameEngine->m_ppSlots[nFirst];

    const auto nSecond = static_cast<short>((pContext->GetActor()->GetSlot() + 3) % kPlayerSlotCount);

    auto flRange = DEFAULT_RANGE;

    auto* const pSecond = g_pGameEngine->m_ppSlots[nSecond];

    auto  flLead       = g_flZeroPlan;
    auto  flFacingGate = DEFAULT_FACING_GATE;
    auto  flDotGate    = DEFAULT_DOT_GATE;
    short nDuration    = kDefaultDuration;

    const auto nArgCount = pNode->m_nArgCount;

    if (nArgCount > 3)
    {
        flRange = static_cast<float>(static_cast<unsigned int>(pNode->m_pArgs[kArgRange].n));

        flFacingGate = static_cast<float>(static_cast<unsigned int>(pNode->m_pArgs[kArgFacingGate].n) * GATE_SCALE - g_flOnePlan);

        flDotGate = static_cast<float>(static_cast<unsigned int>(pNode->m_pArgs[kArgDotGate].n) * GATE_SCALE - g_flOnePlan);
    }

    if (nArgCount >= 5)
    {
        auto* const pArgs = pNode->m_pArgs;

        pWinner = g_pGameEngine->m_ppSlots[static_cast<short>(pArgs[ARG_SLOT].n)];

        if (nArgCount >= 6)
        {
            nDuration = static_cast<short>(pArgs[ARG_DURATION].n);

            if (nArgCount >= 7)
            {
                flLead = static_cast<float>(static_cast<unsigned int>(pArgs[kArgLead].n));
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

    if (pWinner == nullptr)
    {
        return;
    }

    auto* const pBungee = (CBungeeToward*)AllocAgendaItem(Agenda_CBungeeToward);

    auto* const pBindNode = pContext->GetActor()->GetPlanState()->m_pCurrent;

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

    pWinner->PostEvent(kMsgYerAttacked, kLaunchMessageArg, pContext->GetActor()->GetSlot());
}

// The original compares the holder slot even on the path that never wrote it.

// FUNCTION: HYPERX 0x00464f40
void PlanCond::AttachedToSender(Interactor* pContext, PlanNode*, short* pResult, int nArg)
{
    short nHolderSlot = -1;

    auto* pState = pContext->GetActor()->GetPhysicsBody()->pAttach;

    if (pState != nullptr && pState->nSlot < 6)
    {
        nHolderSlot = pState->nSlot;
    }

    *pResult = static_cast<short>(nArg) == nHolderSlot;
}

#define OWNER_SCALE 0.125f
#define JITTER_RATE 15.0f

// Four; its marker is in src/globals.cpp.
extern const float g_flFourPlan;

// Without a carrier the original compares nCarrier unset.
// FUNCTION: HYPERX 0x00464f80
void PlanCond::AmIKnockedLoose(Interactor* pContext, PlanNode*, short* pResult, int nArg)
{
    const auto nOwnerSlot = static_cast<short>(nArg);

    short nCarrier = -1;
    Vec3  vKick;
    Vec3  vVelocity;
    Vec3  vJitter;

    auto* pCarrier = pContext->GetActor()->GetPhysicsBody()->pAttach;

    if (pCarrier != nullptr && pCarrier->nSlot < kPlayerObjectCount)
    {
        nCarrier = pCarrier->nSlot;
    }

    if (nOwnerSlot == nCarrier)
    {
        auto* const pCarrierBlock = g_pGameEngine->m_ppSlots[nCarrier]->GetPhysicsBody();

        Vec3::ScaleTo(&pCarrierBlock->vel, g_flFourPlan, &vKick);

        auto* const pBody  = pContext->GetActor()->m_pBody;
        const auto  vOwner = pBody->vel;

        Vec3::ScaleTo(&vOwner, OWNER_SCALE, &vVelocity);

        vJitter.Set(0.0f, 0.0f, 0.0f);
        vVelocity.Add(vKick);

        vJitter.x += (static_cast<float>(rand()) * (1.0f / static_cast<float>(RAND_MAX))) * g_flTwoPlan - g_flOnePlan;
        vJitter.y += (static_cast<float>(rand()) * (1.0f / static_cast<float>(RAND_MAX))) * g_flTwoPlan - g_flOnePlan;
        vJitter.z += static_cast<float>(rand()) * (1.0f / static_cast<float>(RAND_MAX)) * g_flOnePlan + g_flOnePlan;

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

    auto nRokValue = g_pGameEngine->GetRok()->m_wRokValue;

    const auto nSlot = pContext->GetActor()->GetSlot();

    if (nSlot == kSlotSgoal)
    {
        if (nRokValue < 1)
        {
            nRokValue = 1;
        }
    }
    else if (nSlot == kSlotNgoal)
    {
        nClamped = std::min<int>(-1, nRokValue);

        nRokValue = static_cast<short>(-nClamped);
    }
    else
    {
        nRokValue = 0;
    }

    if (g_pGameEngine->m_ppSlots[kSlotRok]->GetPhysicsBody()->pAttach != nullptr)
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
void PlanCond::Jumping(Interactor* pContext, PlanNode*, short* pResult, int)
{
    auto* const pCollision = g_pGameEngine->GetColObjPtr(pContext->GetModelSlot());

    auto* const pActor = pCollision->GetOwner();

    if (pActor->GetPlanState()->m_pCurrent->m_wKind != kBehFly)
    {
        const auto flInvDelta = g_pGameEngine->m_clock.flInvDelta;

        if (flInvDelta * pActor->GetPhysicsBody()->flSpeed >= SPEED_THRESHOLD)
        {
            *pResult = 0;

            return;
        }
    }

    *pResult = 1;
}

// Within ten of an object without the 0x10 bit answers two; both or neither answers three or a coin.

constexpr auto kStateAvailable = 0x10;

// Inside this the actor counts as standing at the object.
#define AT_OBJECT_RANGE 10.0f

// The four answers: the mine, the swarf, no, and neither.
enum NearestAnswer : short
{
    kAnswerMine    = 0,
    kAnswerSwarf   = 1,
    kAnswerNo      = 2,
    kAnswerNeither = 3
};

// FUNCTION: HYPERX 0x00465310
void PlanCond::RandomInactiveWeapon(Interactor* pContext, PlanNode*, short* pResult, int)
{
    short nAtFirst  = 0;
    short nAtSecond = 0;

    auto* const pActor = pContext->GetActor();

    EngineObject** const apSlots = g_pGameEngine->m_ppSlots;
    auto* const          pFirst  = apSlots[kSlotMine];
    auto* const          pSecond = apSlots[kSlotSwarf];

    const auto vFirst  = *pFirst->GetPositionPtr();
    auto       vSecond = *pSecond->GetPositionPtr();

    auto vSelf = *pActor->GetPositionPtr();

    auto flDx = vSelf.x - vFirst.x;
    auto flDy = vSelf.y - vFirst.y;
    auto flDz = vSelf.z - vFirst.z;

    if (static_cast<float>(sqrt(static_cast<double>(flDx * flDx + flDy * flDy + flDz * flDz))) < AT_OBJECT_RANGE)
    {
        if ((static_cast<unsigned char>(pFirst->m_nFlags) & kStateAvailable) != 0)
        {
            nAtFirst = 1;
        }
        else
        {
            *pResult = kAnswerNo;
            return;
        }
    }

    flDx = vSelf.x - vSecond.x;
    flDy = vSelf.y - vSecond.y;
    flDz = vSelf.z - vSecond.z;

    if (static_cast<float>(sqrt(static_cast<double>(flDx * flDx + flDy * flDy + flDz * flDz))) < AT_OBJECT_RANGE)
    {
        if ((static_cast<unsigned char>(pSecond->m_nFlags) & kStateAvailable) != 0)
        {
            nAtSecond = 1;
        }
        else
        {
            *pResult = kAnswerNo;
            return;
        }
    }

    if (nAtFirst > nAtSecond)
    {
        *pResult = kAnswerMine;
        return;
    }

    if (nAtSecond > nAtFirst)
    {
        *pResult = kAnswerSwarf;
        return;
    }

    if (nAtSecond == nAtFirst)
    {
        if (nAtSecond == 0)
        {
            *pResult = kAnswerNeither;
            return;
        }

        *pResult = static_cast<short>(rand() % 2);
    }
}

// The answer is one-based, so zero means no armory holds fewer than every weapon.
// FUNCTION: HYPERX 0x004654b0
void PlanCond::RandomEmptyArmory(Interactor*, PlanNode*, short* pResult, int)
{
    const auto nSlots = g_pGameEngine->GetArmoryCount();

    auto nFound = -1;

    auto nBest = g_pGameEngine->GetMineCount() + g_pGameEngine->GetSwarfCount();

    for (int iSlot = 0; iSlot < nSlots; iSlot++)
    {
        const auto nArmory = iSlot + 1;
        const auto nCount  = GameEngine::CountEntitiesInRange(static_cast<short>(nArmory + kSlotArmory - 1));

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

    *pResult = static_cast<short>(nFound);
}

// Two arms pick a different killed answer when GameEngine::m_nBrutality is off.

// FUNCTION: HYPERX 0x00465650
void PlanCond::WhoKilledMe(Interactor* pContext, PlanNode*, short* pResult, int)
{
    char bBrutalityOff = 0;

    auto* const pObject = pContext->GetPlayerActor();

    const auto flDamage = pObject->m_stats.GetScalar(kRatingDamage);

    if (g_pGameEngine->m_nBrutality == 0)
    {
        bBrutalityOff = 1;
    }

    switch (pObject->m_wKilledBySlot)
    {
        case kSlotTurnstile:
        case kSlotTurnstile + 1:
        case kSlotTurnstile + 2:
        case kSlotTurnstile + 3:
        case kSlotTurnstile + 4:
        case kSlotTurnstile + 5:
        case kSlotTurnstileLast:
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

        case kSlotLaserHurdle:
        case kSlotLaserHurdle + 1:
        case kSlotLaserHurdle + 2:
        case kSlotLaserHurdle + 3:
        case kSlotLaserHurdle + 4:
        case kSlotLaserHurdleLast:
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

        case kSlotMine:
            if (flDamage <= g_flZeroPlan)
            {
                *pResult = 2;
                return;
            }

            *pResult = 7;
            return;

        case kSlotRok:
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
constexpr auto kHitDamageCeiling = 0x14;

// An object that hits carries its own slot as the attack; the engine's own hit is the far goalie's.
// FUNCTION: HYPERX 0x004657b0
void PlanCond::TakeDamage(Interactor* pContext, PlanNode* pArgs, short* pResult, int nAttack)
{
    float flOtherDamage;
    float flOtherWeight;
    float flOtherChecking;
    float flRange;
    float flDamage;
    float flStrength;
    Vec3  vOffset;

    auto* const pActor = pContext->GetPlayerActor();

    auto nAttacker = pContext->GetTargetSlot();

    if (nAttacker == kSlotGod)
    {
        if (pActor->GetSlot() % 2 != 0)
        {
            nAttacker = kGoalieSlotA;
        }
        else
        {
            nAttacker = kGoalieSlotB;
        }
    }

    pActor->m_wAttackeeSlot = nAttacker;

    *pResult = 0;

    if (nAttacker == kSlotRok)
    {
        if (nAttack == kSlotRok)
        {
            *pResult = pArgs->m_nArgCount - 3;

            return;
        }
    }
    else if (nAttacker == kSlotMine && nAttack == kSlotMine)
    {
        *pResult = pArgs->m_nArgCount - 3;

        return;
    }

    if (nAttacker == static_cast<short>(nAttack))
    {
        if (nAttacker >= kSlotTurnstile && nAttacker <= kSlotTurnstileLast)
        {
            *pResult = pArgs->m_nArgCount - 3;

            return;
        }

        if (nAttacker >= kSlotLaserHurdle && nAttacker <= kSlotLaserHurdleLast)
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

    auto* const pOther = g_pGameEngine->m_ppSlots[nAttacker];

    const Vec3* const pPosSelf  = pActor->GetPositionPtr();
    const Vec3* const pPosOther = pOther->GetPositionPtr();

    vOffset.x = pPosOther->x - pPosSelf->x;
    vOffset.y = pPosOther->y - pPosSelf->y;
    vOffset.z = pPosOther->z - pPosSelf->z;

    if (nAttacker <= kSlotDromer3)
    {
        auto* const pOtherStats = &((PlayerObject*)pOther)->m_stats;

        flOtherChecking = pOtherStats->GetWeightedStat(kRatingChecking);
        flOtherWeight   = pOtherStats->GetWeightedStat(kRatingWeightClass);
        flOtherDamage   = pOtherStats->GetWeightedStat(kRatingDamage);
    }
    else
    {
        flOtherChecking = DAMAGE_STANDIN_CHECKING;
        flOtherWeight   = STANDIN_WEIGHT;
        flOtherDamage   = STANDIN_DAMAGE;
    }

    auto* const pStats = &pActor->m_stats;

    pStats->GetScalar(kRatingDamage);

    const auto flOwnWeight = pStats->GetWeightedStat(kRatingWeightClass);

    if (g_pGameEngine->GetDifficultyLevel() == DifficultyLevel::kEasy)
    {
        flRange = RANGE_BASE;
    }
    else
    {
        flRange = 0.0f;
    }

    if (Vec3::DotInline(&vOffset, &vOffset) <= flRange + RANGE_BASE && (pStats->GetScalar(kRatingImmunity) == 0.0f || nAttack == kAttackLegSweepL || nAttack == kAttackLegSweepR))
    {
        pStats->GetScalar(kRatingDamage);

        Vec3         vSelfDir;
        Vec3         vOtherDir;
        PhysicsBody* pBlockOther;
        PhysicsBody* pBlockSelf;

        switch (nAttack)
        {
            case kAttackHipCheck:
                flStrength = flOtherDamage * flOtherChecking;

                flStrength = flStrength * 0.01f + flOtherChecking;

                flStrength *= 0.5f;

                flStrength += 9.0f;

                flDamage = flStrength * -1.0f;

                break;

            case kAttackPunch:
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

            case kAttackClothesLineL:
            case kAttackClothesLineR:
            case kAttackShoulderCheckL:
            case kAttackShoulderCheckR:
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

        auto nMagnitude = abs(static_cast<short>(flDamage));

        if (nMagnitude > kHitDamageCeiling)
        {
            nMagnitude = kHitDamageCeiling;
        }

        *pResult = static_cast<short>(nMagnitude);

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
void PlanCond::AttackedAnim(Interactor* pContext, PlanNode* pArgs, short* pResult, int nAttack)
{
    int   nReaction;
    float flOther;
    float flWeight;
    Vec3  vOffset;

    // Only the z is ever written.
    Vec3 vCross;

    auto* pActor = pContext->GetPlayerActor();

    const auto nSlot = pActor->m_wAttackeeSlot;

    pActor->m_wAttackeeSlot = -1;

    if (nSlot == -1)
    {
        *pResult = pArgs->m_nArgCount - 2;

        return;
    }

    auto* const pStats = &pActor->m_stats;

    auto flDamage = pStats->GetScalar(kRatingDamage);

    if (flDamage <= DamageGate())
    {
        *pResult = pArgs->m_nArgCount - 3;

        switch (nAttack)
        {
            case kAttackHipCheck:
                pActor->m_wKilledBySlot = kSlotTurnstile;
                break;
        }

        return;
    }

    if (nAttack == kAttackPunch)
    {
        *pResult = pArgs->m_nArgCount - 2;

        return;
    }

    auto* const pOther = g_pGameEngine->m_ppSlots[nSlot];

    const Vec3* const pPosSelf  = pActor->GetPositionPtr();
    const Vec3* const pPosOther = pOther->GetPositionPtr();

    vOffset.x = pPosOther->x - pPosSelf->x;
    vOffset.y = pPosOther->y - pPosSelf->y;
    vOffset.z = pPosOther->z - pPosSelf->z;

    if (nSlot <= kSlotDromer3)
    {
        auto* const pOtherStats = &((PlayerObject*)pOther)->m_stats;

        flOther  = pOtherStats->GetWeightedStat(kRatingChecking);
        flWeight = pOtherStats->GetWeightedStat(kRatingWeightClass);
    }
    else
    {
        flOther  = ATTACKED_STANDIN_CHECKING;
        flWeight = 20.0f;
    }

    flOther += flWeight;

    flDamage = pStats->GetScalar(kRatingDamage);

    const auto flOwnWeight = pStats->GetWeightedStat(kRatingWeightClass);

    auto flScaled = flOwnWeight;

    flScaled *= flDamage;

    const auto flSelf = flScaled * 0.01f + flOwnWeight;

    Math::Vec3Normalize(&vOffset);

    const Vec3* pDir = &pActor->GetPhysicsBody()->velDir;

    vCross.z = vOffset.y * pDir->x - vOffset.x * pDir->y;

    pDir = &pActor->GetPhysicsBody()->velDir;

    const auto flDot = vOffset.z * pDir->z + vOffset.y * pDir->y + vOffset.x * pDir->x;

    if (Vec3::DotInline(&vOffset, &vOffset) <= ATTACKED_RANGE_SQ && (pStats->GetScalar(kRatingImmunity) == 0.0f || nAttack == kAttackLegSweepL || nAttack == kAttackLegSweepR))
    {
        if (g_flTwoPlan + flOther > flSelf)
        {
            switch (nAttack)
            {
                case kAttackPunch:
                    if (flDot > 0.0f)
                    {
                        nReaction = 0;
                    }
                    else
                    {
                        nReaction = 1;
                    }

                    *pResult = static_cast<short>(nReaction);

                    return;

                case kAttackHipCheck:
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

                    *pResult = static_cast<short>(nReaction);

                    return;

                case kAttackClothesLineL:
                case kAttackClothesLineR:
                    if (flDot > 0.0f)
                    {
                        nReaction = 6;
                    }
                    else
                    {
                        nReaction = 7;
                    }

                    *pResult = static_cast<short>(nReaction);

                    return;

                case kAttackShoulderCheckL:
                case kAttackShoulderCheckR:
                    if (flDot > DOT_BEHIND)
                    {
                        nReaction = 8;
                    }
                    else
                    {
                        nReaction = 9;
                    }

                    *pResult = static_cast<short>(nReaction);

                    return;

                case kAttackLegSweepL:
                case kAttackLegSweepR:
                    if (flDot > 0.0f)
                    {
                        nReaction = 10;
                    }
                    else
                    {
                        nReaction = 11;
                    }

                    *pResult = static_cast<short>(nReaction);

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
                case kAttackPunch:
                    if (flDot > 0.0f)
                    {
                        nReaction = 0;
                    }
                    else
                    {
                        nReaction = 1;
                    }

                    *pResult = static_cast<short>(nReaction);

                    return;

                case kAttackHipCheck:
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

                    *pResult = static_cast<short>(nReaction);

                    return;

                case kAttackClothesLineL:
                case kAttackClothesLineR:
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

                    *pResult = static_cast<short>(nReaction);

                    return;

                case kAttackShoulderCheckL:
                case kAttackShoulderCheckR:
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

                    *pResult = static_cast<short>(nReaction);

                    return;

                case kAttackLegSweepL:
                case kAttackLegSweepR:
                    if (flDot > 0.0f)
                    {
                        nReaction = 10;
                    }
                    else
                    {
                        nReaction = 11;
                    }

                    *pResult = static_cast<short>(nReaction);

                    return;
            }
        }
    }

    *pResult = pArgs->m_nArgCount - 2;
}

// The straight arm is looked up and never read; the shoulder checks split left from right only in states nine and thirteen.

// GameEngine::SetTeams writes the first from its home argument and the second from its away one.
// FUNCTION: HYPERX 0x00466190
void PlanCond::SpecialAttacks(Interactor* pContext, PlanNode*, short* pResult, int nAnim)
{
    short nState;

    auto* const pActor = pContext->GetActor();

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
    const auto nHipcheck = g_pAnimRegistry->FindAnimByNameIdx("AA_Hipcheck");

    // STRING: HYPERX 0x004c463c
    const auto nClothesLineL = g_pAnimRegistry->FindAnimByNameIdx("AA_ClthslneL");

    // STRING: HYPERX 0x004c462c
    const auto nClothesLineR = g_pAnimRegistry->FindAnimByNameIdx("AA_ClthslneR");

    // STRING: HYPERX 0x004c4620
    const auto nShoulderL = g_pAnimRegistry->FindAnimByNameIdx("AA_ShlchekL");

    // STRING: HYPERX 0x004c4614
    const auto nShoulderR = g_pAnimRegistry->FindAnimByNameIdx("AA_ShlchekR");

    // STRING: HYPERX 0x004c4608
    const auto nLegTripL = g_pAnimRegistry->FindAnimByNameIdx("AA_LegtripL");

    // STRING: HYPERX 0x004c45fc
    const auto nLegTripR = g_pAnimRegistry->FindAnimByNameIdx("AA_LegtripR");

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
void PlanCond::MeOrTeammateClosestToRok(Interactor* pContext, PlanNode*, short* pResult, int)
{
    const Vec3* pRokAt;
    const Vec3* pOppAt;
    AttachPak*  pnBoundSlot;
    short       nBound;
    Vec3        vOpp;

    pRokAt = g_pGameEngine->m_ppSlots[kSlotRok]->GetPositionPtr();

    const Vec3* const pOwnerAt = pContext->GetActor()->GetPositionPtr();

    const auto nSlotA = pContext->GetActor()->GetSlot();

    pOppAt = g_pGameEngine->m_ppSlots[TeammateSlot(nSlotA)]->GetPositionPtr();

    const auto flOwnX = pRokAt->x - pOwnerAt->x;
    const auto flOwnY = pRokAt->y - pOwnerAt->y;
    const auto flOwnZ = pRokAt->z - pOwnerAt->z;

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

    const auto nSlotB = pContext->GetActor()->GetSlot();

    auto* const pOppObj = g_pGameEngine->GetPlayer(TeammateSlot(nSlotB));

    const auto flDamage = pOppObj->m_stats.GetScalar(kRatingDamage);

    if (*pResult == 1)
    {
        if (flDamage <= DamageGate())
        {
            *pResult = 0;
        }
    }

    const int nSlotC = pContext->GetActor()->GetSlot();

    const auto nOppC = TeammateSlot(static_cast<short>(nSlotC));

    pnBoundSlot = g_pGameEngine->m_ppSlots[kSlotRok]->GetPhysicsBody()->pAttach;

    if (pnBoundSlot != nullptr && (nBound = pnBoundSlot->nSlot) < kPlayerObjectCount && (nBound == nSlotC || nBound == nOppC))
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

#define GREEN_RANDOMIZER_COUNT (kSlotRedRandomizer - kSlotGreenRandomizer)

// The answer is one past the chosen slot, so zero means none.
// FUNCTION: HYPERX 0x004665f0
void PlanCond::RedIsAvailable(Interactor*, PlanNode*, short* pResult, int)
{
    int aFound[GREEN_RANDOMIZER_COUNT];

    if (g_pGameEngine->m_ppSlots[kSlotRedRandomizer]->GetPhysicsBody()->axis.x > g_flZeroPlan)
    {
        auto nSlot  = 0;
        auto nCount = 0;
        int* pFound = aFound;

        for (; nSlot < GREEN_RANDOMIZER_COUNT; nSlot++)
        {
            if (g_pGameEngine->m_ppSlots[static_cast<short>(nSlot + kSlotGreenRandomizer)] != nullptr)
            {
                *pFound = nSlot;
                nCount++;
                pFound++;
            }
        }

        if (nCount != 0)
        {
            *pResult = static_cast<short>(aFound[rand() % nCount] + 1);

            return;
        }
    }

    *pResult = 0;
}

#define SPEED_GATE 10.0f

// The rating index comes from the node, so one condition thresholds any stat.
// FUNCTION: HYPERX 0x00466680
void PlanCond::RatingMaxedOut(Interactor* pContext, PlanNode* pNode, short* pnResult, int nArg)
{
    const auto nDefaultSlot = static_cast<short>(nArg);

    PlayerObject* pObject;

    if (pContext->GetNamedSlot() == -1)
    {
        auto* const pBubble = g_pGameEngine->GetColObjPtr(pContext->GetModelSlot());

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
void PlanCond::ChooseTeamTune(Interactor* pContext, PlanNode*, short* pResult, int nTeam)
{
    int   nIndex;
    short nTeamIndex;

    auto nSelected = pContext->GetNamedSlot();

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
void PlanCond::MyTeamLosingByAtLeast(Interactor* pContext, PlanNode*, short* pResult, int nArg)
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
    auto* const pActor = pContext->GetPlayerActor();

    const auto nStat = pArgs->m_pArgs[1].pNode->m_pArgs[0].n;

    const auto nRange = static_cast<short>(pArgs->m_nArgCount - 2);

    *pResult = static_cast<short>(static_cast<int>(pActor->m_stats.GetScalar(nStat) * (nRange * g_flPlanStatScale)));

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
    const auto nBase = static_cast<short>(nArg);

    EngineObject* pActor;
    AttachPak*    pSlot;
    short         nSlot;

    auto* pTracked = g_pGameEngine->m_ppSlots[kSlotSwarf];

    if (pTracked != nullptr)
    {
        pSlot = pTracked->GetPhysicsBody()->pAttach;

        if (pSlot != nullptr)
        {
            nSlot = pSlot->nSlot;

            if (nSlot < kPlayerObjectCount)
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

    pTracked = g_pGameEngine->m_ppSlots[kSlotMine];

    if (pTracked != nullptr)
    {
        pSlot = pTracked->GetPhysicsBody()->pAttach;

        if (pSlot != nullptr)
        {
            nSlot = pSlot->nSlot;

            if (nSlot < kPlayerObjectCount)
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

    pTracked = g_pGameEngine->m_ppSlots[kSlotRok];

    if (pTracked != nullptr && ((RokObject*)pTracked)->m_nKillState > -1)
    {
        pSlot = pTracked->GetPhysicsBody()->pAttach;

        if (pSlot != nullptr)
        {
            nSlot = pSlot->nSlot;

            if (nSlot < kPlayerObjectCount)
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

    *pResult = static_cast<short>(nBase + 1);
}

// FUNCTION: HYPERX 0x00466960
void PlanCond::DromerNearMine(Interactor*, PlanNode* pArgs, short* pResult, int nArg)
{
    const auto nSlot = static_cast<short>(nArg);

    auto* const pTarget = g_pGameEngine->m_ppSlots[nSlot];
    const auto  nLimit  = static_cast<short>(pArgs->m_pArgs[1].pNode->m_pArgs[0].n);
    auto* const pTeam   = g_pGameEngine->m_ppSlots[kSlotMine];

    // A missing reference object leaves the answer alone rather than writing one.
    if (pTeam != nullptr)
    {
        auto* const pFrom = pTeam->GetPositionPtr();
        auto* const pTo   = pTarget->GetPositionPtr();

        const auto dx = pTo->x - pFrom->x;
        const auto dy = pTo->y - pFrom->y;
        const auto dz = pTo->z - pFrom->z;

        // This one takes the root, unlike its neighbour: the limit here is a plain distance.
        if (static_cast<float>(sqrt(static_cast<double>(dx * dx + dy * dy + dz * dz))) <= nLimit)
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
    auto* const pCarrier = g_pGameEngine->GetRok();

    auto nDirection = pCarrier->m_wRokValue;

    auto* const pState = pCarrier->GetPhysicsBody()->pAttach;

    if (pState != nullptr && pState->nSlot < 6)
    {
        if ((pState->nSlot & 1) != 0)
        {
            nDirection = static_cast<short>(-nDirection);
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
void PlanCond::CheckPassingDistance(Interactor* pContext, PlanNode*, short* pResult, int)
{
    Vec3 vDiff;

    auto* const pActor = pContext->GetActor();

    const int nSlot = pActor->GetSlot();

    auto* const pMate = g_pGameEngine->GetPlayer(TeammateSlot(static_cast<short>(nSlot)));

    auto* const pSelfPos = pContext->GetActor()->GetPositionPtr();

    auto* const pMatePos = pMate->GetPositionPtr();

    vDiff.x = pMatePos->x - pSelfPos->x;
    vDiff.y = pMatePos->y - pSelfPos->y;
    vDiff.z = pMatePos->z - pSelfPos->z;

    if (pMate->m_stats.GetScalar(kRatingLongPassEnable) != 0.0f)
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
void PlanCond::RecvrInRangeNView(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg)
{
    const PlanWord param = { nArg };

    unsigned int nOperand;
    Vec3         vDelta;
    Vec3         vScratch;

    auto* const pActor = pContext->GetActor();

    if (param.IsPronoun())
    {
        nOperand = g_apPlanStatFns[param.wPronoun](pContext->GetActor(), pArgs);
    }
    else
    {
        nOperand = param.n;
    }

    auto* const pTarget = g_pGameEngine->GetPlayer(static_cast<short>(nOperand));

    if ((nOperand & 0xffff) > kSlotDromer3)
    {
        // STRING: HYPERX 0x004bb348
        g_gameApp.Fatal("Fatal failure to initialize the receiver of a pass!");
    }

    const auto flDamage = pTarget->m_stats.GetScalar(kRatingDamage);

    const Vec3* const pActorPos  = pContext->GetActor()->GetPositionPtr();
    const Vec3* const pTargetPos = pTarget->GetPositionPtr();

    vDelta.x = pTargetPos->x - pActorPos->x;
    vDelta.y = pTargetPos->y - pActorPos->y;
    vDelta.z = pTargetPos->z - pActorPos->z;

    const auto flEnable = pTarget->m_stats.GetScalar(kRatingLongPassEnable);

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

    const auto nTargetSlot = pTarget->GetSlot();
    const auto nActorSlot  = pActor->GetSlot();

    auto* const pTargetSlot = g_pGameEngine->m_ppSlots[nTargetSlot];

    vDelta = *pTargetSlot->GetPositionPtr()->Subtract(&vScratch,
                                                      g_pGameEngine->m_ppSlots[nActorSlot]->GetPositionPtr());

    const auto flLength = Vec3::Length(&vDelta);

    const auto flScale = g_flPiPlan / g_flThreePlan;

    const auto flFalloff = (SCORE_RANGE - flLength) * flScale * SCORE_FALLOFF;

    const auto flCos = static_cast<float>(cos(static_cast<double>(flFalloff + flScale)));

    if (pActor->GetDistIfFacing(pTarget, flCos) == 0.0f)
    {
        *pResult = 1;
    }

    const auto nSlot = pContext->GetActor()->GetSlot();

    auto* const pMate = g_pGameEngine->m_ppSlots[TeammateSlot(nSlot)];

    if (g_pGameEngine->m_nTeammateMode == 0)
    {
        // STRING: HYPERX 0x004bb340
        if (strcmp("Human",
                   pMate->GetPlanState()->m_pszName) == 0)
        {
            *pResult = 0;
        }
    }

    if (flDamage <= DamageGate())
    {
        *pResult = 1;
    }
}

constexpr auto kMiddleSlotBias = 4;

// Squared distances, so the limits of ten and just under eight read as a hundred and sixty.
// FUNCTION: HYPERX 0x00466e20
void PlanCond::ChoosePassingPlay(Interactor* pContext, PlanNode*, short* pResult, int)
{
    float flSign;
    Vec3  vNear;
    Vec3  vFar;
    Vec3  vSpan;

    auto* const pActor = pContext->GetPlayerActor();

    const auto nSlot = pActor->GetSlot();

    int nMiddle = nSlot;

    if (nSlot < kMiddleSlotBias)
    {
        nMiddle += 2;
    }

    const auto nNearSlot = OPPOSING_FIRST(nSlot);

    auto* const pEngine = g_pGameEngine;

    auto* const pNear = pEngine->m_ppSlots[nNearSlot];

    const auto nFarSlot = nNearSlot | 2;

    auto* const pMiddle = pEngine->m_ppSlots[nMiddle & 3];
    auto* const pFar    = pEngine->m_ppSlots[nFarSlot];

    if (pActor->m_wTargetGoalSlot == kSlotNgoal)
    {
        flSign = g_flOnePlan;
    }
    else
    {
        flSign = -g_flOnePlan;
    }

    const Vec3* const pSelfNear = pActor->GetPositionPtr();
    const Vec3* const pNearAt   = pNear->GetPositionPtr();

    vNear.x = pNearAt->x - pSelfNear->x;
    vNear.y = pNearAt->y - pSelfNear->y;
    vNear.z = pNearAt->z - pSelfNear->z;

    const Vec3* const pSelfFar = pActor->GetPositionPtr();
    const Vec3* const pFarAt   = pFar->GetPositionPtr();

    vFar.x = pFarAt->x - pSelfFar->x;
    vFar.y = pFarAt->y - pSelfFar->y;
    vFar.z = pFarAt->z - pSelfFar->z;

    const auto flLimit = flSign * pActor->GetPositionPtr()->y + 20.0f;

    if (flLimit < flSign * pMiddle->GetPositionPtr()->y || vNear.z * vNear.z + vNear.y * vNear.y + vNear.x * vNear.x < 100.0 || vFar.z * vFar.z + vFar.y * vFar.y + vFar.x * vFar.x < 100.0)
    {
        const Vec3* const pMidSpan = pMiddle->GetPositionPtr();
        const Vec3* const pSpanAt  = pNear->GetPositionPtr();

        vSpan.x = pSpanAt->x - pMidSpan->x;
        vSpan.y = pSpanAt->y - pMidSpan->y;
        vSpan.z = pSpanAt->z - pMidSpan->z;

        const Vec3* const pMidWide = pMiddle->GetPositionPtr();
        const Vec3* const pWideAt  = pFar->GetPositionPtr();

        const auto flWideX = pWideAt->x - pMidWide->x;
        const auto flWideY = pWideAt->y - pMidWide->y;
        const auto flWideZ = pWideAt->z - pMidWide->z;

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

    const auto flDamage = ((PlayerObject*)pMiddle)->m_stats.GetScalar(kRatingDamage);

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

    auto* const pColObj = g_pGameEngine->GetColObjPtr(pContext->GetModelSlot());

    const AnimDB* const pSet = pColObj->GetOwner()->m_pAnimPlayer->m_pSet;

    if (pSet != nullptr)
    {
        pszName = pSet->GetName();
    }
    else
    {
        pszName = g_pszEmptyName;
    }

    // STRING: HYPERX 0x004c4664
    *pResult = strcmp(pszName, "AA_QuikDuck") != 0;
}

// Two is added while the rok sits at its spawn slot.

// FUNCTION: HYPERX 0x00467110
void PlanCond::RokIsCharged(Interactor*, PlanNode*, short* pResult, int)
{
    auto* const pRok = g_pGameEngine->GetRok();

    const short nFlags = pRok->m_nKillState == -1;
    *pResult           = nFlags;

    if (pRok->m_bAtSpawnSlot != 0)
    {
        *pResult = static_cast<short>(nFlags + 2);
    }
}

// The collision object's kind is fetched and never read; only the rok's own word decides.

// FUNCTION: HYPERX 0x00467150
void PlanCond::RokIsChargedAgainst(Interactor* pContext, PlanNode*, short* pResult, int)
{
    auto* const pCarrier = g_pGameEngine->GetRok();

    if (pContext->GetNamedSlot() == -1)
    {
        auto* const pColObj = g_pGameEngine->GetColObjPtr(pContext->GetOtherSlot());

        pColObj->GetOwner()->GetSlot();
    }

    *pResult = pCarrier->m_nKillState == -1;
}

#define TURNSTILE_SPIN 10.0f
#define TURNSTILE_RATE 0.95f

// The spin is minus ten or ten by which side of the object's velocity the actor stands on.
// FUNCTION: HYPERX 0x004671c0
void PlanAction::SpinTurnstile(Interactor* pContext, PlanNode*, short*, int nArg)
{
    const auto nSlot = static_cast<short>(nArg);

    Vec3  vDiff;
    Vec3  vImpulse;
    float flSpin;

    auto* pObject = g_pGameEngine->m_ppSlots[nSlot];

    const Vec3* const pVelocity = &pObject->GetPhysicsBody()->vel;

    vDiff.x = pObject->GetPositionPtr()->x - pContext->GetActor()->GetPositionPtr()->x;
    vDiff.y = pObject->GetPositionPtr()->y - pContext->GetActor()->GetPositionPtr()->y;
    vDiff.z = pObject->GetPositionPtr()->z - pContext->GetActor()->GetPositionPtr()->z;

    auto* const pRelax = (CRelaxYaw*)AllocAgendaItem(Agenda_CRelaxYaw);

    vImpulse.z = vDiff.y * pVelocity->x - vDiff.x * pVelocity->y;

    pRelax->Bind(pContext->GetActor(), nullptr);

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
void PlanCond::SpinUp(Interactor* pContext, PlanNode*, short* pResult, int nArg)
{
    const auto nSlot = static_cast<short>(nArg);

    Vec3  vDelta;
    Vec3  vUnit;
    float flLength;

    auto* const pTarget = g_pGameEngine->m_ppSlots[nSlot];

    // The original has no guard here, and a run can reach it with an empty slot.
    if (pTarget == nullptr)
    {
        *pResult = 1;
        return;
    }

    pTarget->GetPhysicsBody();
    const Vec3* const pForward = &pTarget->GetPhysicsBody()->velDir;

    auto* pActor = pContext->GetActor();
    vDelta.x     = pTarget->GetPositionPtr()->x - pActor->GetPositionPtr()->x;

    pActor   = pContext->GetActor();
    vDelta.y = pTarget->GetPositionPtr()->y - pActor->GetPositionPtr()->y;

    pActor   = pContext->GetActor();
    vDelta.z = pTarget->GetPositionPtr()->z - pActor->GetPositionPtr()->z;

    flLength = static_cast<float>(sqrt(static_cast<double>(vDelta.x * vDelta.x + vDelta.y * vDelta.y + vDelta.z * vDelta.z)));

    const auto flScale = 1.0f / flLength;

    vUnit.x = flScale * vDelta.x;
    vUnit.y = flScale * vDelta.y;
    vUnit.z = flScale * vDelta.z;

    const auto flDot = vUnit.x * pForward->x + vUnit.y * pForward->y + vUnit.z * pForward->z;

    if ((1.0f - flDot * flDot) * flLength * flLength < g_flInterceptCorridorSq)
    {
        *pResult = 0;
        return;
    }

    *pResult = 1;
}

// Unwinds the last pitch through the staged sine, then applies the node's angle.

// The angle, in whole degrees.
constexpr auto kArgDegrees = 1;

#define DEG_TO_RAD 0.017453292f

// FUNCTION: HYPERX 0x00467410
void PlanAction::ModifyPitch(Interactor* pContext, PlanNode* pNode, short*, int)
{
    EngineObject* pObject;

    if (pContext->GetNamedSlot() == -1)
    {
        auto* const pColObj = g_pGameEngine->GetColObjPtr(pContext->GetOtherSlot());

        pObject = pColObj->GetOwner();
    }
    else
    {
        pObject = pContext->GetActor();
    }

    auto flAngle = -static_cast<float>(Math::Asin(pObject->GetLocalTransformPtr()->m_rotX.flSin));

    auto* pTransform = pObject->GetLocalTransformPtr();

    pTransform->m_rotX.flCos = static_cast<float>(cos(static_cast<double>(flAngle)));
    pTransform->m_rotX.flSin = static_cast<float>(sin(static_cast<double>(flAngle)));

    pTransform->RotateXStaged();

    flAngle = static_cast<float>(static_cast<unsigned int>(pNode->m_pArgs[kArgDegrees].n)) * DEG_TO_RAD;

    pTransform = pObject->GetLocalTransformPtr();

    pTransform->m_rotX.flCos = static_cast<float>(cos(static_cast<double>(flAngle)));
    pTransform->m_rotX.flSin = static_cast<float>(sin(static_cast<double>(flAngle)));

    pTransform->RotateXStaged();
}

// The two bungees differ only in range: fifty five for the first slot, forty five for the second.

constexpr auto kArgFirstSlot  = 1;
constexpr auto kArgSecondSlot = 2;

#define DRAG_BUNGEE_GAIN         0.15f
#define DRAG_BUNGEE_REST_LENGTH  10.0f
#define DRAG_BUNGEE_FIRST_RANGE  55.0f
#define DRAG_BUNGEE_SECOND_RANGE 45.0f
constexpr auto kDragBungeeDuration = 150;

// FUNCTION: HYPERX 0x004674e0
void PlanAction::HookDragBungeesOn(Interactor* pContext, PlanNode* pNode, short*, int)
{
    auto* const pTarget = pContext->GetActor();

    auto* const          pArgs     = pNode->m_pArgs;
    EngineObject** const apObjects = g_pGameEngine->m_ppSlots;

    auto* const pFirst  = apObjects[static_cast<short>(pArgs[kArgFirstSlot].n)];
    auto* const pSecond = apObjects[static_cast<short>(pArgs[kArgSecondSlot].n)];

    auto* const pFirstBungee  = (CBungeeToward*)AllocAgendaItem(Agenda_CBungeeToward);
    auto* const pSecondBungee = (CBungeeToward*)AllocAgendaItem(Agenda_CBungeeToward);

    auto* pBlock = pFirst->GetPlanState();
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

    pFirstBungee->m_nCountdown  = kDragBungeeDuration;
    pSecondBungee->m_nCountdown = kDragBungeeDuration;

    pFirstBungee->Activate();
    pSecondBungee->Activate();
}

constexpr auto kArgKindMask = 2;

// No plan calls this; the field is the one every CTrackTraj range setup clears.
// FUNCTION: HYPERX 0x004675d0
void PlanAction::ResetAccTransient(Interactor* pContext, PlanNode*, short*, int)
{
    auto* const pActor     = pContext->GetActor();
    auto* const pPlanState = pActor->GetPlanState();

    static_cast<CTrackTraj*>(pPlanState->m_pCurrent->m_apSlots[0])->m_flBlend = g_flZeroPlan;
}

// Every input device is written as a KeyboardDeviceObj, so on the 0xd28-byte gamepad, joystick and mouse the
// mask lands past the object, as the original's writes at 0x0046763a and 0x00467649 do (allocated at 0x004863f0).
// FUNCTION: HYPERX 0x004675f0
void PlanAction::ResetKeystateFlags(Interactor* pContext, PlanNode* pNode, short*, int)
{
    auto* const pOwner = pContext->GetActor();

    if (pOwner->m_nKind == KIND_KEYBOARD || pOwner->m_nKind == KIND_GAMEPAD || pOwner->m_nKind == KIND_JOYSTICK || pOwner->m_nKind == KIND_MOUSE)
    {
        if (pNode->m_nArgCount == kArgKindMask)
        {
            EngineObject* pTarget;
            unsigned int  nMask;
            unsigned int  nActions;

            nMask = static_cast<unsigned int>(pNode->m_pArgs[1].n);

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

constexpr auto kArgCurve = 1;

// FUNCTION: HYPERX 0x004676e0
void PlanAction::IncrAnimSpeedWRating(Interactor* pContext, PlanNode* pNode, short*, int)
{
    float flDamage;
    float flScale;

    const auto nCurve = pNode->m_pArgs[kArgCurve].n;

    auto* const pActor = pContext->GetPlayerActor();

    flDamage = pActor->m_stats.GetScalar(kRatingDamage) * 0.01f;

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

constexpr auto kArgRatingBase = 1;
#undef ARG_PERIOD
#define ARG_PERIOD 2

// FUNCTION: HYPERX 0x00467760
void PlanAction::SpeedBurstSituation(Interactor* pContext, PlanNode* pNode, short*, int)
{
    float flScale;

    if (pContext->GetPlayerActor()->m_stats.m_flSpeedRatio >= 0.25f)
    {
        auto nRating = pNode->m_pArgs[kArgRatingBase].n;

        if (GameEngine::IsTeamWinning(static_cast<unsigned char>(pContext->GetActor()->GetSlot())) != 0)
        {
            nRating *= 2;
        }
        else
        {
            nRating = nRating * 2 + 1;
        }

        if (static_cast<int>(pContext->GetPlayerActor()->m_stats.GetScalar(nRating + kRatingSpeedBurst0a)) != 0)
        {
            auto* const pBurst = (CSpeedBurst*)AllocAgendaItem(Agenda_CSpeedBurst);

            if (pContext->GetPlayerActor()->m_stats.m_flSpeedRatio > 0.3)
            {
                flScale = g_flOnePlan;
            }
            else
            {
                flScale = g_flZeroPlan;
            }

            auto* const pCurrentBehaviour = pContext->GetActor()->GetPlanState()->m_pCurrent;

            pBurst->Bind(pContext->GetActor(), pCurrentBehaviour);
            pBurst->Activate();

            pBurst->m_flTarget =
                static_cast<float>(80.0 / static_cast<unsigned int>(pNode->m_pArgs[ARG_PERIOD].n) * flScale + g_flOnePlan);

            *pBurst->m_pBoost = 1.05f;

            auto* pFirst = pContext->GetActor()->GetPhysicsBody();

            auto* const pSpin  = &pFirst->axis;
            const auto  flSpin = pSpin->x + 0.25f;

            pSpin->x = flSpin;

            pFirst = pContext->GetActor()->GetPhysicsBody();

            pFirst->flFriction = 0.1f;

            pFirst = pContext->GetActor()->GetPhysicsBody();

            auto* const pVel = &pFirst->vel;

            const auto flLen = static_cast<float>(sqrt(static_cast<double>(pVel->x * pVel->x + pVel->y * pVel->y + pVel->z * pVel->z)));

            if (flLen < 0.1)
            {
                pFirst              = pContext->GetActor()->GetPhysicsBody();
                auto* const pSecond = pContext->GetActor()->GetPhysicsBody();

                pSecond->vel = pFirst->velDir;
            }
        }
    }
}

// Eight hundred, the ceiling both squared-distance thresholds are clamped to.
// GLOBAL: HYPERX 0x004b378c
extern const float g_flRangeLimit;

#define RATINGS_RANGE 800.0f

constexpr auto kOneInChance = 8;

// One roll in eight inverts the answer, and a local match never attacks around a human team mate.
// FUNCTION: HYPERX 0x00467930
void PlanCond::PassOrRunByRatings(Interactor* pContext, PlanNode*, short* pResult, int)
{
    float flSign;
    Vec3  vNear;
    Vec3  vFar;
    Vec3  vMateNear;
    Vec3  vMateFar;

    auto* const pOwner = pContext->GetPlayerActor();

    const auto nSlot = pOwner->GetSlot();

    const auto nMate = TeammateSlot(nSlot);

    EngineObject** const apObjects = g_pGameEngine->m_ppSlots;

    auto* const pNear = apObjects[OPPOSING_FIRST(nSlot)];

    auto* const pMate = apObjects[nMate];

    auto* const pFar = apObjects[OPPOSING_SECOND(nSlot)];

    if (pOwner->m_wTargetGoalSlot == kSlotNgoal)
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

    auto* pBody = pContext->GetPlayerActor();

    auto flLead = pBody->m_stats.GetScalar(kRatingPass0);

    pBody = pContext->GetPlayerActor();

    auto flOutside = pBody->m_stats.GetScalar(kRatingPass1);

    pBody = pContext->GetPlayerActor();

    auto flInside = pBody->m_stats.GetScalar(kRatingPass2);

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

    const auto flLimit = flSign * pOwner->GetPositionPtr()->y + flLead;

    if ((flSign * pMate->GetPositionPtr()->y <= flLimit && vNear.z * vNear.z + vNear.y * vNear.y + vNear.x * vNear.x >= flOutside && vFar.z * vFar.z + vFar.y * vFar.y + vFar.x * vFar.x >= flOutside) || vMateNear.z * vMateNear.z + vMateNear.y * vMateNear.y + vMateNear.x * vMateNear.x <= flInside || vMateFar.z * vMateFar.z + vMateFar.y * vMateFar.y + vMateFar.x * vMateFar.x <= flInside)
    {
        *pResult = 1;
    }
    else
    {
        *pResult = 0;
    }

    if (rand() % kOneInChance == 0)
    {
        *pResult = !*pResult;
    }

    if (g_pGameEngine->m_nTeammateMode == 0)
    {
        // STRING: HYPERX 0x004bb340
        if (strcmp("Human",
                   pMate->GetPlanState()->m_pszName) == 0)
        {
            *pResult = 0;
        }
    }
}

// Anything outside zero to three becomes zero, the widest a four-child DECIDE can be.

constexpr auto kBranchMax = 3;

// FUNCTION: HYPERX 0x00467c30
void PlanCond::LooseRokWhatToDo(Interactor* pContext, PlanNode*, short* pResult, int)
{
    PlayerObject* pOwner;
    const Vec3*   pRokAt;
    const Vec3*   pOppAt;
    Vec3          vOpp;
    float         flStat;

    pRokAt = g_pGameEngine->m_ppSlots[kSlotRok]->GetPositionPtr();

    const Vec3* const pOwnerAt = pContext->GetActor()->GetPositionPtr();

    const auto nSlot = pContext->GetActor()->GetSlot();

    pOppAt = g_pGameEngine->m_ppSlots[TeammateSlot(nSlot)]->GetPositionPtr();

    const auto flOwnX = pRokAt->x - pOwnerAt->x;
    const auto flOwnY = pRokAt->y - pOwnerAt->y;
    const auto flOwnZ = pRokAt->z - pOwnerAt->z;

    vOpp.x = pRokAt->x - pOppAt->x;
    vOpp.y = pRokAt->y - pOppAt->y;
    vOpp.z = pRokAt->z - pOppAt->z;

    if (flOwnX * flOwnX + flOwnY * flOwnY + flOwnZ * flOwnZ < vOpp.x * vOpp.x + vOpp.y * vOpp.y + vOpp.z * vOpp.z)
    {
        if (GameEngine::IsTeamWinning(static_cast<unsigned char>(pContext->GetActor()->GetSlot())) != 0)
        {
            pOwner = pContext->GetPlayerActor();

            flStat = pOwner->m_stats.GetScalar(kRatingWinCloseFreeRok);
        }
        else
        {
            pOwner = pContext->GetPlayerActor();

            flStat = pOwner->m_stats.GetScalar(kRatingLoseCloseFreeRok);
        }
    }
    else
    {
        if (GameEngine::IsTeamWinning(static_cast<unsigned char>(pContext->GetActor()->GetSlot())) != 0)
        {
            pOwner = pContext->GetPlayerActor();

            flStat = pOwner->m_stats.GetScalar(kRatingWinFarFreeRok);
        }
        else
        {
            pOwner = pContext->GetPlayerActor();

            flStat = pOwner->m_stats.GetScalar(kRatingLoseFarFreeRok);
        }
    }

    const auto nStat = static_cast<int>(flStat);

    if (nStat < 0 || nStat > kBranchMax)
    {
        *pResult = 0;
    }
    else
    {
        *pResult = static_cast<unsigned short>(nStat);
    }

    auto* pOpponent = g_pGameEngine->GetPlayer(OPPOSING_FIRST(pContext->GetActor()->GetSlot()));

    const auto flDamageA = pOpponent->m_stats.GetScalar(kRatingDamage);

    pOpponent = g_pGameEngine->GetPlayer(OPPOSING_SECOND(pContext->GetActor()->GetSlot()));

    const auto flDamageB = pOpponent->m_stats.GetScalar(kRatingDamage);

    if (flDamageA <= DamageGate())
    {
        if (flDamageB <= DamageGate())
        {
            *pResult = 0;
        }
    }
}

#define PERCENT_SCALE 10.0f
constexpr auto kPercentRange = 100;

// The figure is halved before the roll, so a perfect actor is capped at half the die.
// FUNCTION: HYPERX 0x00467e80
void PlanCond::JostledDropRok(Interactor* pContext, PlanNode*, short* pnResult, int)
{
    auto* const pActor = pContext->GetPlayerActor();

    const auto flBase   = pActor->m_stats.GetWeightedStat(kRatingRokHandling) * PERCENT_SCALE;
    const auto flDamage = pActor->m_stats.GetScalar(kRatingDamage);

    const unsigned int nRoll = rand() % kPercentRange;

    const auto flOdds = (flDamage * flBase) * 0.01f;

    if (static_cast<float>(nRoll) > (flOdds + flBase) * 0.5f)
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
constexpr auto kTrackDeltaLimit = 4;

// The Multiplier rating for the team ahead or behind; a value past four either way is two.
constexpr auto kTrackRatingLimit   = 4;
constexpr auto kTrackRatingDefault = 2;

// FUNCTION: HYPERX 0x00467f10
void PlanCond::GoThroughMuliplier(Interactor* pContext, PlanNode*, short* pResult, int)
{
    PlayerObject* pOwner;
    short         nSlot;
    short         nThreshold;

    auto* const pRok = g_pGameEngine->GetRok();

    if (pRok->m_bAtSpawnSlot != 0)
    {
        *pResult = 1;

        return;
    }

    auto nDelta = pRok->m_wRokValue;

    auto* const pCarrier = pRok->GetPhysicsBody()->pAttach;

    if (pCarrier != nullptr)
    {
        nSlot = pCarrier->nSlot;

        if (nSlot < kPlayerObjectCount)
        {
            if ((nSlot & 1) != 0)
            {
                nDelta = -nDelta;
            }

            if (nDelta >= kTrackDeltaLimit)
            {
                *pResult = 1;

                return;
            }

            auto* const pActor = pContext->GetActor();

            if (GameEngine::IsTeamWinning(static_cast<unsigned char>(pActor->GetSlot())) != 0)
            {
                pOwner = pContext->GetPlayerActor();

                nThreshold = static_cast<short>((&pOwner->m_stats)->GetScalar(kRatingMultiplier0));
            }
            else
            {
                pOwner = pContext->GetPlayerActor();

                nThreshold = static_cast<short>((&pOwner->m_stats)->GetScalar(kRatingMultiplier1));
            }

            if (g_anCoachConditions[0] != 0)
            {
                nThreshold = kTrackDeltaLimit;
            }
            else if (nThreshold < -kTrackRatingLimit || nThreshold > kTrackRatingLimit)
            {
                nThreshold = kTrackRatingDefault;
            }

            *pResult = nDelta >= nThreshold;

            return;
        }
    }

    *pResult = 1;
}

constexpr auto kRollSteps = 5;

constexpr auto kOptionAttackModeOn = 1;

// The answer is one past an ID_ATK attack; nought deploys the Rok, mine or swarf the actor carries.
// FUNCTION: HYPERX 0x00468000
void PlanCond::PickAttackMove(Interactor* pContext, PlanNode*, short* pResult, int)
{
    PhysicsBody* pWeaponBody;

    Vec3  vOffset;
    int   nSide;
    short nSlot;

    auto nMove = 0;

    auto nRoll = rand() % kPercentRange;

    auto* pActor = pContext->GetPlayerActor();

    int nAttackerSlot = pActor->GetSlot();

    if (nAttackerSlot > kSlotDromer3)
    {
        nAttackerSlot = g_pGameEngine->m_nViewedSlot;

        nSlot = static_cast<short>(nAttackerSlot);

        pActor = g_pGameEngine->GetPlayer(nSlot);

        if (g_pGameEngine->m_nAttackMode == kOptionAttackModeOn)
        {
            *pResult = kAttackPunch + 1;

            return;
        }
    }

    *pResult = kAttackPunch + 1;

    auto* pWeapon = g_pGameEngine->m_ppSlots[kSlotRok];

    if (pWeapon != nullptr && ((RokObject*)pWeapon)->m_nKillState > -1)
    {
        pWeaponBody = pWeapon->GetPhysicsBody();

        if (pWeaponBody->pAttach != nullptr && pWeaponBody->pAttach->nSlot < kPlayerObjectCount && pWeaponBody->pAttach->nSlot == nAttackerSlot)
        {
            *pResult = 0;

            return;
        }
    }

    pWeapon = g_pGameEngine->m_ppSlots[kSlotMine];

    if (pWeapon != nullptr)
    {
        pWeaponBody = pWeapon->GetPhysicsBody();

        if (pWeaponBody->pAttach != nullptr && pWeaponBody->pAttach->nSlot < kPlayerObjectCount && pWeaponBody->pAttach->nSlot == nAttackerSlot)
        {
            *pResult = 0;

            return;
        }
    }

    pWeapon = g_pGameEngine->m_ppSlots[kSlotSwarf];

    if (pWeapon != nullptr)
    {
        pWeaponBody = pWeapon->GetPhysicsBody();

        if (pWeaponBody->pAttach != nullptr && pWeaponBody->pAttach->nSlot < kPlayerObjectCount && pWeaponBody->pAttach->nSlot == nAttackerSlot)
        {
            *pResult = 0;

            return;
        }
    }

    auto* const pStats = &pActor->m_stats;

    auto nWeight = static_cast<int>(pStats->GetScalar(kRatingFighting0));

    if (nRoll > nWeight)
    {
        while (nRoll > nWeight)
        {
            nMove++;
            nRoll -= nWeight;

            nWeight = static_cast<int>(pStats->GetScalar(nMove + kRatingFighting0));

            if (nMove > kRollSteps - 1)
            {
                *pResult = kAttackPunch + 1;

                return;
            }
        }
    }

    const Vec3* const pSelf = pActor->GetPositionPtr();

    auto bySlot = static_cast<unsigned char>(pActor->GetSlot());

    const Vec3* const pFar = g_pGameEngine->m_ppSlots[OPPOSING_FIRST(bySlot)]->GetPositionPtr();

    bySlot = static_cast<unsigned char>(pActor->GetSlot());

    const Vec3* const pNear = g_pGameEngine->m_ppSlots[OPPOSING_SECOND(bySlot)]->GetPositionPtr();

    const auto flFarX = pSelf->x - pFar->x;
    const auto flFarY = pSelf->y - pFar->y;
    const auto flFarZ = pSelf->z - pFar->z;

    const auto flNearX = pSelf->x - pNear->x;
    const auto flNearY = pSelf->y - pNear->y;
    vOffset.z          = pSelf->z - pNear->z;

    const auto flNearSq = flNearX * flNearX + flNearY * flNearY + vOffset.z * vOffset.z;

    const auto flFarSq = flFarX * flFarX + flFarY * flFarY + flFarZ * flFarZ;

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

    const Vec3* const pVel = &pActor->GetPhysicsBody()->velDir;

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
        *pResult = kAttackPunch + 1;

        return;
    }

    if (nMove == 1)
    {
        *pResult = kAttackHipCheck + 1;

        return;
    }

    if (nMove == 2)
    {
        *pResult = static_cast<short>(nSide + kAttackClothesLineL + 1);

        return;
    }

    if (nMove == 3)
    {
        *pResult = static_cast<short>(nSide + kAttackShoulderCheckL + 1);

        return;
    }

    *pResult = static_cast<short>(nSide + kAttackLegSweepL + 1);
}

// Both opponents hurt turn an index of one into two; zero and two stand.

constexpr auto kOffWingJobMax = 2;

// FUNCTION: HYPERX 0x004682d0
void PlanCond::OffWingJob(Interactor* pContext, PlanNode*, short* pResult, int)
{
    PlayerObject* pOwner;
    int           nState;

    if (GameEngine::IsTeamWinning(static_cast<unsigned char>(pContext->GetActor()->GetSlot())) != 0)
    {
        pOwner = pContext->GetPlayerActor();

        nState = static_cast<int>((&pOwner->m_stats)->GetScalar(kRatingWinOffWingJob));
    }
    else
    {
        pOwner = pContext->GetPlayerActor();

        nState = static_cast<int>((&pOwner->m_stats)->GetScalar(kRatingLoseOffWingJob));
    }

    if (nState < 0 || nState > kOffWingJobMax)
    {
        nState = 0;
    }

    *pResult = static_cast<short>(nState);

    auto* pOpponent = g_pGameEngine->GetPlayer(OPPOSING_FIRST(pContext->GetActor()->GetSlot()));

    const auto flDamageA = pOpponent->m_stats.GetScalar(kRatingDamage);

    pOpponent = g_pGameEngine->GetPlayer(OPPOSING_SECOND(pContext->GetActor()->GetSlot()));

    const auto flDamageB = pOpponent->m_stats.GetScalar(kRatingDamage);

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
void PlanCond::PickFlyingMove(Interactor* pContext, PlanNode* pArgs, short* pResult, int)
{
    const auto vVelocity = pContext->GetActor()->m_pBody->vel;

    const auto nRange = pArgs->m_nArgCount - 1;

    if (static_cast<float>(sqrt(static_cast<double>(Vec3::DotInline(&vVelocity, &vVelocity))) * g_pGameEngine->m_clock.flInvDelta) < 50.0f)
    {
        *pResult = static_cast<unsigned short>(rand() % (nRange / 2));
    }
    else
    {
        *pResult = static_cast<unsigned short>(rand() % nRange);
    }
}

// Picks one of the set bits of the seven-bit kRatingHalfPipeRepertoire mask, with equal weight.

// FUNCTION: HYPERX 0x004684e0
void PlanCond::PickHalfpipeMove(Interactor* pContext, PlanNode*, short* pResult, int)
{
    auto nBit   = 0;
    auto nCount = 0;

    auto* const pObject = pContext->GetPlayerActor();

    const auto nMask = static_cast<int>(pObject->m_stats.GetScalar(kRatingHalfPipeRepertoire));

    if (nMask < 1 || nMask > 0x7f)
    {
        *pResult = static_cast<short>(rand() % 7);
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

    const auto nRemainder = rand() % nCount;

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
    auto* const pActor = pContext->GetPlayerActor();

    *pResult = static_cast<int>(pActor->m_stats.GetScalar(kRatingTurnstile0)) == 0;
}

// FUNCTION: HYPERX 0x00468640
void PlanCond::DecideNever(Interactor*, PlanNode*, short* pResult, int)
{
    *pResult = 0;
}

// The window an authored range has to fall in, and what replaces it when it does not.
constexpr auto kRangeMin     = 10;
constexpr auto kRangeMax     = 350;
constexpr auto kRangeDefault = 150;

// An authored range outside ten to 350 becomes 150; a local match never shoots past a human mate.
// FUNCTION: HYPERX 0x00468650
void PlanCond::ShootSituation(Interactor* pContext, PlanNode*, short* pResult, int)
{
    EngineObject* pGoal;
    Vec3          vDiff;
    int           nRange;
    auto* const   pBody = pContext->GetPlayerActor();

    nRange = static_cast<int>((&pBody->m_stats)->GetScalar(kRatingCarryRokThrowDist));

    if (nRange < kRangeMin || nRange > kRangeMax)
    {
        nRange = kRangeDefault;
    }

    const Vec3* const pSelfAt = pContext->GetActor()->GetPositionPtr();

    const auto nOwnSlot = pContext->GetActor()->GetSlot();

    EngineObject** const apObjects = g_pGameEngine->m_ppSlots;

    if ((nOwnSlot & 1) != 0)
    {
        pGoal = apObjects[kSlotNgoal];
    }
    else
    {
        pGoal = apObjects[kSlotSgoal];
    }

    const Vec3* const pGoalAt = pGoal->GetPositionPtr();

    vDiff.x = pGoalAt->x - pSelfAt->x;
    vDiff.y = pGoalAt->y - pSelfAt->y;
    vDiff.z = pGoalAt->z - pSelfAt->z;

    const auto flDistance = vDiff.Magnitude();

    auto vAxis = pContext->GetActor()->GetPhysicsBody()->velDir;

    auto flRange = static_cast<float>(nRange);

    auto vDir = vDiff;

    if (flDistance <= flRange)
    {
        Math::Vec3Normalize(&vAxis);
        Math::Vec3Normalize(&vDir);

        if (Vec3::DotInline(&vDir, &vAxis) > 0.5f)
        {
            flRange *= 0.5f;

            const auto flRoll = (static_cast<float>(rand()) * (1.0f / static_cast<float>(RAND_MAX))) * flRange;

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

    const auto nSlot = pContext->GetActor()->GetSlot();

    auto* const pMate = g_pGameEngine->m_ppSlots[TeammateSlot(nSlot)];

    if (g_pGameEngine->m_nTeammateMode == 0)
    {
        // STRING: HYPERX 0x004bb340
        if (strcmp("Human",
                   pMate->GetPlanState()->m_pszName) == 0)
        {
            *pResult = 1;
        }
    }
}

// Every value it fetches is discarded and the answer is always zero; the calls are the point.
// FUNCTION: HYPERX 0x00468830
void PlanCond::RegroupSituation(Interactor* pContext, PlanNode*, short* pResult, int)
{
    PlayerObject* pOwner;

    auto* pActor = pContext->GetActor();

    if (GameEngine::IsTeamWinning(static_cast<unsigned char>(pActor->GetSlot())) != 0)
    {
        pOwner = pContext->GetPlayerActor();

        pOwner->m_stats.GetScalar(kRatingWinCarryRokRegroupDist);
    }
    else
    {
        pOwner = pContext->GetPlayerActor();

        pOwner->m_stats.GetScalar(kRatingLoseCarryRokRegroupDist);
    }

    pContext->GetActor()->GetPositionPtr();

    pActor = pContext->GetActor();

    if ((pActor->GetSlot() & 1) != 0)
    {
        g_pGameEngine->m_ppSlots[kSlotSgoal]->GetPositionPtr();

        *pResult = 0;

        return;
    }

    g_pGameEngine->m_ppSlots[kSlotNgoal]->GetPositionPtr();

    *pResult = 0;
}

constexpr auto kAggressionRadiusMax = 400;

// The radius is compared against a squared distance without being squared itself.
// FUNCTION: HYPERX 0x004688d0
void PlanCond::AggressionOnOthers(Interactor* pContext, PlanNode*, short* pResult, int)
{
    Vec3*         pSecond;
    int           nLimit;
    PlayerObject* pOwner;
    Vec3          vB;

    auto* pActor = pContext->GetActor();

    if (GameEngine::IsTeamWinning(static_cast<unsigned char>(pActor->GetSlot())) != 0)
    {
        pOwner = pContext->GetPlayerActor();

        nLimit = static_cast<int>((&pOwner->m_stats)->GetScalar(kRatingWinAggression));
    }
    else
    {
        pOwner = pContext->GetPlayerActor();

        nLimit = static_cast<int>((&pOwner->m_stats)->GetScalar(kRatingLoseAggression));
    }

    if (nLimit < 0 || nLimit > kAggressionRadiusMax)
    {
        nLimit = kAggressionRadiusMax;
    }

    auto* const pSelf = pContext->GetActor()->GetPositionPtr();

    pActor             = pContext->GetActor();
    auto        byTeam = static_cast<unsigned char>(pActor->GetSlot());
    auto* const pFirst = g_pGameEngine->m_ppSlots[OPPOSING_FIRST(byTeam)]->GetPositionPtr();

    pActor  = pContext->GetActor();
    byTeam  = static_cast<unsigned char>(pActor->GetSlot());
    pSecond = g_pGameEngine->m_ppSlots[OPPOSING_SECOND(byTeam)]->GetPositionPtr();

    const auto dxA = pSelf->x - pFirst->x;
    const auto dyA = pSelf->y - pFirst->y;
    const auto dzA = pSelf->z - pFirst->z;

    vB.x = pSelf->x - pSecond->x;
    vB.y = pSelf->y - pSecond->y;
    vB.z = pSelf->z - pSecond->z;

    const auto flToFirst  = dxA * dxA + dyA * dyA + dzA * dzA;
    const auto flToSecond = vB.x * vB.x + vB.y * vB.y + vB.z * vB.z;

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
        g_pGameEngine->GetCamera(kCameraHeadSlot)->m_bShotPowerShown = 1;
    }
    else
    {
        g_pGameEngine->GetCamera(kCameraHeadSlot)->m_bShotPowerShown = 0;
    }
}

// FUNCTION: HYPERX 0x00468aa0
void PlanCond::NearObjectBy(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg)
{
    const auto nSlot = static_cast<short>(nArg);

    const auto nDistance = pArgs->m_pArgs[1].pNode->m_pArgs[0].n;

    auto* const pSelf  = pContext->GetActor()->GetPositionPtr();
    auto* const pOther = g_pGameEngine->m_ppSlots[nSlot]->GetPositionPtr();

    // The distance stays squared: the plan argument is already a square.
    const auto dx = pSelf->x - pOther->x;
    const auto dy = pSelf->y - pOther->y;
    const auto dz = pSelf->z - pOther->z;

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

    if (nParam == kSlotWildcard)
    {
        *pResult = 1;

        return;
    }

    auto* const pRok = g_pGameEngine->m_ppSlots[kSlotRok];

    if (static_cast<short>(((RokObject*)pRok)->m_nCarrierSlot) != static_cast<short>(nParam))
    {
        *pResult = pRok->GetPhysicsBody()->pAttach == nullptr;

        return;
    }

    *pResult = 0;
}

// The Rok's carrier slot must name one of the four skaters; anything else answers yes.

// FUNCTION: HYPERX 0x00468b90
void PlanCond::CheckScoreOnOwnGoal(Interactor* pContext, PlanNode*, short* pResult, int)
{
    auto* const pActor = pContext->GetActor();

    EngineObject** const ppObjects = g_pGameEngine->m_ppSlots;

    auto* const pRok = (RokObject*)ppObjects[kSlotRok];

    const auto nTracked = pRok->m_nCarrierSlot;

    if (nTracked >= 0 && nTracked <= 3)
    {
        const auto nGoal = ((PlayerObject*)ppObjects[static_cast<short>(nTracked)])->m_wTargetGoalSlot;

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

    auto* const pRok = g_pGameEngine->GetRok();

    *pResult = pRok->m_nCarrierSlot % 2 != pRok->m_nPrevCarrierSlot % 2;
}

// Two out of range or facing away; in range, two when the actor is the Rok's carrier slot, else one.

// The largest range the condition will test, and the dot the actor has to beat.
#define RANGE_CEILING 150.0f
#define FACING_DOT    -0.8f

// FUNCTION: HYPERX 0x00468c40
void PlanCond::RokInRange(Interactor* pContext, PlanNode* pArgs, short* pResult, int nArg)
{
    const unsigned int nRange = nArg;

    const Vec3* pRok;
    float       flValue;
    Vec3        vOffset;

    const auto nFlag = pArgs->m_pArgs[1].pNode->m_pArgs[0].n;

    auto flRange = static_cast<float>(nRange);

    const Vec3* const pSelf = pContext->GetActor()->GetPositionPtr();

    pRok = g_pGameEngine->m_ppSlots[kSlotRok]->GetPositionPtr();

    vOffset.x = pSelf->x - pRok->x;
    vOffset.y = pSelf->y - pRok->y;
    vOffset.z = pSelf->z - pRok->z;

    if (nFlag != 0)
    {
        const auto flSide = static_cast<float>(sqrt(static_cast<double>(flRange))) + g_flOnePlan;

        flRange = flSide * flSide;
    }

    auto* const pActor = pContext->GetActor();

    const auto nSlot = pActor->GetSlot();

    if (nSlot < kPlayerSlotCount)
    {
        auto* const pOwner = pContext->GetPlayerActor();

        const auto flHandling = pOwner->m_stats.GetScalar(kRatingRokHandling);

        flValue = pOwner->m_stats.GetScalar(kRatingDamage);

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
            const auto nTracked = g_pGameEngine->GetRok()->m_nCarrierSlot;

            *pResult = static_cast<short>((nTracked == pContext->GetActor()->GetSlot()) + 1);

            return;
        }

        *pResult = 2;

        return;
    }

    *pResult = 2;
}

constexpr auto kGatedSlots = 4;

#define RANGE_START 600.0f

// Goalies at slots four and five are never damage-gated, so they stay in the running.
// FUNCTION: HYPERX 0x00468e50
void PlanCond::ClosestDromerToRok(Interactor*, PlanNode*, short* pResult, int)
{
    EngineObject* apActor[kPlayerObjectCount];
    Vec3          aOffset[kPlayerObjectCount];
    Vec3          vTo;
    float         flLimit;
    int           nSlot;

    *pResult = kGatedSlots;

    auto flBest = RANGE_START;

    EngineObject** const ppObjects = g_pGameEngine->m_ppSlots;

    apActor[0] = ppObjects[0];
    apActor[1] = ppObjects[1];
    apActor[2] = ppObjects[2];
    apActor[3] = ppObjects[3];
    apActor[4] = ppObjects[4];
    apActor[5] = ppObjects[5];

    auto* const pRok = ppObjects[kSlotRok];

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

    for (nSlot = 0; nSlot < kPlayerObjectCount; nSlot++)
    {
        if (nSlot < kGatedSlots)
        {
            flLimit = DamageGate();

            if (flLimit >= ((PlayerObject*)apActor[nSlot])->m_stats.GetScalar(kRatingDamage))
            {
                continue;
            }
        }

        const auto flZ = aOffset[nSlot].z;
        const auto flY = aOffset[nSlot].y;
        const auto flX = aOffset[nSlot].x;

        const auto flDistance = (flX * flX + flZ * flZ) + flY * flY;

        if (flDistance < flBest)
        {
            *pResult = static_cast<unsigned short>(nSlot);

            flBest = flDistance;
        }
    }
}

// FUNCTION: HYPERX 0x00469110
void PlanCond::SoundBank(Interactor*, PlanNode*, short* pResult, int)
{
    *pResult = PlanUtil::CheckSequenceStep(static_cast<unsigned int>(-1)) == 0;
}

// FUNCTION: HYPERX 0x00469130
void PlanCond::GetTeamParity(Interactor* pContext, PlanNode*, short* pResult, int)
{
    auto* const pActor = pContext->GetActor();

    *pResult = static_cast<signed char>(pActor->GetSlot() & 1);
}

// FUNCTION: HYPERX 0x00469150
void PlanCond::WinningOrLosing(Interactor* pContext, PlanNode*, short* pResult, int)
{
    auto* const pActor = pContext->GetActor();

    *pResult = static_cast<short>(GameEngine::IsTeamWinning(
                                      static_cast<unsigned char>(pActor->GetSlot())) == 0);
}

// A network session always answers two.

// FUNCTION: HYPERX 0x00469180
void PlanCond::IsDromerKnockedDown(Interactor*, PlanNode* pArgs, short* pResult, int nDefault)
{
    auto nSlot = pArgs->m_pArgs[1].pNode->m_pArgs[0].n;

    if (nSlot == PLAN_TOKEN_MESSAGE_DATA)
    {
        nSlot = nDefault;
    }

    auto* const pObject = g_pGameEngine->GetPlayer(static_cast<short>(nSlot));

    // m_stats at 0xd20 assumes a PlayerObject; RokObject puts its own field at the same offset.
    if (pObject->m_nKnockedDownState == 0 && DamageGate() < pObject->m_stats.GetScalar(kRatingDamage))
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
void PlanCond::IsDromerInjured(Interactor* pContext, PlanNode* pArgs, short* pResult, int nPrevSlot)
{
    const auto arg = pArgs->m_pArgs[1].pNode->m_pArgs[0];
    int        nSlot;
    float      flLimit;

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

    auto* const pObject = g_pGameEngine->GetPlayer(static_cast<short>(nSlot));

    flLimit = DamageGate();

    if (pObject->m_stats.GetScalar(kRatingDamage) <= flLimit)
    {
        *pResult = 1;
        return;
    }

    *pResult = 2;
}

// FUNCTION: HYPERX 0x00469310
void PlanCond::CheckRenderFlag(Interactor* pContext, PlanNode* pArgs, short* pResult, int nPrevSlot)
{
    const auto arg = pArgs->m_pArgs[1].pNode->m_pArgs[0];
    int        nSlot;

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

    if ((g_pGameEngine->m_ppSlots[static_cast<short>(nSlot)]->m_nFlags & kEngineObjectHidden) != 0)
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
