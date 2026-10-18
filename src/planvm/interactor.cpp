#include "common/compat.h"
#include "planvm/planaction.h"
#include "planvm/decisionlog.h"
#include "planvm/planfunctionids.h"
#include "interactor.h"
#include "conditiontable.h"
#include "plannode.h"
#include "planstate.h"
#include "engine/gameengine.h"
#include "planvm/agendaitem.h"
#include "planvm.h"
#include "common/pooledclasses.h"
#include "engine/msgevent.h"
#include "common/pooledownerroot.h"
#include "engine/objectphase.h"
#include "engine/engineobject.h"

// The action table is the opcode table's tail, 141 slots on; the node count is never bounds-checked.

// The tables are marked where they are filled, so they are declared bare here.

extern PlanHandlerFn     g_pfnPlanActionHandlers[140];
extern PlanOpcodeSlot    g_pfnPlanOpcodeHandlers[141];
extern BehParamHandlerFn g_apfnBehaviorParamHandlers[17];

// FUNCTION: HYPERX 0x0045d860
void Interactor::ExecuteScript(PlanWord* pCursor, short nCount, int nArg, short* pState)
{
    PlanNode*   pNode;
    AgendaItem* pBubble;
    short       nField;

    GameBehaviorObject* const pOwner = m_pOwner->GetPlanState()->m_pCurrent;

    while (nCount-- != 0)
    {
        pNode = pCursor->pNode;

        const unsigned int nOpcode = (unsigned int)pNode->m_nOpcode;

        switch (nOpcode & OP_CATEGORY)
        {
            case CATEGORY_ACTION:
                switch (nOpcode & OP_MEMBER)
                {
                    case MEMBER_DO_NET_ACTION:
                        if (m_pOwner->GetPhase() == OBJECT_PHASE_LOCAL)
                        {
                            break;
                        }

                        g_pfnPlanActionHandlers[(short)pNode->m_pArgs[0].n](this, pNode, pState, nArg);
                        break;

                    case MEMBER_DO_LOCAL_ACTION:
                        if (m_pOwner->GetPhase() == OBJECT_PHASE_NETWORK)
                        {
                            break;
                        }

                    case MEMBER_DO_ACTION:
                        g_pfnPlanActionHandlers[(short)pNode->m_pArgs[0].n](this, pNode, pState, nArg);
                        break;

                    case MEMBER_SET_INTERACTOR_PARAM:
                    {
                        PlanState* const pPlan = m_pOwner->GetPlanState();

                        g_pfnPlanOpcodeHandlers[(short)pNode->m_pArgs[0].n].pfnSetParam(
                            m_pOwner, pNode, pPlan->m_pCurrent, this);
                        break;
                    }
                }
                break;

            case CATEGORY_BEHAVIOUR:
                switch (nOpcode & OP_MEMBER)
                {
                    case MEMBER_CHANGE_TO_BEHAVIOR:
                        m_nBehaviour = pNode->m_pArgs[0].n;
                        break;

                    case MEMBER_SET_BEHAVIOR_PARAM:
                    {
                        PlanState* const pPlan = m_pOwner->GetPlanState();

                        g_apfnBehaviorParamHandlers[(short)pNode->m_pArgs[0].n](
                            m_pOwner, pNode, pPlan->m_pCurrent);
                        break;
                    }
                }
                break;

            case CATEGORY_PLAN_CHANGE:
                if ((nOpcode & OP_MEMBER) == MEMBER_CHANGE_OF_PLAN)
                {
                    m_wSlot      = (short)pNode->m_pArgs[0].n;
                    m_wBehaviour = (short)pNode->m_pArgs[1].n;
                }
                break;

            case CATEGORY_DECIDE:
                if (DecisionLog::IsRecording(*pState) != 0)
                {
                    g_pfnPlanOpcodeHandlers[(short)pNode->m_pArgs[0].n + PLAN_DECISION_BASE].pfnDecide(
                        this, pNode, &nField, nArg);

                    DecisionLog::Push(pState, nField);
                }
                else
                {
                    DecisionLog::Pop(pState, &nField);
                }

                if (pNode->m_nArgCount - 1 > nField)
                {
                    ExecuteScript(&pNode->m_pArgs[1 + nField], 1, nArg, pState);
                }
                break;

            case CATEGORY_AGENDA:
                switch (nOpcode)
                {
                    case (int)CATEGORY_AGENDA | MEMBER_ADD_AGENDA_ITEM:
                        pBubble = (AgendaItem*)g_apAgendaItemPools[(short)pNode->m_pArgs[0].n]->Alloc();

                        if (pNode->m_nArgCount == 1)
                        {
                            pBubble->Bind(m_pOwner, pOwner);
                        }
                        else
                        {
                            pBubble->BindWithNode(m_pOwner, pOwner, pNode, nArg);
                        }

                        pBubble->Activate();
                        break;

                    case (int)CATEGORY_AGENDA | MEMBER_SET_TIMEOUTMSG:
                        pBubble = AllocAgendaItem(Agenda_CTimerActivity);

                        pBubble->BindWithNode(m_pOwner, pOwner, pNode, nArg);

                        pBubble->Activate();
                        break;

                    case (int)CATEGORY_AGENDA | MEMBER_REMOVE_AGENDA_ITEM:
                        pOwner->RemoveAgendaItem(pNode->m_pArgs[0].n);
                        break;
                }
                break;

            case CATEGORY_BLOCK:
                ExecuteScript(pNode->m_pArgs, pNode->m_nArgCount, nArg, pState);
                break;

            default:
                // STRING: HYPERX 0x004c4494
                throw "Invalid Instruction object in Interactor Process\n";
        }

        pCursor++;
    }
}

// FUNCTION: HYPERX 0x0046bb40
int Interactor::GetActiveState()
{
    return m_nActiveState;
}

// FUNCTION: HYPERX 0x0046bb60
int Interactor::GetState()
{
    return m_nMember;
}

// FUNCTION: HYPERX 0x0046bb70
EngineObject* Interactor::GetActor()
{
    return m_pOwner;
}

// FUNCTION: HYPERX 0x0046bb80
void Interactor::SetActiveState(int nState)
{
    m_nActiveState = nState;
}
