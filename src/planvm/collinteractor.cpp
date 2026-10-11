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

#define ARG_KEY             0
#define ARG_TAG             1
#define ARG_CONDITION_FIRST 2

// Its marker is in src/planvm/planvm.cpp.
extern PlanOpcodeSlot g_pfnPlanOpcodeHandlers[141];

// FUNCTION: HYPERX 0x0045dc00
void CollInteractor::Discard()
{
    m_nActiveState = 0;
    m_bEnabled     = 1;
    m_pNode        = 0;
    pNext          = 0;

    g_pCollInteractorPool->Insert(this);
}

// FUNCTION: HYPERX 0x0045dc40
short CollInteractor::GetOtherSlot()
{
    if (m_otherSlot.IsSelector())
    {
        return g_apPlanStatFns[m_otherSlot.wSelector](m_pOwner, m_pNode);
    }

    return (short)m_otherSlot.n;
}

// FUNCTION: HYPERX 0x0045dc70
short CollInteractor::GetModelSlot()
{
    if (m_ownSlotOrMessage.IsSelector())
    {
        return g_apPlanStatFns[m_ownSlotOrMessage.wSelector](m_pOwner, m_pNode);
    }

    return (short)m_ownSlotOrMessage.n;
}

// FUNCTION: HYPERX 0x0045dd00
CollInteractor* CollInteractor::Setup(EngineObject* pObject, PlanNode* pPlan)
{
    m_pOwner           = pObject;
    m_nActiveState     = 0;
    m_otherSlot        = pPlan->m_pArgs[ARG_KEY];
    m_ownSlotOrMessage = pPlan->m_pArgs[ARG_TAG];
    m_nBehaviour       = PLAN_TOKEN_NO_BEH_CHANGE;

    short     nCount = (short)(pPlan->m_nArgCount - ARG_CONDITION_FIRST);
    PlanWord* pEntry = pPlan->m_pArgs + ARG_CONDITION_FIRST;

    while (nCount-- != 0)
    {
        PlanNode* const    pInstr  = pEntry->pNode;
        const unsigned int nOpcode = (unsigned int)pInstr->m_nOpcode;
        const unsigned int nClass  = nOpcode & OP_CATEGORY;

        switch (nClass)
        {
            case CATEGORY_DEBUG:
                break;

            case CATEGORY_BEHAVIOUR:
                switch (nOpcode & OP_MEMBER)
                {
                    case MEMBER_CHANGE_TO_BEHAVIOR:
                        m_nBehaviour = pInstr->m_pArgs[0].n;
                        break;
                }
                break;

            case CATEGORY_ACTION:
                if ((nOpcode & OP_MEMBER) == MEMBER_SET_INTERACTOR_PARAM)
                {
                    PlanState* const pState = m_pOwner->GetPlanState();

                    g_pfnPlanOpcodeHandlers[(short)pInstr->m_pArgs[0].n].pfnSetParam(
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
    PlanWord* pCursor;
    short     nCount;

    m_nActiveState = 0;

    // Mode one runs only for a network object and mode zero only for the others.
    if (!((nMode == 1 && m_pOwner->GetPhase() == OBJECT_PHASE_NETWORK) || (nMode == 0 && m_pOwner->GetPhase() != OBJECT_PHASE_NETWORK)))
    {
        return;
    }

    m_wSlot      = -1;
    m_wBehaviour = 0;

    PlanNode* const pNode = m_pNode;

    if (pNode != 0)
    {
        pCursor = pNode->m_pArgs;
        nCount  = pNode->m_nArgCount;
    }
    else
    {
        // The cursor stays unset; with a count of nought nothing reads it.
        nCount = 0;
    }

    ExecuteScript(pCursor, nCount, nArg, pOutcome);

    if ((unsigned int)m_nBehaviour < PLAN_TOKEN_NO_BEH_CHANGE && m_bEnabled != 0)
    {
        m_pOwner->DispatchSceneMessage(MSG_EXIT_BEHAVIOR, 0, DECISION_LOG_FRESH, m_pOwner->GetSlot(), 0);

        m_pOwner->GetPlanState()->ActivateBehavior((unsigned int)m_nBehaviour);
    }

    if (nMode == 0 && m_pOwner->GetPhase() != OBJECT_PHASE_NETWORK && m_pOwner->GetPhysStateObj() != 0)
    {
        EnterCriticalSection(m_pOwner->GetCPlayerStateLock());

        m_pOwner->EnqueueSceneEvent(MsgEvent::Pack(m_wEventKind,
                                                   (short)m_otherSlot.n,
                                                   (short)m_ownSlotOrMessage.n,
                                                   (short)*pOutcome),
                                    nArg);

        LeaveCriticalSection(m_pOwner->GetCPlayerStateLock());
    }

    m_pOwner->GetPlanState()->m_nTickBudget = TICK_BUDGET_DISPATCHED;

    if (m_wSlot >= 0)
    {
        m_pOwner->DispatchSceneMessage(MSG_EXIT_BEHAVIOR, 0, DECISION_LOG_FRESH, m_pOwner->GetSlot(), 0);

        m_pOwner->InstantiateAIPlan(m_wSlot);

        m_pOwner->GetPlanState()->ActivateBehavior(m_wBehaviour);
    }
}

// FUNCTION: HYPERX 0x0046bb90
CollInteractor::~CollInteractor()
{
}
