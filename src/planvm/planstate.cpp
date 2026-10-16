#include "common/compat.h"
#include "planvm.h"
#include "plannode.h"
#include "planvm/planfunctionids.h"
#include "common/pooledclasses.h"
#include "engine/gameengine.h"
#include "engine/devicector.h"
#include "planarg.h"
#include "planopcodetable.h"
#include "planvmglobals.h"
#include "ui/hudglobals.h"
#include "planvm/agendaitem.h"
#include <string.h>
#include "collinteractor.h"
#include "msginteractor.h"
#include "engine/engineobject.h"
#include "planvm/planinteractorfactories.h"
#include "common/pooledownerroot.h"

// The plan PlanState::Reinitialize compiles: nine words, one compound node then END_PLAN.
// GLOBAL: HYPERX 0x004c689c
extern PlanWord g_aReinitPlanImage[9];

// The constructor skips the two fields the object's plan table fills in itself.

// FUNCTION: HYPERX 0x0047aef0
PlanState::PlanState()
{
    m_pOwner           = 0;
    m_nBehaviorCount   = 0;
    m_ppBehaviors      = 0;
    m_ppCursor         = 0;
    m_pHead            = 0;
    m_pCurrent         = 0;
    m_nIndex           = 0;
    m_nPrevious        = 0;
    m_pCollInteractors = 0;
    m_pMsgInteractors  = 0;
    m_nInteractorCount = 0;
    m_pflLocalReals    = 0;
    m_nLocalRealCount  = 0;
    m_apAnonLocals     = 0;
    m_nAnonLocals      = 0;
}

// FUNCTION: HYPERX 0x0047af30
PlanState::~PlanState()
{
    Teardown();

    FreeCompiledBehaviors(m_nBehaviorCount);

    if (m_apAnonLocals != 0)
    {
        delete[] m_apAnonLocals;

        m_apAnonLocals = 0;
    }
}

// FUNCTION: HYPERX 0x0047af60
void PlanState::FreeCompiledBehaviors(short nCount)
{
    PlanNode** const ppNode = m_ppBehaviors;

    if (ppNode != 0)
    {
        for (short i = 0; i < nCount; i++)
        {
            DestroyNode(ppNode[i]);
        }

        delete[] m_ppBehaviors;
    }
}

// The operand array and the node are freed whatever the opcode, even an unknown one.
// FUNCTION: HYPERX 0x0047afa0
void PlanState::DestroyNode(PlanNode* pNode)
{
    PlanWord* pOperands;
    PlanWord* pCursor;
    int       nCount;
    int       nIndex;

    if (pNode != 0 && pNode->m_nArgCount != 0)
    {
        switch ((unsigned int)pNode->m_nOpcode)
        {
            case CATEGORY_DEBUG | MEMBER_SET_TRACE:
                delete[] pNode->m_pArgs[0].psz;
                break;

            case CATEGORY_BEHAVIOUR | MEMBER_DECLARE_BEHAVIOR:
                delete[] pNode->m_pArgs[1].psz;
                // Falls through: a declaration's children start where a collision interactor's do.

            case CATEGORY_COLLISION_INTERACTOR | MEMBER_SET_COLLISION_INTERACTOR:
                nCount    = pNode->m_nArgCount;
                pOperands = pNode->m_pArgs;
                pCursor   = &pOperands[nCount - 1];

                for (nIndex = nCount - 1; nIndex >= 2; nIndex--)
                {
                    DestroyNode(pCursor->pNode);
                    pCursor--;
                }

                break;

            case CATEGORY_MESSAGE_INTERACTOR | MEMBER_SET_MESSAGE_INTERACTOR:
            case CATEGORY_DECIDE | MEMBER_DECIDE_BY_AMONG:
            case CATEGORY_DECIDE | MEMBER_DECIDE_BY_WITH_AMONG:
                nCount    = pNode->m_nArgCount;
                pOperands = pNode->m_pArgs;
                pCursor   = &pOperands[nCount - 1];

                for (nIndex = nCount - 1; nIndex >= 1; nIndex--)
                {
                    DestroyNode(pCursor->pNode);
                    pCursor--;
                }

                break;

            case CATEGORY_ACTION | MEMBER_DO_ACTION:
                // The one action whose second operand is a string the node owns.
                if (pNode->m_pArgs[0].n == ACF_SET_HEAD_CAM_TEXT)
                {
                    delete[] pNode->m_pArgs[1].psz;
                }

                break;

            case CATEGORY_GLOBAL_INTERACTORS:
            case CATEGORY_BLOCK:
            case CATEGORY_ENABLE:
                nCount    = pNode->m_nArgCount;
                pOperands = pNode->m_pArgs;
                pCursor   = &pOperands[nCount - 1];

                for (nIndex = nCount - 1; nIndex >= 0; nIndex--)
                {
                    DestroyNode(pCursor->pNode);
                    pCursor--;
                }

                break;

            default:
                break;
        }

        delete[] pNode->m_pArgs;
    }

    delete pNode;
}

// FUNCTION: HYPERX 0x0047b0e0
void PlanState::SetOwnerAndReset(EngineObject* pOwner, PlanNode** ppBehaviors)
{
    m_pOwner = pOwner;

    if (m_ppBehaviors != ppBehaviors)
    {
        FreeCompiledBehaviors(m_nBehaviorCount);
    }

    m_ppBehaviors = ppBehaviors;
    m_ppCursor    = ppBehaviors;
}

// FUNCTION: HYPERX 0x0047b110
unsigned char PlanState::SetAndInit(const PlanWord* pStream)
{
    const short nOldCount = m_nBehaviorCount;

    PlanNode** const ppCompiled = Compile(pStream);

    if (ppCompiled == 0)
    {
        return 0;
    }

    if (m_ppBehaviors != ppCompiled)
    {
        FreeCompiledBehaviors(nOldCount);
    }

    m_ppBehaviors = ppCompiled;
    m_ppCursor    = ppCompiled;

    return 1;
}

// An index past the end walks off the list: there is no bound.
// FUNCTION: HYPERX 0x0047b150
GameBehaviorObject* PlanState::GetBehaviorByIndex(unsigned int nIndex)
{
    short nLeft;

    GameBehaviorObject* pAt = m_pHead;

    if (nIndex == PLAN_TOKEN_PREVIOUS_BEH)
    {
        nLeft = m_nPrevious;
    }
    else
    {
        nLeft = (short)nIndex;
    }

    while (nLeft-- > 0)
    {
        pAt = pAt->m_pStandby;
    }

    return pAt;
}

// The index is continuous across the two lists, split at m_nInteractorCount.

// FUNCTION: HYPERX 0x0047b180
CollInteractor* PlanState::WalkLinkedList(short nIndex)
{
    Interactor* pNode;
    short       nAt;

    if (nIndex < m_nInteractorCount)
    {
        pNode = m_pCollInteractors;
        nAt   = 0;
    }
    else
    {
        pNode = m_pCurrent->m_pCollInteractors;
        nAt   = m_nInteractorCount;
    }

    while (nAt++ < nIndex)
    {
        if (pNode != 0)
        {
            pNode = pNode->pNext;
        }
    }

    return (CollInteractor*)pNode;
}

// A negative index runs the counted walk rather than skipping it.
// FUNCTION: HYPERX 0x0047b1c0
void PlanState::ActivateBehavior(unsigned int nIndex)
{
    short nSlot = (short)nIndex;

    if (nIndex == PLAN_TOKEN_PREVIOUS_BEH)
    {
        nSlot = m_nPrevious;
    }

    m_ppCursor = m_ppBehaviors;

    if (m_pCurrent->m_bDigressive == 0)
    {
        m_nPrevious = m_nIndex;
    }

    m_nIndex = nSlot;

    m_pCurrent->SetInteractorsActive(0);

    m_pCurrent = m_pHead;

    while (nSlot-- != 0)
    {
        if (m_pCurrent->m_pStandby != 0)
        {
            m_ppCursor++;
            m_pCurrent = m_pCurrent->m_pStandby;
        }
        else
        {
            m_pCurrent = m_pHead;
            m_ppCursor = m_ppBehaviors;
        }
    }

    m_pCurrent->TickAndSortMessages();
    m_pCurrent->SendTimedMessage();

    while (m_pCurrent->CanPassOn())
    {
        m_ppCursor++;
        m_pCurrent = m_pCurrent->m_pStandby;

        m_pCurrent->TickAndSortMessages();
        m_pCurrent->SendTimedMessage();
    }
}

#define PLAN_NODES_FIRST 500
#define PLAN_NODES_GROW  250

// FUNCTION: HYPERX 0x0047b290
void PlanState::PostUpdateCleanup()
{
    if (m_nTickBudget-- == 0)
    {
        Interactor* pNode = m_pCollInteractors;

        while (pNode != 0)
        {
            pNode->SetActiveState(1);

            pNode = pNode->pNext;
        }
    }

    m_pCurrent->UpdateBehaviorPollCountdown();
}

// A plan of 500 nodes or more keeps parsing into the array the grow just released.
// FUNCTION: HYPERX 0x0047b2d0
PlanNode** PlanState::Compile(const PlanWord* pStream)
{
    int nIndex;

    short     nCapacity = PLAN_NODES_FIRST;
    PlanWord* aWords    = new PlanWord[nCapacity];
    memset(aWords, 0, nCapacity * sizeof(PlanWord));

    m_nBehaviorCount = 0;
    m_nAnonLocals    = 0;

    if (m_apAnonLocals != 0)
    {
        delete[] m_apAnonLocals;

        m_apAnonLocals = 0;
    }

    PlanWord* pWord = aWords;
    char      cFlag = 0;

    while (ParseNodeStream(1, &pStream, pWord, &cFlag) != 0)
    {
        pWord++;
        m_nBehaviorCount++;

        if (m_nBehaviorCount == nCapacity)
        {
            nCapacity += PLAN_NODES_GROW;
            PlanWord* const aGrown = new PlanWord[nCapacity];

            for (nIndex = 0; nIndex < m_nBehaviorCount; nIndex++)
            {
                aGrown[nIndex] = aWords[nIndex];
            }

            delete[] aWords;

            aWords = aGrown;
        }

        if (pWord[-1].pNode->m_nOpcode == CATEGORY_END_PLAN)
        {
            PlanNode** const apResult = new PlanNode*[m_nBehaviorCount];

            for (nIndex = 0; nIndex < m_nBehaviorCount; nIndex++)
            {
                apResult[nIndex] = aWords[nIndex].pNode;
            }

            delete[] aWords;

            if (m_nAnonLocals != 0)
            {
                m_apAnonLocals = new Interactor*[m_nAnonLocals];
            }

            return apResult;
        }
    }

    // STRING: HYPERX 0x004c68c0
    throw (char*)"ERROR: No Instruction returned from DecodeArg at Top Level\n";
}

// Only a compound node answers yes; CHANGE_OF_PLAN sets the flag that resolves strings to indices.
// FUNCTION: HYPERX 0x0047b430
char PlanState::ParseNodeStream(int nDepth, const PlanWord** ppStream, PlanWord* pOut, char* pcPlanRef)
{
    char szName[32];

    const PlanWord* pCursor = *ppStream;

    if (pCursor->n == PLAN_MARKER_NODE)
    {
        pCursor++;

        PlanNode* pNode = new PlanNode;

        pNode->m_nOpcode = pCursor->n;
        pCursor++;

        pNode->m_nArgCount = 0;
        pNode->m_pArgs     = 0;

        if (pNode->m_nOpcode == CATEGORY_END_PLAN)
        {
            pNode->m_pArgs = 0;

            *ppStream   = pCursor;
            pOut->pNode = pNode;

            return 1;
        }

        pNode->m_nArgCount = (short)pCursor->n;
        pCursor++;

        if (pNode->m_nArgCount != 0)
        {
            const short nMasked = (short)(pNode->m_nArgCount & 0x7fff);

            PlanWord* pChild = pNode->m_pArgs = new PlanWord[nMasked];

            for (int nChild = 0; nChild < nMasked; nChild++)
            {
                const unsigned int nOpcode = pNode->m_nOpcode;

                if ((nOpcode & OP_CATEGORY) == CATEGORY_PLAN_CHANGE)
                {
                    if ((nOpcode & OP_MEMBER) == MEMBER_CHANGE_OF_PLAN)
                    {
                        *pcPlanRef = 1;
                    }
                }
                else if ((nOpcode & OP_CATEGORY) == CATEGORY_ACTION && (nOpcode & OP_MEMBER) == MEMBER_SET_INTERACTOR_PARAM && pNode->m_pArgs->n == 0)
                {
                    m_nAnonLocals++;
                }

                ParseNodeStream(nDepth + 1, &pCursor, pChild++, pcPlanRef);
            }
        }
        else
        {
            pNode->m_pArgs = 0;
        }

        *ppStream   = pCursor;
        pOut->pNode = pNode;

        return 1;
    }

    if (pCursor->n == PLAN_MARKER_STRING)
    {
        pCursor++;

        short nLength = 0;

        while (pCursor[nLength].n != 0)
        {
            szName[nLength] = (char)pCursor[nLength].n;
            nLength++;
        }

        szName[nLength] = '\0';
        nLength++;

        if (*pcPlanRef != 0)
        {
            pOut->n = m_pOwner->FindPlanByName(szName);

            *pcPlanRef = 0;
        }
        else
        {
            pOut->psz = strcpy(new char[nLength], szName);
        }

        *ppStream = pCursor + nLength;

        return 0;
    }

    *pcPlanRef = '\0';
    *ppStream  = pCursor + 1;

    const PlanWord leaf = *pCursor;

    if (leaf.IsPronoun())
    {
        g_apPlanArgFns[leaf.wPronoun](&m_pOwner, pCursor, &pOut->n);

        return 0;
    }

    pOut->n = leaf.n;

    return 0;
}

// Argument one is the behaviour's name.
#define ARG_NAME 1

// What the state hands out is the tick budget the plan machine spends per activation.
#define TICK_BUDGET 10

// FUNCTION: HYPERX 0x0047b680
PlanState* PlanState::Instantiate()
{
    PlanWord* pChildWord;
    short     nRemaining;
    short     nOrder;
    short     nSlot;

    Interactor*         pCondition  = 0;
    GameBehaviorObject* pBehaviour  = 0;
    Interactor*         pMessage    = 0;
    int                 nBehaviours = 0;
    int                 nConditions = 0;

    if (m_ppCursor != 0)
    {
        for (;;)
        {
            PlanNode* const pNode = *m_ppCursor;

            GameBehaviorObject* const pPrevious = pBehaviour;

            switch ((unsigned int)pNode->m_nOpcode & OP_CATEGORY)
            {
                case CATEGORY_PLAN_CHANGE:
                    nRemaining = pNode->m_nArgCount;

                    m_nLocalRealCount = nRemaining;

                    m_pflLocalReals = new float[nRemaining];

                    for (nSlot = 0; nSlot < m_nLocalRealCount; nSlot++)
                    {
                        m_pflLocalReals[nSlot] = g_flZeroPlanState;
                    }

                    break;

                case CATEGORY_BEHAVIOUR:
                    if (((unsigned int)pNode->m_nOpcode & OP_MEMBER) != MEMBER_DECLARE_BEHAVIOR)
                    {
                        // STRING: HYPERX 0x004c6960
                        throw (char*)"Expected a Behavior Declaration\n";
                    }

                    pBehaviour = (GameBehaviorObject*)g_apBehaviorRegistry[pNode->m_pArgs[0].n]
                                     ->Alloc();

                    pBehaviour->m_bDigressive = 0;

                    pBehaviour->Init(m_pOwner, pNode, 0);

                    pBehaviour->AssignPriorities(nConditions);

                    pBehaviour->SetInteractorsActive(0);

                    pBehaviour->m_pszName = pNode->m_pArgs[ARG_NAME].psz;

                    nOrder = (short)nBehaviours;

                    nBehaviours++;

                    pBehaviour->m_wSyncValue = nOrder;

                    if (pPrevious != 0)
                    {
                        pPrevious->m_pStandby = pBehaviour;
                    }
                    else
                    {
                        m_pHead = pBehaviour;
                    }

                    break;

                case CATEGORY_GLOBAL_INTERACTORS:
                    nRemaining = pNode->m_nArgCount;
                    pChildWord = pNode->m_pArgs;

                    while (nRemaining-- != 0)
                    {
                        PlanNode* const pChild = pChildWord->pNode;

                        switch ((unsigned int)pChild->m_nOpcode & OP_CATEGORY)
                        {
                            case CATEGORY_COLLISION_INTERACTOR:
                                pCondition = PlanInteractorFactories::CreateCollInteractor(m_pOwner, pChild, 0, pCondition);

                                nOrder = (short)nConditions;

                                nConditions++;

                                pCondition->m_wEventKind = nOrder;

                                pCondition->SetMember((unsigned int)pChild->m_nOpcode & OP_MEMBER);

                                break;

                            case CATEGORY_MESSAGE_INTERACTOR:
                                pMessage = PlanInteractorFactories::CreateMsgInteractor(m_pOwner, pChild, 0, pMessage);

                                pMessage->SetMember((unsigned int)pChild->m_nOpcode & OP_MEMBER);

                                break;

                            default:
                                // STRING: HYPERX 0x004c6928
                                throw (char*)"Invalid Instruction object in global interactor setup\n";
                        }

                        pChildWord++;
                    }

                    break;

                case CATEGORY_END_PLAN:
                    break;

                default:
                    // STRING: HYPERX 0x004c68fc
                    throw (char*)"Invalid Instruction in Plan Instantiation\n\n";
            }

            m_ppCursor++;

            if (((unsigned int)pNode->m_nOpcode & OP_CATEGORY) == CATEGORY_END_PLAN)
            {
                pBehaviour->m_pStandby = 0;

                m_nIndex    = 0;
                m_nPrevious = 0;
                m_ppCursor  = m_ppBehaviors;
                m_pCurrent  = m_pHead;

                m_nInteractorCount = (short)nConditions;

                m_nTickBudget = TICK_BUDGET;

                return this;
            }
        }
    }

    return 0;
}

// The next pointer is read before each discard, because discarding frees the node.

// FUNCTION: HYPERX 0x0047b900
void PlanState::Teardown()
{
    Interactor* pNext;

    if (m_pflLocalReals != 0)
    {
        delete[] m_pflLocalReals;

        m_pflLocalReals   = 0;
        m_nLocalRealCount = 0;
    }

    Interactor* pNode = m_pCollInteractors;

    while (pNode != 0)
    {
        pNext = pNode->pNext;

        pNode->Discard();

        pNode = pNext;
    }

    pNode = m_pMsgInteractors;

    m_pCollInteractors = 0;

    while (pNode != 0)
    {
        pNext = pNode->pNext;

        pNode->Discard();

        pNode = pNext;
    }

    m_pMsgInteractors  = 0;
    m_nInteractorCount = 0;

    if (m_pOwner != 0)
    {
        AgendaItem* pObserver;
        AgendaItem* pNextObserver;

        pObserver = m_pOwner->GetObserver();

        while (pObserver != 0)
        {
            pNextObserver = pObserver->m_pNext;

            pObserver->OnListChanged();

            pObserver = pNextObserver;
        }

        m_pOwner->SetObserver(0);
    }

    if (m_pHead != 0)
    {
        // Reset cascades through the standby link, so the head alone is enough.
        m_pHead->Reset();
    }

    m_pHead    = 0;
    m_pCurrent = 0;
}

// FUNCTION: HYPERX 0x0047b990
void PlanState::Reinitialize()
{
    SetAndInit(g_aReinitPlanImage);
}

// One DECLARE_BEHAVIOR node, a leaf child of six and the string "x", then END_PLAN.
PlanWord g_aReinitPlanImage[9] = {
    { PLAN_MARKER_NODE }, { CATEGORY_BEHAVIOUR | MEMBER_DECLARE_BEHAVIOR }, { 2 }, { 6 }, { PLAN_MARKER_STRING }, { 'x' }, { 0 }, { PLAN_MARKER_NODE }, { CATEGORY_END_PLAN }
};
