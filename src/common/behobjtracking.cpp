#include "common/compat.h"
#include "planvm/behaviorids.h"
#include "common/pooledclasses.h"
#include "planvm/conditiontable.h"
#include "planvm/planvmglobals.h"
#include "pooledownerroot.h"

// FUNCTION: HYPERX 0x0041e710
BehObj_Tracking::BehObj_Tracking()
{
}

// FUNCTION: HYPERX 0x0041e750
BehObj_Tracking::~BehObj_Tracking()
{
}

// FUNCTION: HYPERX 0x0041e760
void BehObj_Tracking::Init(EngineObject* pOwner, PlanNode* pBlock, int nFlags)
{
    PurgeAgendaForTracking();

    // STRING: HYPERX 0x004bc390
    m_pszName = "Tracking";

    GameBehaviorObject::Init(pOwner, pBlock, nFlags);

    m_wKind = BEH_TRACKING;

    m_apSlots[0] = AllocAgendaItem(Agenda_CTrack);

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
