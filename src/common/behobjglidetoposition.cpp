#include "common/compat.h"
#include "planvm/behaviorids.h"
#include "common/pooledclasses.h"
#include "planvm/conditiontable.h"
#include "planvm/planvmglobals.h"
#include "engine/engineobject.h"
#include "pooledownerroot.h"

// FUNCTION: HYPERX 0x0041d590
BehObj_GlideToPosition::BehObj_GlideToPosition()
{
}

// FUNCTION: HYPERX 0x0041d5d0
BehObj_GlideToPosition::~BehObj_GlideToPosition()
{
}

// FUNCTION: HYPERX 0x0041d5e0
void BehObj_GlideToPosition::Init(EngineObject* pOwner, PlanNode* pBlock, int nFlags)
{
    PurgeAgendaForGlideToPosition();

    // STRING: HYPERX 0x004bc298
    m_pszName = "GlideToPosition";

    m_wKind = BEH_GLIDE_TO_POSITION;

    m_apSlots[0] = AllocAgendaItem(Agenda_CAdjustAcceleration);

    m_apSlots[1] = AllocAgendaItem(Agenda_CSteerToPosition);
    m_apSlots[2] = AllocAgendaItem(Agenda_CGlideOnSurface);
    m_apSlots[3] = AllocAgendaItem(Agenda_CStepAnimation);
    m_apSlots[4] = AllocAgendaItem(Agenda_CSetAttitude);

    short nSlot  = 0;
    m_apSlots[5] = 0;

    m_pOwner = pOwner;
    m_pBody  = pOwner->GetPhysicsBody();

    while (m_apSlots[nSlot] != 0)
    {
        m_apSlots[nSlot]->Bind(m_pOwner, this);

        AgendaItem* const pNode = m_apSlots[nSlot];
        nSlot++;
        pNode->m_bPersistent = 1;
    }

    GameBehaviorObject::Init(pOwner, pBlock, nFlags);
}
