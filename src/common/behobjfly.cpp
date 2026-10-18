#include "common/compat.h"
#include "planvm/behaviorids.h"
#include "common/pooledclasses.h"
#include "planvm/conditiontable.h"
#include "planvm/planvmglobals.h"
#include "engine/engineobject.h"
#include "pooledownerroot.h"

// FUNCTION: HYPERX 0x0041dc10
BehObj_Fly::BehObj_Fly() = default;

// FUNCTION: HYPERX 0x0041dc50
BehObj_Fly::~BehObj_Fly() = default;

// FUNCTION: HYPERX 0x0041dc60
void BehObj_Fly::Init(EngineObject* pOwner, PlanNode* pBlock, int nFlags)
{
    PurgeAgendaForFly();

    // STRING: HYPERX 0x004bc2fc
    m_pszName = "Fly";

    GameBehaviorObject::Init(pOwner, pBlock, nFlags);

    m_wKind = BEH_FLY;

    m_apSlots[0] = AllocAgendaItem(Agenda_CProjectAndBounce);

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
