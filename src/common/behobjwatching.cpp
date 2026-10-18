#include "common/compat.h"
#include "planvm/behaviorids.h"
#include "common/pooledclasses.h"
#include "planvm/conditiontable.h"
#include "planvm/planvmglobals.h"
#include "pooledownerroot.h"

// FUNCTION: HYPERX 0x0041e8b0
BehObj_Watching::BehObj_Watching() = default;

// FUNCTION: HYPERX 0x0041e8f0
BehObj_Watching::~BehObj_Watching() = default;

// FUNCTION: HYPERX 0x0041e900
void BehObj_Watching::Init(EngineObject* pOwner, PlanNode* pBlock, int nFlags)
{
    PurgeAgendaForWatching();

    // STRING: HYPERX 0x004bc39c
    m_pszName = "Watch";

    GameBehaviorObject::Init(pOwner, pBlock, nFlags);

    m_wKind = BEH_WATCHING;

    m_apSlots[0] = AllocAgendaItem(Agenda_CWatch);

    m_apSlots[1] = AllocAgendaItem(Agenda_CViewIt);
    m_apSlots[2] = nullptr;

    short nSlot = 0;
    while (m_apSlots[nSlot] != nullptr)
    {
        m_apSlots[nSlot]->Bind(m_pOwner, this);

        AgendaItem* const pNode = m_apSlots[nSlot];
        nSlot++;
        pNode->m_bPersistent = 1;
    }
}
