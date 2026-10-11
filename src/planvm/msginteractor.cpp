#include "common/compat.h"
#include "plannode.h"
#include "engine/gameengine.h"
#include "planvm.h"
#include "planstate.h"
#include "globals.h"
#include "msginteractor.h"
#include "collinteractor.h"
#include "common/pooledownerroot.h"
#include "engine/engineobject.h"
#include "planvm/decisionlog.h"

#define ARG_KEY           0
#define ARG_MESSAGE_FIRST 1

// Its marker is in src/planvm/planvm.cpp.
extern PlanOpcodeSlot g_pfnPlanOpcodeHandlers[141];

// FUNCTION: HYPERX 0x0045dc20
void MsgInteractor::Discard()
{
    m_nActiveState = 0;
    m_bEnabled     = 1;
    m_pNode        = 0;
    pNext          = 0;

    g_pMsgInteractorPool->Insert(this);
}

// FUNCTION: HYPERX 0x0045dfc0
MsgInteractor* MsgInteractor::Setup(EngineObject* pObject, PlanNode* pPlanArg)
{
    m_pOwner           = pObject;
    m_nActiveState     = 0;
    m_ownSlotOrMessage = pPlanArg->m_pArgs[ARG_KEY];
    m_nBehaviour       = PLAN_TOKEN_NO_BEH_CHANGE;
    m_otherSlot.n      = pObject->GetSlot();

    PlanWord* pEntry = pPlanArg->m_pArgs + ARG_MESSAGE_FIRST;
    short     nCount = (short)(pPlanArg->m_nArgCount - ARG_MESSAGE_FIRST);

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
                // STRING: HYPERX 0x004c44fc
                throw "Invalid Instruction object in MsgInteractor setup\n";
        }

        pEntry++;
    }

    return this;
}

// FUNCTION: HYPERX 0x0045e0b0
void MsgInteractor::SetSenderAndDispatch(int nArg, short* pState, int nSender)
{
    m_otherSlot.n = nSender;

    Dispatch(nArg, pState);
}

// FUNCTION: HYPERX 0x0045e0d0
void MsgInteractor::Dispatch(int nArg, short* pState)
{
    PlanWord* pCursor;
    short     nCount;

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

    ExecuteScript(pCursor, nCount, nArg, pState);

    if ((unsigned int)m_nBehaviour < PLAN_TOKEN_NO_BEH_CHANGE && m_bEnabled != 0)
    {
        m_pOwner->DispatchSceneMessage(MSG_EXIT_BEHAVIOR, 0, DECISION_LOG_FRESH, m_pOwner->GetSlot(), 0);

        m_pOwner->GetPlanState()->ActivateBehavior((unsigned int)m_nBehaviour);
    }

    m_pOwner->GetPlanState()->m_nTickBudget = TICK_BUDGET_DISPATCHED;

    if (m_wSlot >= 0)
    {
        m_pOwner->DispatchSceneMessage(MSG_EXIT_BEHAVIOR, 0, DECISION_LOG_FRESH, m_pOwner->GetSlot(), 0);

        m_pOwner->InstantiateAIPlan(m_wSlot);

        m_pOwner->GetPlanState()->ActivateBehavior(m_wBehaviour);
    }
}

// FUNCTION: HYPERX 0x0046bd20
short MsgInteractor::GetTargetSlot()
{
    return (short)m_otherSlot.n;
}
