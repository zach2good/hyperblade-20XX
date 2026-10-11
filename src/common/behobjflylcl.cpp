#include "common/compat.h"
#include "planvm/behaviorids.h"
#include "common/pooledclasses.h"
#include "planvm/conditiontable.h"
#include "planvm/planvmglobals.h"
#include "engine/engineobject.h"
#include "planvm/cprojectandbounce.h"
#include "pooledownerroot.h"

// FUNCTION: HYPERX 0x0041dd40
BehObj_FlyLCL::BehObj_FlyLCL()
{
}

// FUNCTION: HYPERX 0x0041dd80
BehObj_FlyLCL::~BehObj_FlyLCL()
{
}

// FUNCTION: HYPERX 0x0041dd90
void BehObj_FlyLCL::Init(EngineObject* pOwner, PlanNode* pBlock, int nFlags)
{
    PurgeAgendaForFly();

    // STRING: HYPERX 0x004bc300
    m_pszName = "FlyLCL";

    GameBehaviorObject::Init(pOwner, pBlock, nFlags);

    m_wKind = BEH_FLY_LCL;

    CProjectAndBounce* const pFirst = (CProjectAndBounce*)AllocAgendaItem(Agenda_CProjectAndBounce);
    m_apSlots[0]                    = (AgendaItem*)pFirst;
    pFirst->m_nMode                 = 1;

    m_apSlots[1] = AllocAgendaItem(Agenda_CStepAnimation);
    m_apSlots[2] = 0;

    short nSlot = 0;
    while (m_apSlots[nSlot] != 0)
    {
        m_apSlots[nSlot]->Bind(m_pOwner, this);

        AgendaItem* const pNode = m_apSlots[nSlot];
        nSlot++;
        pNode->m_bPersistent = 1;
    }
}
