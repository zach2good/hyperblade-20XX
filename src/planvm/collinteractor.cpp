#include "common/compat.h"
#include "collinteractor.h"
#include "plannode.h"
#include "engine/devicector.h"
#include "planstate.h"
#include "engine/gameengine.h"
#include "planvm.h"
#include "common/pooledclasses.h"
#include "engine/msgevent.h"
#include "common/pooledownerroot.h"
#include "engine/objectphase.h"
#include "engine/messageids.h"
#include "engine/engineobject.h"
#include "planvm/decisionlog.h"

#define ARG_KEY 0
constexpr auto kArgTag            = 1;
constexpr auto kArgConditionFirst = 2;

// Its marker is in src/planvm/planvm.cpp.
extern PlanOpcodeSlot g_pfnPlanOpcodeHandlers[141];

// FUNCTION: HYPERX 0x0045dc00
void CollInteractor::Discard()
{
    m_nActiveState = 0;
    m_bEnabled     = 1;
    m_pNode        = nullptr;
    pNext          = nullptr;

    g_pCollInteractorPool->Insert(this);
}

// FUNCTION: HYPERX 0x0045dc40
short CollInteractor::GetOtherSlot()
{
    if (m_otherSlot.IsPronoun())
    {
        return g_apPlanStatFns[m_otherSlot.wPronoun](m_pOwner, m_pNode);
    }

    return static_cast<short>(m_otherSlot.n);
}

// FUNCTION: HYPERX 0x0045dc70
short CollInteractor::GetModelSlot()
{
    if (m_ownSlotOrMessage.IsPronoun())
    {
        return g_apPlanStatFns[m_ownSlotOrMessage.wPronoun](m_pOwner, m_pNode);
    }

    return static_cast<short>(m_ownSlotOrMessage.n);
}

// FUNCTION: HYPERX 0x0045dd00
CollInteractor* CollInteractor::Setup(EngineObject* pObject, PlanNode* pPlan)
{
    m_pOwner           = pObject;
    m_nActiveState     = 0;
    m_otherSlot        = pPlan->m_pArgs[ARG_KEY];
    m_ownSlotOrMessage = pPlan->m_pArgs[kArgTag];
    m_nBehaviour       = PLAN_TOKEN_NO_BEH_CHANGE;

    auto  nCount = static_cast<short>(pPlan->m_nArgCount - kArgConditionFirst);
    auto* pEntry = pPlan->m_pArgs + kArgConditionFirst;

    while (nCount-- != 0)
    {
        auto* const pInstr  = pEntry->pNode;
        const auto  nOpcode = static_cast<unsigned int>(pInstr->m_nOpcode);
        const auto  nClass  = nOpcode & kOpCategory;

        switch (nClass)
        {
            case CATEGORY_DEBUG:
                break;

            case CATEGORY_BEHAVIOUR:
                switch (nOpcode & kOpMember)
                {
                    case MEMBER_CHANGE_TO_BEHAVIOR:
                        m_nBehaviour = pInstr->m_pArgs[0].n;
                        break;
                }
                break;

            case CATEGORY_ACTION:
                if ((nOpcode & kOpMember) == MEMBER_SET_INTERACTOR_PARAM)
                {
                    auto* const pState = m_pOwner->GetPlanState();

                    g_pfnPlanOpcodeHandlers[static_cast<short>(pInstr->m_pArgs[0].n)].pfnSetParam(
                        m_pOwner, pInstr, pState->m_pCurrent, this);
                }
                break;

            case CATEGORY_ENABLE:
                m_pNode = pInstr;
                break;

            default:
                // STRING: HYPERX 0x004c44c8
                throw "Invalid Instruction object in CollInteractor setup\n";
        }

        pEntry++;
    }

    return this;
}

// FUNCTION: HYPERX 0x0045ddf0
void CollInteractor::Process(int nArg, short* pOutcome, int nMode)
{
    PlanWord* pCursor = nullptr;
    short     nCount;

    m_nActiveState = 0;

    // Mode one runs only for a network object and mode zero only for the others.
    if (!((nMode == 1 && m_pOwner->GetPhase() == ObjectPhase::kNetwork) || (nMode == 0 && m_pOwner->GetPhase() != ObjectPhase::kNetwork)))
    {
        return;
    }

    m_wSlot      = -1;
    m_wBehaviour = 0;

    auto* const pNode = m_pNode;

    if (pNode != nullptr)
    {
        pCursor = pNode->m_pArgs;
        nCount  = pNode->m_nArgCount;
    }
    else
    {
        // With a count of nought nothing reads the cursor.
        nCount = 0;
    }

    ExecuteScript(pCursor, nCount, nArg, pOutcome);

    if (static_cast<unsigned int>(m_nBehaviour) < static_cast<unsigned int>(PLAN_TOKEN_NO_BEH_CHANGE) && m_bEnabled != 0)
    {
        m_pOwner->DispatchSceneMessage(kMsgExitBehavior, 0, kDecisionLogFresh, m_pOwner->GetSlot(), 0);

        m_pOwner->GetPlanState()->ActivateBehavior(static_cast<unsigned int>(m_nBehaviour));
    }

    if (nMode == 0 && m_pOwner->GetPhase() != ObjectPhase::kNetwork && m_pOwner->GetPhysStateObj() != nullptr)
    {
        m_pOwner->GetCPlayerStateLock()->lock();

        m_pOwner->EnqueueSceneEvent(MsgEvent::Pack(m_wEventKind,
                                                   static_cast<short>(m_otherSlot.n),
                                                   static_cast<short>(m_ownSlotOrMessage.n),
                                                   *pOutcome),
                                    nArg);

        m_pOwner->GetCPlayerStateLock()->unlock();
    }

    m_pOwner->GetPlanState()->m_nCollisionRearmTicks = kCollisionRearmTicksAfterRun;

    if (m_wSlot >= 0)
    {
        m_pOwner->DispatchSceneMessage(kMsgExitBehavior, 0, kDecisionLogFresh, m_pOwner->GetSlot(), 0);

        m_pOwner->InstantiateAIPlan(m_wSlot);

        m_pOwner->GetPlanState()->ActivateBehavior(m_wBehaviour);
    }
}

// FUNCTION: HYPERX 0x0046bb90
CollInteractor::~CollInteractor() = default;
