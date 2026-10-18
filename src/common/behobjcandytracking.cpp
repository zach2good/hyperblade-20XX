#include "common/compat.h"
#include "planvm/behaviorids.h"
#include "common/pooledclasses.h"
#include "planvm/conditiontable.h"
#include "planvm/planvmglobals.h"
#include "pooledownerroot.h"

// FUNCTION: HYPERX 0x0041eb70
BehObj_CandyTracking::BehObj_CandyTracking() = default;

// FUNCTION: HYPERX 0x0041ebc0
BehObj_CandyTracking::~BehObj_CandyTracking() = default;

// FUNCTION: HYPERX 0x0041ebd0
void BehObj_CandyTracking::Init(EngineObject* pOwner, PlanNode* pBlock, int nFlags)
{
    PurgeAgendaForCandyTracking();

    // STRING: HYPERX 0x004bc3c4
    m_pszName = "CandyTracking";

    GameBehaviorObject::Init(pOwner, pBlock, nFlags);

    m_wKind = kBehCandyTracking;

    m_apSlots[0] = AllocAgendaItem(Agenda_CCircleTrack);

    m_apSlots[1] = AllocAgendaItem(Agenda_CViewIt);
    m_apSlots[2] = nullptr;

    short nSlot = 0;
    while (m_apSlots[nSlot] != nullptr)
    {
        m_apSlots[nSlot]->Bind(m_pOwner, this);

        auto* const pNode = m_apSlots[nSlot];
        nSlot++;
        pNode->m_bPersistent = 1;
    }
}
