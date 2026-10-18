#include "common/compat.h"
#include "planvm/behaviorids.h"
#include "common/pooledclasses.h"
#include "planvm/collinteractor.h"
#include "planvm/msginteractor.h"
#include "physics/physicsbody.h"
#include "globals.h"
#include "pooledownerroot.h"

// FUNCTION: HYPERX 0x0041e0a0
BehObj_UpdateWhileAttached::BehObj_UpdateWhileAttached()
{
}

// FUNCTION: HYPERX 0x0041e0e0
BehObj_UpdateWhileAttached::~BehObj_UpdateWhileAttached()
{
}

// Unlike the base, it sets the collision interactor aimed at the attached object back to zero.
// FUNCTION: HYPERX 0x0041e0f0
void BehObj_UpdateWhileAttached::SetInteractorsActive(int nArg)
{
    Interactor* pFirst  = m_pCollInteractors;
    Interactor* pSecond = m_pMsgInteractors;

    while (pFirst != nullptr)
    {
        if (pFirst->GetActiveState() != -1)
        {
            pFirst->SetActiveState(nArg);

            AttachPak* const pBinding = m_pBody->pAttach;

            if (pBinding != nullptr)
            {
                const short nBound = pBinding->nIndex;

                if (nBound == pFirst->GetOtherSlot())
                {
                    pFirst->SetActiveState(0);
                }
            }
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

// FUNCTION: HYPERX 0x0041e170
void BehObj_UpdateWhileAttached::Init(EngineObject* pOwner, PlanNode* pBlock, int nFlags)
{
    PurgeAgendaForUpdateWhileAttached();

    // STRING: HYPERX 0x004bc324
    m_pszName = "UpdateAttached";

    GameBehaviorObject::Init(pOwner, pBlock, nFlags);

    m_wKind = BEH_UPDATE_WHILE_ATTACHED;

    AgendaItem* pNode = m_apSlots[0] = AllocAgendaItem(Agenda_CBeAttached);

    short nSlot = 0;

    m_apSlots[1] = nullptr;

    while (pNode != nullptr)
    {
        pNode->Bind(m_pOwner, this);

        pNode = m_apSlots[nSlot];
        nSlot++;
        pNode->m_bPersistent = 1;

        pNode = m_apSlots[nSlot];
    }
}
