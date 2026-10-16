#include "common/compat.h"
#include "planvm/behaviorids.h"
#include "common/behobjglidebetweenfacing.h"
#include "common/behobjglidetoposition.h"
#include "engine/gameengine.h"
#include "engine/devicector.h"
#include "plannode.h"
#include "globals.h"
#include "planvm/planstate.h"
#include "planarg.h"
#include "planopcodetable.h"
#include "common/staticinit.h"
#include "anim/animset.h"
#include "engine/objectstats.h"
#include "behparam.h"
#include "common/pooledclasses.h"
#include "planvm/agendaitem.h"
#include "anim/animregistry.h"
#include "engine/objectclasses.h"
#include "engine/statids.h"
#include "damagegate.h"
#include "app/gameapp.h"
#include "engine/playerobject.h"
#include "engine/engineobject.h"
#include "engine/cameraobject.h"
#include "engine/arenastate.h"
#include "physics/collisionellipsoid.h"
#include "planvm/csteertotarget.h"
#include "engine/csetpositionrelative.h"
#include "planvm/cavoidobjects.h"
#include "engine/modelids.h"

#define ARG_COUNT_MARKER 2

#define ARG_MARKER 1

#define ARG_COUNT_POINT 3

#define ARG_X 1

#define ARG_Y 2

// The damage gate is the difficulty step times this.
#define GATE_SCALE 3.0f

#define PAIR_NONE -1.0f

// Slot zero of the behaviour-parameter table: three targets and a period, stored as its reciprocal.

#define ARG_COUNT    5
#define TARGET_COUNT 3

#define ARG_FIRST  1
#define ARG_PERIOD 4

// FUNCTION: HYPERX 0x0041c780
void BehParam::SetModsTweenNFacing(EngineObject* pActor, PlanNode* pNode, GameBehaviorObject* pBehaviour)
{
    BehObj_GlideBetweenFacing* const pTarget = static_cast<BehObj_GlideBetweenFacing*>(pBehaviour);
    short                            aSlot[TARGET_COUNT];
    short                            nValue;

    short nIndex = 0;
    aSlot[0]     = 0;
    aSlot[1]     = 0;
    aSlot[2]     = 0;

    if (pNode->m_nArgCount != ARG_COUNT)
    {
        // STRING: HYPERX 0x004bc234
        g_gameApp.Fatal("Unexpected number of args in Behavior parameterization functions.");
    }

    for (; nIndex < TARGET_COUNT; nIndex++)
    {
        if (pNode->m_pArgs[ARG_FIRST + nIndex].IsPronoun())
        {
            const unsigned int nPronoun = pNode->m_pArgs[ARG_FIRST + nIndex].wPronoun;

            nValue = g_apPlanArgFns[nPronoun](&pActor->GetPlanState()->m_pOwner, 0, 0);
        }
        else
        {
            nValue = (short)pNode->m_pArgs[ARG_FIRST + nIndex].n;
        }

        aSlot[nIndex] = nValue;

        if (nValue < 0)
        {
            return;
        }
    }

    pTarget->m_pObjectA   = g_pGameEngine->m_ppSlots[aSlot[0]];
    pTarget->m_pObjectB   = g_pGameEngine->m_ppSlots[aSlot[1]];
    pTarget->m_pAimObject = g_pGameEngine->m_ppSlots[aSlot[2]];

    const unsigned int nPeriod = pNode->m_pArgs[ARG_PERIOD].n;

    if (nPeriod != 0)
    {
        pTarget->m_flBlendStep = 1.0f / (float)nPeriod;
    }
    else
    {
        pTarget->SetBlendStep(g_flZeroBehavior);
    }
}

// The node stores the value in hundredths of whatever the engine counts in.

// FUNCTION: HYPERX 0x0041c8a0
void BehParam::SetInteractorLatency(EngineObject*, PlanNode* pNode, GameBehaviorObject* pTarget)
{
    pTarget->m_wLatency = (short)(g_pGameEngine->m_clock.flInvDelta * ((short)pNode->m_pArgs[1].n * 0.01));
}

// FUNCTION: HYPERX 0x0041c8e0
void BehParam::SetDigressive(EngineObject*, PlanNode* pNode, GameBehaviorObject* pTarget)
{
    pTarget->m_bDigressive = (char)pNode->m_pArgs[1].n;
}

// The only data-driven way to set a behaviour's purged byte for an agenda id.

// FUNCTION: HYPERX 0x0041c900
void BehParam::SetPurgeFlag(EngineObject*, PlanNode* pItem, GameBehaviorObject* pTarget)
{
    PlanWord* const pArgs = pItem->m_pArgs;

    pTarget->SetAgendaPurged(pArgs[1].n, (char)pArgs[2].n);
}

// FUNCTION: HYPERX 0x0041c920
void BehParam::SetTarget(EngineObject*, PlanNode* pNode, GameBehaviorObject* pBehaviour)
{
    static_cast<CSteerToTarget*>(pBehaviour->m_apSlots[0])->SetTargetObjectByIndex((short)pNode->m_pArgs[1].n);
}

// The receiver's copy at 0x98 is kept only when its kind is eleven, and nothing reads it.
// FUNCTION: HYPERX 0x0041c940
void BehParam::SetTargetPosition(EngineObject*, PlanNode* pNotify, GameBehaviorObject* pReceiver)
{
    if (pNotify->m_nArgCount == ARG_COUNT_MARKER)
    {
        pReceiver->m_apSlots[0]
            ->Tick(g_apPositionMarkers[pNotify->m_pArgs[ARG_MARKER].n]);

        if (pReceiver->m_apSlots[1] != 0)
        {
            pReceiver->m_apSlots[1]
                ->Tick(g_apPositionMarkers[pNotify->m_pArgs[ARG_MARKER].n]);
        }

        return;
    }

    if (pNotify->m_nArgCount != ARG_COUNT_POINT)
    {
        return;
    }

    Vec3* pVec = pReceiver->m_apSlots[0]->GetVector();

    if (pVec != 0)
    {
        pVec->x = (float)pNotify->m_pArgs[ARG_X].n;
        pVec->y = (float)pNotify->m_pArgs[ARG_Y].n;
        pVec->z = g_arenaEllipsoid.GetZ(pVec->x, pVec->y);

        pReceiver->m_apSlots[0]->Tick(pVec);
    }

    if (pReceiver->m_apSlots[1] != 0)
    {
        pVec = pReceiver->m_apSlots[1]->GetVector();

        if (pVec != 0)
        {
            pVec->x = (float)pNotify->m_pArgs[ARG_X].n;
            pVec->y = (float)pNotify->m_pArgs[ARG_Y].n;
            pVec->z = g_arenaEllipsoid.GetZ(pVec->x, pVec->y);

            pReceiver->m_apSlots[1]->Tick(pVec);
        }
    }

    if (pReceiver->m_wKind == BEH_GLIDE_TO_POSITION)
    {
        pVec = &((BehObj_GlideToPosition*)pReceiver)->m_vTarget;

        if (pVec != 0)
        {
            pVec->x = (float)pNotify->m_pArgs[ARG_X].n;
            pVec->y = (float)pNotify->m_pArgs[ARG_Y].n;
            pVec->z = g_arenaEllipsoid.GetZ(pVec->x, pVec->y);
        }
    }
}

// A plan names a spot on the rink here, not a player.

// FUNCTION: HYPERX 0x0041ca60
void BehParam::SetRelativePosition(EngineObject*, PlanNode* pNode, GameBehaviorObject* pBehaviour)
{
    static_cast<CSetPositionRelative*>(pBehaviour->m_apSlots[0])
        ->SetPositionWithTerrainHeight(g_apPositionMarkers[pNode->m_pArgs[1].n]);
}

// FUNCTION: HYPERX 0x0041ca80
void BehParam::SetModelImRelativeTo(EngineObject*, PlanNode* pNode, GameBehaviorObject* pBehaviour)
{
    static_cast<CSetPositionRelative*>(pBehaviour->m_apSlots[0])->SetSourceObjectByIndex((short)pNode->m_pArgs[1].n);
}

// FUNCTION: HYPERX 0x0041caa0
void BehParam::SetObjectToAvoid(EngineObject*, PlanNode* pNode, GameBehaviorObject* pBehaviour)
{
    static_cast<CAvoidObjects*>(pBehaviour->m_apSlots[0])->AddAvoidanceTarget((short)pNode->m_pArgs[1].n);
}

// MODEL_ID_NONE means nothing: the first two followers are told zero and the third not at all.
// FUNCTION: HYPERX 0x0041cac0
void BehParam::SetModelImFacing(EngineObject*, PlanNode* pNotify, GameBehaviorObject* pTrajectory)
{
    if (pNotify->m_pArgs[1].n != MODEL_ID_NONE)
    {
        pTrajectory->m_apSlots[0]->OnKind((short)pNotify->m_pArgs[1].n);

        pTrajectory->m_apSlots[1]->OnKind((short)pNotify->m_pArgs[1].n);

        if (pTrajectory->m_apSlots[4] != 0)
        {
            pTrajectory->m_apSlots[4]->OnKind((short)pNotify->m_pArgs[1].n);
        }

        return;
    }

    pTrajectory->m_apSlots[0]->OnKind(0);

    pTrajectory->m_apSlots[1]->OnKind(0);
}

// FUNCTION: HYPERX 0x0041cb20
void __cdecl BehParam::SetPersistentAnimation(EngineObject* pActor, PlanNode* pRef, GameBehaviorObject* pOut)
{
    AnimDB* pEntry;

    short nAnim = (short)pRef->m_pArgs[1].n;

    char bLimped = 0;

    // A hurt player swaps the standard and fast skates for their limping versions.
    if (pActor->GetSlot() < PLAYER_SLOT_COUNT)
    {
        const float flGate = DamageGate();

        if (((PlayerObject*)pActor)->m_stats.GetScalar(RATING_DAMAGE) < flGate * GATE_SCALE)
        {
            // STRING: HYPERX 0x004bb2a0
            if (nAnim == g_pAnimRegistry->FindAnimByNameIdx("AA_Skate_Std"))
            {
                // STRING: HYPERX 0x004bb290
                nAnim   = g_pAnimRegistry->FindAnimByNameIdx("AA_LimpSkate");
                bLimped = 1;
            }
        }
    }

    if (bLimped == 0 && pActor->GetSlot() < PLAYER_SLOT_COUNT)
    {
        const float flGate = DamageGate();

        if (((PlayerObject*)pActor)->m_stats.GetScalar(RATING_DAMAGE) < flGate * GATE_SCALE)
        {
            // STRING: HYPERX 0x004bc278
            if (nAnim == g_pAnimRegistry->FindAnimByNameIdx("AA_Skate_Fast"))
            {
                // STRING: HYPERX 0x004bb270
                nAnim = g_pAnimRegistry->FindAnimByNameIdx("AA_LimpSpeed");
            }
        }
    }

    const AnimScriptTable* pTable = g_pAnimRegistry->m_pScripts;

    pEntry = pTable->Find(nAnim);

    pOut->m_pPersistentAnimation = pEntry;
    pOut->m_pAnimation           = pEntry;

    if (pRef->m_nArgCount == 4)
    {
        pOut->SetPersistentPlayback(pRef->m_pArgs[2].fl, pRef->m_pArgs[3].n);
        pOut->SetAnimationPlayback(pRef->m_pArgs[2].fl, pRef->m_pArgs[3].n);

        return;
    }

    pOut->m_flPersistentRate = PAIR_NONE;
    pOut->m_nPersistentFlags = 1;
    pOut->m_flAnimationRate  = PAIR_NONE;
    pOut->m_nAnimationFlags  = 1;
}

// FUNCTION: HYPERX 0x0041cce0
void BehParam::SetAnimation(EngineObject*, PlanNode* pNode, GameBehaviorObject* pBehaviour)
{
    AnimDB* pClip;

    const short            nClip     = (short)pNode->m_pArgs[1].n;
    const AnimScriptTable* pRegistry = g_pAnimRegistry->m_pScripts;

    pClip = pRegistry->Find(nClip);

    pBehaviour->m_pAnimation = pClip;

    if (pNode->m_nArgCount == 4)
    {
        pBehaviour->SetAnimationPlayback(pNode->m_pArgs[2].fl, pNode->m_pArgs[3].n);
        return;
    }

    pBehaviour->m_flAnimationRate = -1.0f;
    pBehaviour->m_nAnimationFlags = -1;
}

// FUNCTION: HYPERX 0x0041cd40
void BehParam::SetAnimationEvent(EngineObject*, PlanNode* pNode, GameBehaviorObject* pBehaviour)
{
    int nFourth;

    if (pNode->m_nArgCount == 5)
    {
        nFourth = pNode->m_pArgs[4].n;
    }
    else
    {
        nFourth = 0;
    }

    PlanWord* const pArgs = pNode->m_pArgs;

    const int nThird           = pArgs[3].n;
    const int nSecond          = pArgs[2].n;
    pBehaviour->m_nChannelSlot = pArgs[1].n;
    pBehaviour->m_nEventParam  = nFourth;
    pBehaviour->m_nEventTarget = nSecond;
    pBehaviour->m_nEventKind   = nThird;
}

// FUNCTION: HYPERX 0x0041cd80
void BehParam::SetObjectToFollow(EngineObject*, PlanNode* pNode, GameBehaviorObject* pBehaviour)
{
    EngineObject* const pEntity = g_pGameEngine->m_ppSlots[(short)pNode->m_pArgs[1].n];
    AgendaItem* const   pTarget = pBehaviour->m_apSlots[1];

    pTarget->Tick(pEntity->GetPositionPtr());
}

// FUNCTION: HYPERX 0x0041cdb0
void BehParam::SetTargetGoal(EngineObject* pActor, PlanNode* pNode, GameBehaviorObject*)
{
    ((PlayerObject*)pActor)->m_wTargetGoalSlot = (short)pNode->m_pArgs[1].n;
}
