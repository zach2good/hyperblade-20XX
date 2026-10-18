#include "common/compat.h"
#include "planvm/planaction.h"
#include "planvm/agendaitemids.h"
#include "common/pooledclasses.h"
#include "plannode.h"
#include "engine/devicector.h"
#include "physics/physicsbody.h"
#include "planvm/agendaitem.h"
#include "planvm.h"
#include "engine/objectstats.h"
#include "engine/statids.h"
#include "globals.h"
#include "engine/objectclasses.h"
#include "anim/animregistry.h"
#include "anim/animdb.h"
#include "engine/gameengine.h"
#include "planvmglobals.h"
#include "engine/playerobject.h"
#include "collinteractor.h"
#include "msginteractor.h"
#include <cstring>
#include "engine/engineobject.h"
#include "planvm/planinteractorfactories.h"
#include "common/pooledownerroot.h"
#include "engine/messageids.h"
#include "engine/objectphase.h"

// The game object's construction, shared by twenty-four derived constructors.

// FUNCTION: HYPERX 0x0041bf50
GameBehaviorObject::GameBehaviorObject()
{
    memset(m_apSlots, 0, sizeof(m_apSlots));

    m_pBody                = nullptr;
    m_pOwner               = nullptr;
    m_pszName              = nullptr;
    m_pAnimation           = nullptr;
    m_pPersistentAnimation = nullptr;
    m_nEventTarget         = 0;
    m_nEventParam          = 0;
    m_bDigressive          = 0;
    m_pCollInteractors     = nullptr;
    m_pMsgInteractors      = nullptr;

    m_flAnimationRate  = -1.0f;
    m_nAnimationFlags  = -1;
    m_flPersistentRate = -1.0f;
    m_nPersistentFlags = -1;
    m_nEventKind       = kMsgNomsg;
    m_wPollCountdown   = -1;
    m_wLatency         = kBehaviorDefaultLatency;
    m_pStandby         = nullptr;
}

// Each table is marked where it is filled, so all three are declared bare here.
extern BehParamHandlerFn g_apfnBehaviorParamHandlers[17];
extern PlanOpcodeSlot    g_pfnPlanOpcodeHandlers[141];
extern PlanHandlerFn     g_pfnPlanActionHandlers[140];

constexpr auto kHeaderItems = 2;

// Out of line: the original calls this destructor from every derived one.
// FUNCTION: HYPERX 0x0041bfe0
GameBehaviorObject::~GameBehaviorObject() = default;

// The behaviour's setup pass over a compiled plan block; every BehObj_*::Init ends by calling it.
// FUNCTION: HYPERX 0x0041bff0
void GameBehaviorObject::Init(EngineObject* pOwner, PlanNode* pBlock, int)
{
    PlanWord*   pCursor;
    AgendaItem* pBubble;

    auto nCount = pBlock->m_nArgCount;

    m_pOwner = pOwner;

    Interactor* pCondition = nullptr;

    Interactor* pMessage = nullptr;

    m_pBody = pOwner->GetPhysicsBody();

    if (pBlock->m_nOpcode == (static_cast<int>(CATEGORY_BEHAVIOUR) | MEMBER_DECLARE_BEHAVIOR))
    {
        pCursor = pBlock->m_pArgs + kHeaderItems;

        nCount -= kHeaderItems;
    }
    else
    {
        pCursor = pBlock->m_pArgs;
    }

    while (nCount-- != 0)
    {
        auto* const pItem = pCursor->pNode;

        switch (pItem->m_nOpcode & kOpCategory)
        {
            case CATEGORY_MESSAGE_INTERACTOR:
                pMessage = PlanInteractorFactories::CreateMsgInteractor(m_pOwner, pItem, this, pMessage);

                pMessage->SetMember(pItem->m_nOpcode & kOpMember);
                break;

            case CATEGORY_COLLISION_INTERACTOR:
                pCondition = PlanInteractorFactories::CreateCollInteractor(m_pOwner, pItem, this, pCondition);

                pCondition->SetMember(pItem->m_nOpcode & kOpMember);
                break;

            case CATEGORY_BEHAVIOUR:
                if ((pItem->m_nOpcode & kOpMember) == MEMBER_SET_BEHAVIOR_PARAM)
                {
                    g_apfnBehaviorParamHandlers[static_cast<short>(pItem->m_pArgs[0].n)](m_pOwner, pItem, this);
                }
                break;

            case CATEGORY_AGENDA:
                switch (pItem->m_nOpcode)
                {
                    case static_cast<int>(CATEGORY_AGENDA) | MEMBER_ADD_AGENDA_ITEM:
                        pBubble = static_cast<AgendaItem*>(g_apAgendaItemPools[static_cast<short>(pItem->m_pArgs[0].n)]->Alloc());

                        pBubble->Bind(m_pOwner, this);

                        pBubble->Activate();
                        break;

                    case static_cast<int>(CATEGORY_AGENDA) | MEMBER_REMOVE_AGENDA_ITEM:
                        RemoveAgendaItem(pItem->m_pArgs[0].n);
                        break;
                }
                break;

            case CATEGORY_ACTION:
                switch (pItem->m_nOpcode & kOpMember)
                {
                    case MEMBER_DO_NET_ACTION:
                        if (m_pOwner->GetPhase() != OBJECT_PHASE_LOCAL)
                        {
                            g_pfnPlanActionHandlers[static_cast<short>(pItem->m_pArgs[0].n)](reinterpret_cast<Interactor*>(this), pItem, nullptr, 0);
                        }
                        break;

                    case MEMBER_DO_LOCAL_ACTION:
                        if (m_pOwner->GetPhase() != OBJECT_PHASE_NETWORK)
                        {
                            g_pfnPlanActionHandlers[static_cast<short>(pItem->m_pArgs[0].n)](reinterpret_cast<Interactor*>(this), pItem, nullptr, 0);
                        }
                        break;

                    case MEMBER_DO_ACTION:
                        g_pfnPlanActionHandlers[static_cast<short>(pItem->m_pArgs[0].n)](nullptr, pItem, nullptr, 0);
                        break;

                    case MEMBER_SET_INTERACTOR_PARAM:
                        g_pfnPlanOpcodeHandlers[static_cast<short>(pItem->m_pArgs[0].n)].pfnSetParam(m_pOwner, pItem, this, nullptr);
                        break;
                }
                break;

            case CATEGORY_DEBUG:
                break;

            default:
                // STRING: HYPERX 0x004bc1d0
                throw "Invalid Instruction object in Behavior setup.";
        }

        pCursor++;
    }
}

// The limp swap: the standard skate becomes the limp skate and the fast skate the limp speed.
// FUNCTION: HYPERX 0x0041c220
void GameBehaviorObject::UpdateLimpAnim()
{
    if (m_pPersistentAnimation == nullptr)
    {
        return;
    }

    if (m_pOwner->GetSlot() <= kPlayerSlotCount - 1 && ((PlayerObject*)m_pOwner)->m_stats.GetScalar(kRatingDamage) < 30.0f)
    {
        const auto* const pszFirst = m_pPersistentAnimation->GetName();

        // STRING: HYPERX 0x004bb2a0
        if (strcmp(pszFirst, "AA_Skate_Std") == 0)
        {
            m_pPersistentAnimation = (g_pAnimRegistry)
                                         // STRING: HYPERX 0x004bb290
                                         ->FindAnimByNamePtr("AA_LimpSkate");
        }
        else
        {
            const auto* const pszSecond = m_pPersistentAnimation->GetName();

            // STRING: HYPERX 0x004bb280
            if (strcmp(pszSecond, "AA_Skatefast") == 0)
            {
                m_pPersistentAnimation = (g_pAnimRegistry)
                                             // STRING: HYPERX 0x004bb270
                                             ->FindAnimByNamePtr("AA_LimpSpeed");
            }
        }
    }

    m_pAnimation = m_pPersistentAnimation;

    m_pOwner->StartClipPlayback(m_pPersistentAnimation, m_flPersistentRate, m_nPersistentFlags);
}

// Both heads are read before either walk, so an interactor added during the first is not set.

// FUNCTION: HYPERX 0x0041c2c0
void GameBehaviorObject::SetInteractorsActive(int nArg)
{
    Interactor* pFirst  = m_pCollInteractors;
    Interactor* pSecond = m_pMsgInteractors;

    while (pFirst != nullptr)
    {
        if (pFirst->GetActiveState() != -1)
        {
            pFirst->SetActiveState(nArg);
        }

        pFirst = pFirst->pNext;
    }

    while (pSecond != nullptr)
    {
        if (pSecond->GetActiveState() != -1)
        {
            pSecond->SetActiveState(nArg);
        }

        pSecond = pSecond->pNext;
    }
}

// Sends the node's timed message as kMsgInitiate and starts its next wait.

// FUNCTION: HYPERX 0x0041c320
void GameBehaviorObject::SendTimedMessage()
{
    m_pOwner->PostEvent(kMsgInitiate, 0, m_pOwner->GetSlot());

    m_wPollCountdown = m_wLatency;

    if (m_pAnimation != nullptr)
    {
        m_pOwner->StartClipPlayback(m_pAnimation,
                                    m_flAnimationRate,
                                    m_nAnimationFlags);
    }
}

// The counter decrements every frame, so a fired poll wraps to -1 and keeps counting.

// FUNCTION: HYPERX 0x0041c360
void GameBehaviorObject::UpdateBehaviorPollCountdown()
{
    if (m_wPollCountdown-- == 0)
    {
        SetInteractorsActive(1);
    }
}

// Run as the behaviour is activated: purge, detach, merge the slots, then re-register the message interactors.

// FUNCTION: HYPERX 0x0041c380
void GameBehaviorObject::TickAndSortMessages()
{
    AgendaItem* pNext;

    short nSlot = 0;

    auto* pNode = m_pOwner->GetObserver();

    AgendaItem* pPrev = nullptr;

    while (pNode != nullptr)
    {
        pNext = pNode->m_pNext;

        if (m_abAgendaPurged[pNode->m_wKind] != 0)
        {
            if (pPrev != nullptr)
            {
                pPrev->m_pNext = pNext;
            }
            else
            {
                m_pOwner->SetObserver(pNext);
            }

            pNode->SetNext(nullptr);

            if (pNode->m_bPersistent == 0)
            {
                pNode->OnListChanged();

                pNode = pNext;
                continue;
            }
        }

        if ((pNode->GetReadyFlag() != 0 || pNode->m_bPersistent == 0) && m_bDigressive == 0)
        {
            pNode->Detach();
        }

        pNode = pNext;

        if (pPrev != nullptr)
        {
            if (pPrev->m_pNext != pNext)
            {
                pPrev = pPrev->m_pNext;
            }
        }
        else if (m_pOwner->GetObserver() != pNext)
        {
            pPrev = m_pOwner->GetObserver();
        }
    }

    pNode = m_pOwner->GetObserver();
    pPrev = nullptr;

    while (m_apSlots[nSlot] != nullptr)
    {
        while (pNode != nullptr)
        {
            AgendaItem** const ppSlot = &m_apSlots[nSlot];

            if (*ppSlot == nullptr)
            {
                break;
            }

            if ((*ppSlot)->m_wKind > pNode->m_wKind)
            {
                pPrev = pNode;
                pNode = pNode->m_pNext;
                continue;
            }

            if ((*ppSlot)->m_wKind == pNode->m_wKind)
            {
                (*ppSlot)->m_pNext = pNode->m_pNext;
                pNode->m_pNext     = nullptr;
            }
            else
            {
                (*ppSlot)->m_pNext = pNode;
            }

            if (pPrev != nullptr)
            {
                pPrev->m_pNext = *ppSlot;
            }
            else
            {
                m_pOwner->SetObserver(*ppSlot);
            }

            nSlot++;
            pPrev = *ppSlot;
            pNode = pPrev->m_pNext;
        }

        if (pNode == nullptr)
        {
            if (pPrev != nullptr)
            {
                pPrev->m_pNext = m_apSlots[nSlot];
            }
            else
            {
                m_pOwner->SetObserver(m_apSlots[nSlot]);
            }

            pPrev = m_apSlots[nSlot];
            nSlot++;
        }
    }

    m_pOwner->ResetSceneMessageHandlers();

    Interactor* pMessage = m_pOwner->GetPlanState()->m_pMsgInteractors;

    while (pMessage != nullptr)
    {
        if (pMessage->GetState() == 0 || (pMessage->GetState() == 1 && pMessage->GetActor()->GetPhase() == OBJECT_PHASE_LOCAL) || (pMessage->GetState() == 2 && pMessage->GetActor()->GetPhase() == OBJECT_PHASE_NETWORK))
        {
            m_pOwner->RegisterSceneMessageHandler(pMessage);
        }

        pMessage = pMessage->pNext;
    }

    pMessage = m_pMsgInteractors;

    while (pMessage != nullptr)
    {
        if (pMessage->GetState() == 0 || (pMessage->GetState() == 1 && pMessage->GetActor()->GetPhase() == OBJECT_PHASE_LOCAL) || (pMessage->GetState() == 2 && pMessage->GetActor()->GetPhase() == OBJECT_PHASE_NETWORK))
        {
            m_pOwner->RegisterSceneMessageHandler(pMessage);
        }

        pMessage = pMessage->pNext;
    }
}

// FUNCTION: HYPERX 0x0041c5d0
void GameBehaviorObject::AssignPriorities(int nStart)
{
    Interactor* pAt = m_pCollInteractors;

    if (pAt == nullptr)
    {
        return;
    }

    auto nPriority = nStart;

    while (pAt != nullptr)
    {
        const auto nValue = static_cast<short>(nPriority);

        nPriority++;

        pAt->m_wEventKind = nValue;

        pAt = pAt->pNext;
    }
}

// The walk reads the next link first, because Detach can free the node it stands on.
// FUNCTION: HYPERX 0x0041c5f0
void GameBehaviorObject::RemoveAgendaItem(int nAgendaItem)
{
    auto* pNode = m_pOwner->GetObserver();

    AgendaItem* pPrev = nullptr;

    while (pNode != nullptr)
    {
        auto* const pNext = pNode->m_pNext;

        if (pNode->m_wKind == nAgendaItem)
        {
            if (pNode->m_bPersistent != 0)
            {
                pNode->m_pNext = nullptr;

                if (pPrev != nullptr)
                {
                    pPrev->m_pNext = pNext;
                }
                else
                {
                    m_pOwner->SetObserver(pNext);
                }
            }
            else
            {
                pNode->Detach();
            }
        }

        pPrev = pNode;

        pNode = pNext;
    }
}

// FUNCTION: HYPERX 0x0041c6c0
void GameBehaviorObject::Reset()
{
    Interactor* pNext;

    Interactor* pNode = m_pCollInteractors;

    if (m_pStandby != nullptr)
    {
        m_pStandby->Reset();
    }

    while (pNode != nullptr)
    {
        pNext = pNode->pNext;
        pNode->Discard();
        pNode = pNext;
    }

    pNode = m_pMsgInteractors;

    m_pCollInteractors = nullptr;

    while (pNode != nullptr)
    {
        pNext = pNode->pNext;
        pNode->Discard();
        pNode = pNext;
    }

    m_pMsgInteractors = nullptr;

    AgendaItem** ppSlot = &m_apSlots[0];
    auto         nLeft  = kBehaviorSlotCount;

    do
    {
        if (*ppSlot != nullptr)
        {
            (*ppSlot)->OnListChanged();

            *ppSlot = nullptr;
        }

        ppSlot++;
        nLeft--;
    } while (nLeft != 0);

    m_nAnimationFlags      = -1;
    m_nPersistentFlags     = -1;
    m_wPollCountdown       = -1;
    m_pBody                = nullptr;
    m_pOwner               = nullptr;
    m_pAnimation           = nullptr;
    m_pStandby             = nullptr;
    m_pPersistentAnimation = nullptr;
    m_flAnimationRate      = -1.0f;
    m_flPersistentRate     = -1.0f;
    m_nEventTarget         = 0;
    m_nEventKind           = kMsgNomsg;
    m_nEventParam          = 0;
    m_nChannelSlot         = 0;
    m_bDigressive          = 0;
    m_wLatency             = kBehaviorDefaultLatency;

    g_apBehaviorRegistry[m_wKind]->Insert(this);
}

// FUNCTION: HYPERX 0x0041d1b0
void GameBehaviorObject::PurgeAgendaForGlideToObject()
{
    memset(m_abAgendaPurged, 0, sizeof(m_abAgendaPurged));

    m_abAgendaPurged[Agenda_CProjectAndBounce]   = 1;
    m_abAgendaPurged[Agenda_CSteerToPosition]    = 1;
    m_abAgendaPurged[Agenda_CSteerToward]        = 1;
    m_abAgendaPurged[Agenda_CSteerBetweenFacing] = 1;
    m_abAgendaPurged[Agenda_CSlideOnRail]        = 1;
}

// FUNCTION: HYPERX 0x0041d1e0
void GameBehaviorObject::PurgeAgendaForGlideToPosition()
{
    memset(m_abAgendaPurged, 0, sizeof(m_abAgendaPurged));

    m_abAgendaPurged[Agenda_CSteerToTarget]      = 1;
    m_abAgendaPurged[Agenda_CProjectAndBounce]   = 1;
    m_abAgendaPurged[Agenda_CSteerToward]        = 1;
    m_abAgendaPurged[Agenda_CSteerBetweenFacing] = 1;
    m_abAgendaPurged[Agenda_CSlideOnRail]        = 1;
}

// FUNCTION: HYPERX 0x0041d210
void GameBehaviorObject::PurgeAgendaForGlideBetweenFacing()
{
    memset(m_abAgendaPurged, 0, sizeof(m_abAgendaPurged));

    m_abAgendaPurged[Agenda_CProjectAndBounce] = 1;
    m_abAgendaPurged[Agenda_CSteerToTarget]    = 1;
    m_abAgendaPurged[Agenda_CSteerToward]      = 1;
    m_abAgendaPurged[Agenda_CSlideOnRail]      = 1;
}

// FUNCTION: HYPERX 0x0041d240
void GameBehaviorObject::PurgeAgendaForGlide()
{
    memset(m_abAgendaPurged, 0, sizeof(m_abAgendaPurged));

    m_abAgendaPurged[Agenda_CBeAttached]         = 1;
    m_abAgendaPurged[Agenda_CProjectAndBounce]   = 1;
    m_abAgendaPurged[Agenda_CSteerToPosition]    = 1;
    m_abAgendaPurged[Agenda_CSteerToTarget]      = 1;
    m_abAgendaPurged[Agenda_CAdjustAcceleration] = 1;
    m_abAgendaPurged[Agenda_CSteerToward]        = 1;
    m_abAgendaPurged[Agenda_CSteerBetweenFacing] = 1;
    m_abAgendaPurged[Agenda_CSlideOnRail]        = 1;
}

// FUNCTION: HYPERX 0x0041d280
void GameBehaviorObject::PurgeAgendaForRailSlide()
{
    memset(m_abAgendaPurged, 0, sizeof(m_abAgendaPurged));

    m_abAgendaPurged[Agenda_CBeAttached]         = 1;
    m_abAgendaPurged[Agenda_CProjectAndBounce]   = 1;
    m_abAgendaPurged[Agenda_CSteerToTarget]      = 1;
    m_abAgendaPurged[Agenda_CAdjustAcceleration] = 1;
    m_abAgendaPurged[Agenda_CGlideOnSurface]     = 1;
    m_abAgendaPurged[Agenda_CSteerBetweenFacing] = 1;
}

// FUNCTION: HYPERX 0x0041d2c0
void GameBehaviorObject::PurgeAgendaForFly()
{
    memset(m_abAgendaPurged, 0, sizeof(m_abAgendaPurged));

    m_abAgendaPurged[Agenda_CBeAttached]         = 1;
    m_abAgendaPurged[Agenda_CGlideOnSurface]     = 1;
    m_abAgendaPurged[Agenda_CSetAttitude]        = 1;
    m_abAgendaPurged[Agenda_CSteerToTarget]      = 1;
    m_abAgendaPurged[Agenda_CSteerBetweenFacing] = 1;
    m_abAgendaPurged[Agenda_CAdjustAcceleration] = 1;
    m_abAgendaPurged[Agenda_CSlideOnRail]        = 1;
    m_abAgendaPurged[Agenda_CSpeedBurst]         = 1;
}

// FUNCTION: HYPERX 0x0041d300
void GameBehaviorObject::PurgeAgendaForUpdateWhileAttached()
{
    memset(m_abAgendaPurged, 0, sizeof(m_abAgendaPurged));

    m_abAgendaPurged[Agenda_CGlideOnSurface]     = 1;
    m_abAgendaPurged[Agenda_CStepAnimation]      = 1;
    m_abAgendaPurged[Agenda_CSetAttitude]        = 1;
    m_abAgendaPurged[Agenda_CProjectAndBounce]   = 1;
    m_abAgendaPurged[Agenda_CSteerToTarget]      = 1;
    m_abAgendaPurged[Agenda_CSteerToward]        = 1;
    m_abAgendaPurged[Agenda_CSteerBetweenFacing] = 1;
    m_abAgendaPurged[Agenda_CAdjustAcceleration] = 1;
}

// FUNCTION: HYPERX 0x0041d340
void GameBehaviorObject::PurgeAgendaForSetMovingInPlace()
{
    memset(m_abAgendaPurged, 0, sizeof(m_abAgendaPurged));

    m_abAgendaPurged[Agenda_CBeAttached]          = 1;
    m_abAgendaPurged[Agenda_CSteerToTarget]       = 1;
    m_abAgendaPurged[Agenda_CSetPositionRelative] = 1;
    m_abAgendaPurged[Agenda_CSteerToPosition]     = 1;
    m_abAgendaPurged[Agenda_CSteerToward]         = 1;
    m_abAgendaPurged[Agenda_CAdjustAcceleration]  = 1;
    m_abAgendaPurged[Agenda_CAvoidObjects]        = 1;
    m_abAgendaPurged[Agenda_CGlideOnSurface]      = 1;
    m_abAgendaPurged[Agenda_CSlideOnRail]         = 1;
    m_abAgendaPurged[Agenda_CSteerBetweenFacing]  = 1;
    m_abAgendaPurged[Agenda_CProjectAndBounce]    = 1;
}

// FUNCTION: HYPERX 0x0041d390
void GameBehaviorObject::PurgeAgendaForTracking()
{
    memset(m_abAgendaPurged, 0, sizeof(m_abAgendaPurged));

    m_abAgendaPurged[Agenda_CTrackPasser_Receiver] = 1;
    m_abAgendaPurged[Agenda_CTrackTraj]            = 1;
    m_abAgendaPurged[Agenda_CWatch]                = 1;
    m_abAgendaPurged[Agenda_CCircleTrack]          = 1;
}

// FUNCTION: HYPERX 0x0041d3c0
void GameBehaviorObject::PurgeAgendaForWatching()
{
    memset(m_abAgendaPurged, 0, sizeof(m_abAgendaPurged));

    m_abAgendaPurged[Agenda_CTrackPasser_Receiver] = 1;
    m_abAgendaPurged[Agenda_CTrackTraj]            = 1;
    m_abAgendaPurged[Agenda_CTrack]                = 1;
    m_abAgendaPurged[Agenda_CCircleTrack]          = 1;
}

// FUNCTION: HYPERX 0x0041d3f0
void GameBehaviorObject::PurgeAgendaForTrackTrajectory()
{
    memset(m_abAgendaPurged, 0, sizeof(m_abAgendaPurged));

    m_abAgendaPurged[Agenda_CTrackPasser_Receiver] = 1;
    m_abAgendaPurged[Agenda_CTrack]                = 1;
    m_abAgendaPurged[Agenda_CWatch]                = 1;
    m_abAgendaPurged[Agenda_CCircleTrack]          = 1;
}

// FUNCTION: HYPERX 0x0041d420
void GameBehaviorObject::PurgeAgendaForTrackPassPrep()
{
    memset(m_abAgendaPurged, 0, sizeof(m_abAgendaPurged));

    m_abAgendaPurged[Agenda_CTrack]           = 1;
    m_abAgendaPurged[Agenda_CSteerToPosition] = 1;
    m_abAgendaPurged[Agenda_CTrackTraj]       = 1;
    m_abAgendaPurged[Agenda_CWatch]           = 1;
    m_abAgendaPurged[Agenda_CCircleTrack]     = 1;
}

// FUNCTION: HYPERX 0x0041eb30
void GameBehaviorObject::PurgeAgendaForCandyTracking()
{
    memset(m_abAgendaPurged, 0, sizeof(m_abAgendaPurged));

    m_abAgendaPurged[Agenda_CTrackPasser_Receiver] = 1;
    m_abAgendaPurged[Agenda_CTrackTraj]            = 1;
    m_abAgendaPurged[Agenda_CWatch]                = 1;
    m_abAgendaPurged[Agenda_CTrack]                = 1;
    m_abAgendaPurged[Agenda_CHeadTrack]            = 1;
}
