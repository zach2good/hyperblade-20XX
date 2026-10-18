#include "common/compat.h"
#include "planvm/behaviorids.h"
#include "common/pooledclasses.h"
#include "planvm/conditiontable.h"
#include "engine/engineobject.h"
#include "pooledownerroot.h"

// FUNCTION: HYPERX 0x0041d7b0
BehObj_Avoidance::BehObj_Avoidance() = default;

// FUNCTION: HYPERX 0x0041d7f0
BehObj_Avoidance::~BehObj_Avoidance() = default;

// FUNCTION: HYPERX 0x0041d800
void BehObj_Avoidance::Init(EngineObject* pOwner, PlanNode* pBlock, int nFlags)
{
    FillAgendaPurged();

    // STRING: HYPERX 0x004bc2bc
    m_pszName = "Avoidance";

    m_wKind = BEH_AVOIDANCE;

    m_apSlots[0] = AllocAgendaItem(Agenda_CAvoidObjects);

    short nSlot  = 0;
    m_apSlots[2] = nullptr;

    m_pOwner = pOwner;
    m_pBody  = pOwner->GetPhysicsBody();

    while (m_apSlots[nSlot] != nullptr)
    {
        m_apSlots[nSlot]->Bind(m_pOwner, this);

        AgendaItem* const pNode = m_apSlots[nSlot];
        nSlot++;
        pNode->m_bPersistent = 1;
    }

    GameBehaviorObject::Init(pOwner, pBlock, nFlags);
}
