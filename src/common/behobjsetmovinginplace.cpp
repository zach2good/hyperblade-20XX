#include "common/compat.h"
#include "planvm/behaviorids.h"
#include "engine/csetattitude.h"
#include "common/pooledclasses.h"
#include "planvm/conditiontable.h"
#include "planvm/planvmglobals.h"
#include "engine/engineobject.h"
#include "engine/objectclasses.h"
#include "pooledownerroot.h"

// FUNCTION: HYPERX 0x0041e450
BehObj_SetMovingInPlace::BehObj_SetMovingInPlace() = default;

// FUNCTION: HYPERX 0x0041e4a0
BehObj_SetMovingInPlace::~BehObj_SetMovingInPlace() = default;

// FUNCTION: HYPERX 0x0041e4b0
void BehObj_SetMovingInPlace::Init(EngineObject* pOwner, PlanNode* pBlock, int nFlags)
{
    PurgeAgendaForSetMovingInPlace();

    // STRING: HYPERX 0x004bc360
    m_pszName = "SetMovingInPlace";

    m_wKind = kBehSetMovingInPlace;

    m_apSlots[0] = AllocAgendaItem(Agenda_CStepAnimation);

    m_apSlots[1] = AllocAgendaItem(Agenda_CSetAttitude);

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

    if (m_pOwner->GetSlot() == kGoalieSlotA || m_pOwner->GetSlot() == kGoalieSlotB)
    {
        ((CSetAttitude*)m_apSlots[1])->m_bLimitPitch = 1;
    }

    GameBehaviorObject::Init(pOwner, pBlock, nFlags);
}
