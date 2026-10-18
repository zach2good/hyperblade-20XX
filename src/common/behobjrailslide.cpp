#include "common/compat.h"
#include "planvm/behaviorids.h"
#include "common/pooledclasses.h"
#include "planvm/conditiontable.h"
#include "planvm/planvmglobals.h"
#include "pooledownerroot.h"

// FUNCTION: HYPERX 0x0041db30
BehObj_RailSlide::BehObj_RailSlide() = default;

// FUNCTION: HYPERX 0x0041db70
BehObj_RailSlide::~BehObj_RailSlide() = default;

// FUNCTION: HYPERX 0x0041db80
void BehObj_RailSlide::Init(EngineObject* pOwner, PlanNode* pBlock, int nFlags)
{
    PurgeAgendaForRailSlide();

    // STRING: HYPERX 0x004bc2f0
    m_pszName = "RailSlide";

    GameBehaviorObject::Init(pOwner, pBlock, nFlags);

    m_wKind = BEH_RAIL_SLIDE;

    m_apSlots[0] = AllocAgendaItem(Agenda_CSlideOnRail);

    m_apSlots[1] = AllocAgendaItem(Agenda_CStepAnimation);
    m_apSlots[2] = AllocAgendaItem(Agenda_CSetAttitude);

    short nSlot  = 0;
    m_apSlots[3] = nullptr;

    while (m_apSlots[nSlot] != nullptr)
    {
        m_apSlots[nSlot]->Bind(m_pOwner, this);

        AgendaItem* const pNode = m_apSlots[nSlot];
        nSlot++;
        pNode->m_bPersistent = 1;
    }
}
