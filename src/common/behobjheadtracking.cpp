#include "common/compat.h"
#include "planvm/behaviorids.h"
#include "common/pooledclasses.h"
#include "planvm/conditiontable.h"
#include "planvm/planvmglobals.h"
#include "pooledownerroot.h"

// FUNCTION: HYPERX 0x0041e7e0
BehObj_HeadTracking::BehObj_HeadTracking()
{
}

// FUNCTION: HYPERX 0x0041e820
BehObj_HeadTracking::~BehObj_HeadTracking()
{
}

// FUNCTION: HYPERX 0x0041e830
void BehObj_HeadTracking::Init(EngineObject* pOwner, PlanNode* pBlock, int nFlags)
{
    PurgeAgendaForTracking();

    // STRING: HYPERX 0x004bc390
    m_pszName = "Tracking";

    GameBehaviorObject::Init(pOwner, pBlock, nFlags);

    m_wKind = BEH_HEAD_TRACKING;

    m_apSlots[0] = AllocAgendaItem(Agenda_CHeadTrack);

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
