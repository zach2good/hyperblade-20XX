#include "common/compat.h"
#include "engine/messageids.h"
#include "planvm/ctracktraj.h"
#include "planvm/csteertotarget.h"
#include "planvm/csteertoward.h"
#include "engine/devicector.h"
#include "anim/animbinding.h"
#include "anim/animset.h"
#include "engine/objectclasses.h"
#include "engine/weaponobject.h"
#include "plannode.h"
#include "globals.h"
#include "planvm.h"
#include "common/pooledclasses.h"
#include "anim/animdb.h"
#include "anim/animregistry.h"
#include "engine/gameengine.h"
#include "engine/cameraobject.h"
#include "physics/physicsbody.h"
#include "math/vector.h"
#include "planvmglobals.h"
#include "ui/hudglobals.h"
#include "engine/particle.h"
#include "planvm/agendaitem.h"
#include "planvm/cbungeetoward.h"
#include "engine/cspeedburst.h"
#include "engine/csetpositionrelative.h"
#include "planvm/cpaletteeffect.h"
#include "graphics/meshbone.h"
#include "math/transformobject.h"
#include "physics/upaxis.h"
#include "engine/objectstats.h"
#include "damagegate.h"
#include "physics/boundbubble.h"
#include "graphics/meshboundingbox.h"
#include "engine/teamblock.h"
#include "common/stub.h"
#include "planarg.h"
#include "engine/statids.h"
#include "engine/messagequeue.h"
#include "physics/collisionellipsoid.h"
#include "graphics/colorpalette.h"
#include "graphics/meshmaterial.h"
#include "graphics/modeldatabase.h"
#include "engine/rampobject.h"
#include "axisframe.h"
#include "graphics/renderer.h"
#include "ui/infobarbase.h"
#include "input/keyboarddeviceobj.h"
#include "engine/playerobject.h"
#include "anim/animplayer.h"
#include "app/gameapp.h"
#include "engine/rokobject.h"
#include "ui/infobar.h"
#include "physics/collisionobj.h"
#include <cmath>
#include <cstdlib>
#include "engine/particlesystem.h"
#include "physics/collisionplane.h"
#include "engine/engineobject.h"
#include "physics/collisioncheck.h"
#include "engine/object.h"
#include "planvm/planaction.h"
#include "planvm/plancond.h"
#include <cstring>

constexpr auto kArgCountWithRate = 4;

#define ARG_ANIM 1
constexpr auto kArgRate = 2;
#define ARG_FLAGS 3

// Minus one as a float, pushed when the block carries no rate.
#define RATE_NONE -1.0f

// Applies the same limp swap as GameBehaviorObject::UpdateLimpAnim before handing the animation over.
// FUNCTION: HYPERX 0x00401000
void PlanAction::SetAnimation(Interactor* pContext, PlanNode* pArgs, short*, int nParam)
{
    EngineObject* pActor;
    // A second name for the actor, which the original's frame keeps.
    EngineObject*       pOwner;
    GameBehaviorObject* pBehavior;
    AnimDB*             pAnim;

    const unsigned int nBehavior = pContext->m_nBehaviour;

    if (static_cast<int>(nBehavior) == PLAN_TOKEN_NO_BEH_CHANGE)
    {
        pActor = pContext->GetActor();

        pBehavior = pActor->GetPlanState()->m_pCurrent;
    }
    else
    {
        pActor = pContext->GetActor();

        pBehavior = pActor->GetPlanState()->GetBehaviorByIndex(nBehavior);
    }

    auto nIndex = pArgs->m_pArgs[ARG_ANIM].n;

    if (nIndex == PLAN_TOKEN_MESSAGE_DATA)
    {
        nIndex = nParam;
    }

    const AnimScriptTable* const pDb = g_pAnimRegistry->m_pScripts;

    pAnim = pDb->Find(static_cast<short>(nIndex));

    if (pAnim != nullptr && pContext->GetActor()->GetSlot() <= kPlayerSlotCount - 1 && ((PlayerObject*)(pOwner = pContext->GetActor()))->m_stats.GetScalar(kRatingDamage) < 30.0f)
    {
        const auto* const pszName = pAnim->GetName();

        // STRING: HYPERX 0x004bb2a0
        if (strcmp(pszName, "AA_Skate_Std") == 0)
        {
            pAnim = (g_pAnimRegistry)
                        // STRING: HYPERX 0x004bb290
                        ->FindAnimByNamePtr("AA_LimpSkate");
        }
        else if (strcmp(pszName,
                        // STRING: HYPERX 0x004bb280
                        "AA_Skatefast") == 0)
        {
            pAnim = (g_pAnimRegistry)
                        // STRING: HYPERX 0x004bb270
                        ->FindAnimByNamePtr("AA_LimpSpeed");
        }
    }

    if (pArgs->m_nArgCount == kArgCountWithRate)
    {
        auto* const pBlock = pArgs->m_pArgs;

        const auto flRate = pBlock[kArgRate].fl;
        const auto nFlags = pBlock[ARG_FLAGS].n;

        pBehavior->m_pAnimation      = pAnim;
        pBehavior->m_flAnimationRate = flRate;
        pBehavior->m_nAnimationFlags = nFlags;

        pContext->GetActor()->StartClipPlayback(pAnim, flRate, nFlags);

        return;
    }

    pContext->GetActor()->StartClipPlayback(pAnim, RATE_NONE, -1);
}

// FUNCTION: HYPERX 0x00401140
void PlanAction::RestorePersistentAnimation(Interactor* pContext, PlanNode*, short*, int)
{
    const unsigned int  nBehavior = pContext->m_nBehaviour;
    GameBehaviorObject* pBehaviour;

    if (static_cast<int>(nBehavior) == PLAN_TOKEN_NO_BEH_CHANGE)
    {
        pBehaviour = pContext->GetActor()->GetPlanState()->m_pCurrent;
    }
    else
    {
        pBehaviour = pContext->GetActor()->GetPlanState()->GetBehaviorByIndex(nBehavior);
    }

    pBehaviour->UpdateLimpAnim();
}

constexpr auto kArgEventChannel   = 1;
constexpr auto kArgEventTarget    = 2;
constexpr auto kArgEventKind      = 3;
constexpr auto kArgEventParam     = 4;
constexpr auto kArgCountWithParam = 5;

// Without a behaviour the four values go to the actor's own event channel.
// FUNCTION: HYPERX 0x00401190
void PlanAction::SetAnimationEvent(Interactor* pContext, PlanNode* pArgs, short*, int nParam)
{
    EngineObject*       pActor;
    GameBehaviorObject* pBehavior;
    PlanWord*           pBlock;
    int                 nEventParam;
    int                 nEventTarget;

    const unsigned int nBehavior = pContext->m_nBehaviour;

    if (static_cast<int>(nBehavior) != PLAN_TOKEN_NO_BEH_CHANGE)
    {
        pActor = pContext->GetActor();

        pBehavior = pActor->GetPlanState()->GetBehaviorByIndex(nBehavior);

        if (pArgs->m_nArgCount != kArgCountWithParam || pArgs->m_pArgs[kArgEventParam].n == PLAN_TOKEN_MESSAGE_DATA)
        {
            nEventParam = nParam;
        }
        else
        {
            nEventParam = pArgs->m_pArgs[kArgEventParam].n;
        }

        pBlock = pArgs->m_pArgs;

        nEventTarget = nParam;

        if (pBlock[kArgEventTarget].n != PLAN_TOKEN_MESSAGE_DATA)
        {
            nEventTarget = pBlock[kArgEventTarget].n;
        }

        const auto nEventKind = pBlock[kArgEventKind].n;

        pBehavior->m_nEventParam  = nEventParam;
        pBehavior->m_nEventTarget = nEventTarget;
        pBehavior->m_nChannelSlot = pBlock[kArgEventChannel].n;
        pBehavior->m_nEventKind   = nEventKind;
        return;
    }

    if (pArgs->m_nArgCount != kArgCountWithParam || (nEventParam = pArgs->m_pArgs[kArgEventParam].n) == PLAN_TOKEN_MESSAGE_DATA)
    {
        nEventParam = nParam;
    }

    nEventTarget = nParam;

    if (pArgs->m_pArgs[kArgEventTarget].n != PLAN_TOKEN_MESSAGE_DATA)
    {
        nEventTarget = pArgs->m_pArgs[kArgEventTarget].n;
    }

    pActor = pContext->GetActor();

    pBlock = pArgs->m_pArgs;

    pActor->BindAnimEventChannel(pBlock[kArgEventChannel].n, nEventTarget, pBlock[kArgEventKind].n, nEventParam);
}

// FUNCTION: HYPERX 0x00401250
void PlanAction::SetAnimationState(Interactor* pContext, PlanNode* pArgs, short*, int)
{
    auto* const pActor = pContext->GetActor();

    pActor->m_pAnimPlayer->SetPlayState(pArgs->m_pArgs[1].n);
}

// The speed is in hundredths and converts as unsigned, so a negative one becomes very large.

// FUNCTION: HYPERX 0x00401270
void PlanAction::SetAnimationSpeed(Interactor* pContext, PlanNode* pNode, short*, int)
{
    auto* const pActor = pContext->GetActor();

    const auto flSpeed = static_cast<float>(static_cast<unsigned int>(pNode->m_pArgs[1].n) * 0.01);

    pActor->m_pAnimPlayer->SetPlaybackRate(flSpeed);
}

// FUNCTION: HYPERX 0x004012b0
void PlanAction::IncrAnimationSpeed(Interactor* pContext, PlanNode* pNode, short*, int)
{
    auto* const pRate   = pContext->GetActor()->m_pAnimPlayer;
    auto* const pPlayer = pContext->GetActor()->m_pAnimPlayer;

    pPlayer->SetPlaybackRate(
        static_cast<float>(static_cast<unsigned int>(pNode->m_pArgs[1].n) * 0.01) * pRate->GetPlaybackRate());
}

// FUNCTION: HYPERX 0x00401310
void PlanAction::SetAnimationDoneMessage(Interactor* pContext, PlanNode* pArgs, short*, int nParam)
{
    PlanWord*     pBlock;
    EngineObject* pActor;
    EngineObject* pKeyedOn;
    AnimPlayer*   pPlayer;
    int           nMessageParam;
    int           nTargetSlot;

    switch (pArgs->m_nArgCount)
    {
        case 3:
            pActor = pContext->GetActor();
            pBlock = pArgs->m_pArgs;

            pActor->m_pAnimPlayer->SetCallbackTarget(pBlock[1].n,
                                                     pBlock[2].n,
                                                     0);
            break;

        case 2:
            pKeyedOn = pContext->GetActor();
            pActor   = pContext->GetActor();

            pPlayer = pActor->m_pAnimPlayer;

            pPlayer->SetCallbackTarget(pKeyedOn->GetSlot(), pArgs->m_pArgs[1].n, 0);
            break;

        case 4:
            pBlock = pArgs->m_pArgs;

            if (pBlock[3].n == PLAN_TOKEN_MESSAGE_DATA)
            {
                nMessageParam = nParam;
            }
            else
            {
                nMessageParam = pBlock[3].n;
            }

            if (pBlock[1].n != PLAN_TOKEN_MESSAGE_DATA)
            {
                nTargetSlot = pBlock[1].n;
            }
            else
            {
                nTargetSlot = nParam;
            }

            pActor = pContext->GetActor();

            pActor->m_pAnimPlayer->SetCallbackTarget(nTargetSlot,
                                                     pArgs->m_pArgs[2].n,
                                                     nMessageParam);
            break;
    }
}

// The Rok's carrier slots hold its carrier history; slot parity is the team.

// FUNCTION: HYPERX 0x00401460
void PlanAction::Set_AttachPak(Interactor* pContext, PlanNode* pArgs, short*, int nSlot)
{
    int nOld;
    int nNew;

    if (nSlot == kSlotWildcard)
    {
        nSlot = g_pGameEngine->m_nViewedSlot;
    }

    auto* const pActor = pContext->GetActor();

    auto nIndex = pContext->GetOtherSlot();

    if (pArgs->m_nArgCount == 2)
    {
        nIndex = static_cast<short>(pArgs->m_pArgs[1].n);
    }
    else if (nIndex < 0)
    {
        nIndex = static_cast<short>(AnimSet::GetFieldOffset(static_cast<short>(nSlot)));
    }

    if (pActor->GetSlot() == kSlotRok)
    {
        PhysicsBody* pBody;
        AttachPak*   pTracked;

        pBody = pContext->GetActor()->GetPhysicsBody();

        pTracked = pBody->pAttach;

        if (pTracked != nullptr)
        {
            g_pGameEngine->m_ppSlots[pTracked->nSlot]->PostSceneEvent(kMsgLostIt, 0);
        }
    }

    auto* const pBubble = g_pGameEngine->GetColObjPtr(nIndex);

    auto* const pAttach = pBubble->m_pAttach;

    pAttach->nIndex = nIndex;

    pActor->GetPhysicsBody()->pAttach = pAttach;

    if (pActor->GetSlot() == kSlotRok)
    {
        g_pGameEngine->m_ppSlots[pAttach->nSlot]->PostSceneEvent(kMsgGotIt, 0);

        auto* const pRok = (RokObject*)pActor;

        nOld = pRok->m_nCarrierSlot;

        pRok->m_nPrevCarrierSlot = nOld;

        if ((nOld & 1) != 0)
        {
            pRok->m_nOddPrevCarrierSlot = nOld;
        }

        nNew = pAttach->nSlot;

        pRok->m_nCarrierSlot = nNew;

        if ((nNew & 1) != 0)
        {
            pRok->m_nOddCarrierSlot = nNew;
        }

        g_pGameEngine->m_ppSlots[kSlotSoundMan]->PostSceneEvent(kMsgAttach, 0);
    }

    // The mine is a WeaponObject; it keeps the Rok's two words but not its parity pair.
    if (pActor->GetSlot() == kSlotMine)
    {
        auto* const pWeapon = (WeaponObject*)pActor;

        nOld = pWeapon->m_nAttachedSlot;

        pWeapon->m_nPrevAttachedSlot = nOld;

        nNew = pAttach->nSlot;

        pWeapon->m_nAttachedSlot = nNew;

        pActor->SetEntitySlot(nNew);
    }
}

// FUNCTION: HYPERX 0x004015c0
void PlanAction::Unset_AttachPak(Interactor* pContext, PlanNode*, short*, int)
{
    auto* const pActor = pContext->GetActor();
    auto* const pBlock = pActor->GetPhysicsBody();

    pBlock->pAttach = nullptr;
}
