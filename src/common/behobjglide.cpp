#include "common/compat.h"
#include "planvm/behaviorids.h"
#include "common/pooledclasses.h"
#include "planvm/conditiontable.h"
#include "planvm/planvmglobals.h"
#include "pooledownerroot.h"

// FUNCTION: HYPERX 0x0041d960
BehObj_Glide::BehObj_Glide() = default;

// FUNCTION: HYPERX 0x0041d9a0
BehObj_Glide::~BehObj_Glide() = default;

// FUNCTION: HYPERX 0x0041d9b0
void BehObj_Glide::Init(EngineObject* pOwner, PlanNode* pBlock, int nFlags)
{
    PurgeAgendaForGlide();

    // STRING: HYPERX 0x004bc2dc
    m_pszName = "Glide";

    GameBehaviorObject::Init(pOwner, pBlock, nFlags);

    m_wKind = kBehGlide;

    m_apSlots[0] = AllocAgendaItem(Agenda_CGlideOnSurface);

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
