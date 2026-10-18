#include "common/compat.h"
#include "planvm/behaviorids.h"
#include "common/pooledclasses.h"
#include "planvm/conditiontable.h"
#include "engine/engineobject.h"
#include "pooledownerroot.h"

// FUNCTION: HYPERX 0x0041d890
BehObj_UpdateRelativePos::BehObj_UpdateRelativePos() = default;

// FUNCTION: HYPERX 0x0041d8d0
BehObj_UpdateRelativePos::~BehObj_UpdateRelativePos() = default;

// FUNCTION: HYPERX 0x0041d8e0
void BehObj_UpdateRelativePos::Init(EngineObject* pOwner, PlanNode* pBlock, int nFlags)
{
    FillAgendaPurged();

    // STRING: HYPERX 0x004bc2c8
    m_pszName = "UpdateRelativePos";

    m_wKind = kBehUpdateRelativePos;

    m_apSlots[0] = AllocAgendaItem(Agenda_CSetPositionRelative);
    m_apSlots[1] = nullptr;

    m_pOwner = pOwner;
    m_pBody  = pOwner->GetPhysicsBody();

    m_apSlots[0]->Bind(m_pOwner, this);

    m_apSlots[0]->m_bPersistent = 1;

    GameBehaviorObject::Init(pOwner, pBlock, nFlags);
}
