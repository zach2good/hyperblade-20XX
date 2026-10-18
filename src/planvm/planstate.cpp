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
#include <cstring>
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
    m_pOwner             = nullptr;
    m_nBehaviorCount     = 0;
    m_ppBehaviors        = nullptr;
    m_ppCursor           = nullptr;
    m_pHead              = nullptr;
    m_pCurrent           = nullptr;
    m_nIndex             = 0;
    m_nPrevious          = 0;
    m_pCollInteractors   = nullptr;
    m_pMsgInteractors    = nullptr;
    m_nInteractorCount   = 0;
    m_pflLocalReals      = nullptr;
    m_nLocalRealCount    = 0;
    m_apNamedInteractors = nullptr;
    m_nNamedInteractors  = 0;
}

// FUNCTION: HYPERX 0x0047af30
PlanState::~PlanState()
{
    Teardown();

    FreeCompiledBehaviors(m_nBehaviorCount);

    if (m_apNamedInteractors != nullptr)
    {
        delete[] m_apNamedInteractors;

        m_apNamedInteractors = nullptr;
    }
}

// FUNCTION: HYPERX 0x0047af60
void PlanState::FreeCompiledBehaviors(short nCount)
{
    PlanNode** const ppNode = m_ppBehaviors;

    if (ppNode != nullptr)
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

    if (pNode != nullptr && pNode->m_nArgCount != 0)
    {
        switch (static_cast<unsigned int>(pNode->m_nOpcode))
        {
            case static_cast<int>(CATEGORY_DEBUG) | MEMBER_SET_TRACE:
                delete[] pNode->m_pArgs[0].psz;
                break;

            case static_cast<int>(CATEGORY_BEHAVIOUR) | MEMBER_DECLARE_BEHAVIOR:
                delete[] pNode->m_pArgs[1].psz;
                // Falls through: a declaration's children start where a collision interactor's do.

            case static_cast<int>(CATEGORY_COLLISION_INTERACTOR) | MEMBER_SET_COLLISION_INTERACTOR:
                nCount    = pNode->m_nArgCount;
                pOperands = pNode->m_pArgs;
                pCursor   = &pOperands[nCount - 1];

                for (nIndex = nCount - 1; nIndex >= 2; nIndex--)
                {
                    DestroyNode(pCursor->pNode);
                    pCursor--;
                }

                break;

            case static_cast<int>(CATEGORY_MESSAGE_INTERACTOR) | MEMBER_SET_MESSAGE_INTERACTOR:
            case static_cast<int>(CATEGORY_DECIDE) | MEMBER_DECIDE_BY_AMONG:
            case static_cast<int>(CATEGORY_DECIDE) | MEMBER_DECIDE_BY_WITH_AMONG:
                nCount    = pNode->m_nArgCount;
                pOperands = pNode->m_pArgs;
                pCursor   = &pOperands[nCount - 1];

                for (nIndex = nCount - 1; nIndex >= 1; nIndex--)
                {
                    DestroyNode(pCursor->pNode);
                    pCursor--;
                }

                break;

            case static_cast<int>(CATEGORY_ACTION) | MEMBER_DO_ACTION:
                // The one action whose second operand is a string the node owns.
                if (pNode->m_pArgs[0].n == kACFSetHeadCamText)
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
    const auto nOldCount = m_nBehaviorCount;

    PlanNode** const ppCompiled = Compile(pStream);

    if (ppCompiled == nullptr)
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

    auto* pAt = m_pHead;

    if (static_cast<int>(nIndex) == PLAN_TOKEN_PREVIOUS_BEH)
    {
        nLeft = m_nPrevious;
    }
    else
    {
        nLeft = static_cast<short>(nIndex);
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
        if (pNode != nullptr)
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
    auto nSlot = static_cast<short>(nIndex);

    if (static_cast<int>(nIndex) == PLAN_TOKEN_PREVIOUS_BEH)
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
        if (m_pCurrent->m_pStandby != nullptr)
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

constexpr auto kPlanNodesFirst = 500;
constexpr auto kPlanNodesGrow  = 250;

// FUNCTION: HYPERX 0x0047b290
void PlanState::PostUpdateCleanup()
{
    if (m_nCollisionRearmTicks-- == 0)
    {
        Interactor* pNode = m_pCollInteractors;

        while (pNode != nullptr)
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

    short nCapacity = kPlanNodesFirst;
    auto* aWords    = new PlanWord[nCapacity]();
    memset(aWords, 0, nCapacity * sizeof(PlanWord));

    m_nBehaviorCount    = 0;
    m_nNamedInteractors = 0;

    if (m_apNamedInteractors != nullptr)
    {
        delete[] m_apNamedInteractors;

        m_apNamedInteractors = nullptr;
    }

    auto* pWord = aWords;
    char  cFlag = 0;

    while (ParseNodeStream(1, &pStream, pWord, &cFlag) != 0)
    {
        pWord++;
        m_nBehaviorCount++;

        if (m_nBehaviorCount == nCapacity)
        {
            nCapacity += kPlanNodesGrow;
            auto* const aGrown = new PlanWord[nCapacity]();

            for (nIndex = 0; nIndex < m_nBehaviorCount; nIndex++)
            {
                aGrown[nIndex] = aWords[nIndex];
            }

            delete[] aWords;

            aWords = aGrown;
        }

        if (pWord[-1].pNode->m_nOpcode == static_cast<int>(CATEGORY_END_PLAN))
        {
            auto** const apResult = new PlanNode*[m_nBehaviorCount];

            for (nIndex = 0; nIndex < m_nBehaviorCount; nIndex++)
            {
                apResult[nIndex] = aWords[nIndex].pNode;
            }

            delete[] aWords;

            if (m_nNamedInteractors != 0)
            {
                m_apNamedInteractors = new Interactor*[m_nNamedInteractors];
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

    const auto* pCursor = *ppStream;

    if (static_cast<unsigned int>(pCursor->n) == kPlanMarkerNode)
    {
        pCursor++;

        auto* pNode = new PlanNode;

        pNode->m_nOpcode = pCursor->n;
        pCursor++;

        pNode->m_nArgCount = 0;
        pNode->m_pArgs     = nullptr;

        if (pNode->m_nOpcode == static_cast<int>(CATEGORY_END_PLAN))
        {
            pNode->m_pArgs = nullptr;

            *ppStream   = pCursor;
            pOut->pNode = pNode;

            return 1;
        }

        pNode->m_nArgCount = static_cast<short>(pCursor->n);
        pCursor++;

        if (pNode->m_nArgCount != 0)
        {
            const auto nMasked = static_cast<short>(pNode->m_nArgCount & 0x7fff);

            // Zeroed: an integer word fills only the low half, and teardown reads child words as nodes.
            PlanWord* pChild = pNode->m_pArgs = new PlanWord[nMasked]();

            for (int nChild = 0; nChild < nMasked; nChild++)
            {
                const unsigned int nOpcode = pNode->m_nOpcode;

                if ((nOpcode & kOpCategory) == CATEGORY_PLAN_CHANGE)
                {
                    if ((nOpcode & kOpMember) == MEMBER_CHANGE_OF_PLAN)
                    {
                        *pcPlanRef = 1;
                    }
                }
                else if ((nOpcode & kOpCategory) == CATEGORY_ACTION && (nOpcode & kOpMember) == MEMBER_SET_INTERACTOR_PARAM && pNode->m_pArgs->n == 0)
                {
                    m_nNamedInteractors++;
                }

                ParseNodeStream(nDepth + 1, &pCursor, pChild++, pcPlanRef);
            }
        }
        else
        {
            pNode->m_pArgs = nullptr;
        }

        *ppStream   = pCursor;
        pOut->pNode = pNode;

        return 1;
    }

    if (static_cast<unsigned int>(pCursor->n) == kPlanMarkerString)
    {
        pCursor++;

        short nLength = 0;

        while (pCursor[nLength].n != 0)
        {
            szName[nLength] = static_cast<char>(pCursor[nLength].n);
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

    const auto leaf = *pCursor;

    if (leaf.IsPronoun())
    {
        g_apPlanArgFns[leaf.wPronoun](&m_pOwner, pCursor, &pOut->n);

        return 0;
    }

    pOut->n = leaf.n;

    return 0;
}

// Argument one is the behaviour's name.
constexpr auto kArgName = 1;

// Ticks after instantiation before every collision interactor is set active again.
constexpr auto kCollisionRearmTicks = 10;

// FUNCTION: HYPERX 0x0047b680
PlanState* PlanState::Instantiate()
{
    PlanWord* pChildWord;
    short     nRemaining;
    short     nOrder;
    short     nSlot;

    Interactor*         pCondition  = nullptr;
    GameBehaviorObject* pBehaviour  = nullptr;
    Interactor*         pMessage    = nullptr;
    auto                nBehaviours = 0;
    auto                nConditions = 0;

    if (m_ppCursor != nullptr)
    {
        for (;;)
        {
            auto* const pNode = *m_ppCursor;

            auto* const pPrevious = pBehaviour;

            switch (static_cast<unsigned int>(pNode->m_nOpcode) & kOpCategory)
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
                    if ((static_cast<unsigned int>(pNode->m_nOpcode) & kOpMember) != MEMBER_DECLARE_BEHAVIOR)
                    {
                        // STRING: HYPERX 0x004c6960
                        throw (char*)"Expected a Behavior Declaration\n";
                    }

                    pBehaviour = static_cast<GameBehaviorObject*>(g_apBehaviorRegistry[pNode->m_pArgs[0].n]
                                                                      ->Alloc());

                    pBehaviour->m_bDigressive = 0;

                    pBehaviour->Init(m_pOwner, pNode, 0);

                    pBehaviour->AssignPriorities(nConditions);

                    pBehaviour->SetInteractorsActive(0);

                    pBehaviour->m_pszName = pNode->m_pArgs[kArgName].psz;

                    nOrder = static_cast<short>(nBehaviours);

                    nBehaviours++;

                    pBehaviour->m_wSyncValue = nOrder;

                    if (pPrevious != nullptr)
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
                        auto* const pChild = pChildWord->pNode;

                        switch (static_cast<unsigned int>(pChild->m_nOpcode) & kOpCategory)
                        {
                            case CATEGORY_COLLISION_INTERACTOR:
                                pCondition = PlanInteractorFactories::CreateCollInteractor(m_pOwner, pChild, nullptr, pCondition);

                                nOrder = static_cast<short>(nConditions);

                                nConditions++;

                                pCondition->m_wEventKind = nOrder;

                                pCondition->SetMember(static_cast<unsigned int>(pChild->m_nOpcode) & kOpMember);

                                break;

                            case CATEGORY_MESSAGE_INTERACTOR:
                                pMessage = PlanInteractorFactories::CreateMsgInteractor(m_pOwner, pChild, nullptr, pMessage);

                                pMessage->SetMember(static_cast<unsigned int>(pChild->m_nOpcode) & kOpMember);

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

            if ((static_cast<unsigned int>(pNode->m_nOpcode) & kOpCategory) == CATEGORY_END_PLAN)
            {
                pBehaviour->m_pStandby = nullptr;

                m_nIndex    = 0;
                m_nPrevious = 0;
                m_ppCursor  = m_ppBehaviors;
                m_pCurrent  = m_pHead;

                m_nInteractorCount = static_cast<short>(nConditions);

                m_nCollisionRearmTicks = kCollisionRearmTicks;

                return this;
            }
        }
    }

    return nullptr;
}

// The next pointer is read before each discard, because discarding frees the node.

// FUNCTION: HYPERX 0x0047b900
void PlanState::Teardown()
{
    Interactor* pNext;

    if (m_pflLocalReals != nullptr)
    {
        delete[] m_pflLocalReals;

        m_pflLocalReals   = nullptr;
        m_nLocalRealCount = 0;
    }

    Interactor* pNode = m_pCollInteractors;

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

    m_pMsgInteractors  = nullptr;
    m_nInteractorCount = 0;

    if (m_pOwner != nullptr)
    {
        AgendaItem* pObserver;
        AgendaItem* pNextObserver;

        pObserver = m_pOwner->GetObserver();

        while (pObserver != nullptr)
        {
            pNextObserver = pObserver->m_pNext;

            pObserver->OnListChanged();

            pObserver = pNextObserver;
        }

        m_pOwner->SetObserver(nullptr);
    }

    if (m_pHead != nullptr)
    {
        // Reset cascades through the standby link, so the head alone is enough.
        m_pHead->Reset();
    }

    m_pHead    = nullptr;
    m_pCurrent = nullptr;
}

// FUNCTION: HYPERX 0x0047b990
void PlanState::Reinitialize()
{
    SetAndInit(g_aReinitPlanImage);
}

// One DECLARE_BEHAVIOR node, a leaf child of six and the string "x", then END_PLAN.
PlanWord g_aReinitPlanImage[9] = {
    { static_cast<int>(kPlanMarkerNode) }, { static_cast<int>(CATEGORY_BEHAVIOUR) | MEMBER_DECLARE_BEHAVIOR }, { 2 }, { 6 }, { static_cast<int>(kPlanMarkerString) }, { 'x' }, { 0 }, { static_cast<int>(kPlanMarkerNode) }, { static_cast<int>(CATEGORY_END_PLAN) }
};
