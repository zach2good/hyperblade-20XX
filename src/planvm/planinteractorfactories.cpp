#include "common/compat.h"
#include "globals.h"
#include "common/pooledownerroot.h"
#include "anim/animbinding.h"
#include "collinteractor.h"
#include "msginteractor.h"
#include "engine/gameengine.h"
#include "gamebehaviorobject.h"
#include "planstate.h"
#include "planvm/planinteractorfactories.h"

// FUNCTION: HYPERX 0x0045d810
void PlanInteractorFactories::ResetAll()
{
    g_pCollInteractorPool->Init();

    g_pMsgInteractorPool->Init();
}

// FUNCTION: HYPERX 0x0045d830
void PlanInteractorFactories::DestroyAll()
{
    if (g_pCollInteractorPool != nullptr)
    {
        g_pCollInteractorPool = g_pCollInteractorPool->Destroy();
    }

    if (g_pMsgInteractorPool != nullptr)
    {
        g_pMsgInteractorPool = g_pMsgInteractorPool->Destroy();
    }
}

// Links after the previous interactor, else heads the behaviour's chain, else the plan state's.
// FUNCTION: HYPERX 0x0045dca0
CollInteractor* PlanInteractorFactories::CreateCollInteractor(EngineObject* pObject, PlanNode* pInstr, GameBehaviorObject* pNode, Interactor* pPrevious)
{
    CollInteractor* pInteractor;

    if (pNode == nullptr)
    {
        pObject->GetPlanState();
    }

    pInteractor = (static_cast<CollInteractor*>(g_pCollInteractorPool->Alloc()))->Setup(pObject, pInstr);

    if (pPrevious == nullptr)
    {
        if (pNode != nullptr)
        {
            pNode->m_pCollInteractors = pInteractor;

            return pInteractor;
        }

        pObject->GetPlanState()->m_pCollInteractors = pInteractor;

        return pInteractor;
    }

    pPrevious->pNext = pInteractor;

    return pInteractor;
}

// Links after the previous interactor, else heads the behaviour's chain, else the plan state's.
// FUNCTION: HYPERX 0x0045df60
MsgInteractor* PlanInteractorFactories::CreateMsgInteractor(EngineObject* pObject, PlanNode* pInstr, GameBehaviorObject* pNode, Interactor* pPrevious)
{
    MsgInteractor* pInteractor;

    if (pNode == nullptr)
    {
        pObject->GetPlanState();
    }

    pInteractor = (static_cast<MsgInteractor*>(g_pMsgInteractorPool->Alloc()))->Setup(pObject, pInstr);

    if (pPrevious == nullptr)
    {
        if (pNode != nullptr)
        {
            pNode->m_pMsgInteractors = pInteractor;

            return pInteractor;
        }

        pObject->GetPlanState()->m_pMsgInteractors = pInteractor;

        return pInteractor;
    }

    pPrevious->pNext = pInteractor;

    return pInteractor;
}
