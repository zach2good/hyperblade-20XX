#include "common/compat.h"
#include "planvm/behaviorids.h"
#include "common/pooledclasses.h"
#include "planvm/conditiontable.h"
#include "planvm/planvmglobals.h"
#include "pooledownerroot.h"

// FUNCTION: HYPERX 0x0041de20
BehObj_FlyToward::BehObj_FlyToward() = default;

// FUNCTION: HYPERX 0x0041de60
BehObj_FlyToward::~BehObj_FlyToward() = default;

// FUNCTION: HYPERX 0x0041de70
void BehObj_FlyToward::Init(EngineObject* pOwner, PlanNode* pBlock, int nFlags)
{
    PurgeAgendaForFly();

    // STRING: HYPERX 0x004bc308
    m_pszName = "FlyToward";

    GameBehaviorObject::Init(pOwner, pBlock, nFlags);

    m_wKind = kBehFlyToward;

    m_apSlots[0] = AllocAgendaItem(Agenda_CSteerToward);

    m_apSlots[1] = AllocAgendaItem(Agenda_CStepAnimation);
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
