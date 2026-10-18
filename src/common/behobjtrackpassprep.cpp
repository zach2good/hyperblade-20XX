#include "common/compat.h"
#include "planvm/behaviorids.h"
#include "common/pooledclasses.h"
#include "planvm/conditiontable.h"
#include "planvm/planvmglobals.h"
#include "pooledownerroot.h"

// FUNCTION: HYPERX 0x0041ea60
BehObj_TrackPassPrep::BehObj_TrackPassPrep()
{
}

// FUNCTION: HYPERX 0x0041eaa0
BehObj_TrackPassPrep::~BehObj_TrackPassPrep()
{
}

// FUNCTION: HYPERX 0x0041eab0
void BehObj_TrackPassPrep::Init(EngineObject* pOwner, PlanNode* pBlock, int nFlags)
{
    PurgeAgendaForTrackPassPrep();

    // STRING: HYPERX 0x004bc3b4
    m_pszName = "TrackPassPrep";

    GameBehaviorObject::Init(pOwner, pBlock, nFlags);

    m_wKind = BEH_TRACK_PASS_PREP;

    m_apSlots[0] = AllocAgendaItem(Agenda_CTrackPasser_Receiver);

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
