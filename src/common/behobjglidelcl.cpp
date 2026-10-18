#include "common/compat.h"
#include "planvm/behaviorids.h"
#include "common/pooledclasses.h"
#include "planvm/conditiontable.h"
#include "planvm/planvmglobals.h"
#include "planvm/cglideonsurface.h"
#include "pooledownerroot.h"

// FUNCTION: HYPERX 0x0041da40
BehObj_GlideLCL::BehObj_GlideLCL() = default;

// FUNCTION: HYPERX 0x0041da80
BehObj_GlideLCL::~BehObj_GlideLCL() = default;

// FUNCTION: HYPERX 0x0041da90
void BehObj_GlideLCL::Init(EngineObject* pOwner, PlanNode* pBlock, int nFlags)
{
    PurgeAgendaForGlide();

    // STRING: HYPERX 0x004bc2e4
    m_pszName = "GlideLCL";

    GameBehaviorObject::Init(pOwner, pBlock, nFlags);

    m_wKind = kBehGlideLcl;

    auto* const pFirst   = (CGlideOnSurface*)AllocAgendaItem(Agenda_CGlideOnSurface);
    m_apSlots[0]         = (AgendaItem*)pFirst;
    pFirst->m_nOnSurface = 1;

    m_apSlots[1] = AllocAgendaItem(Agenda_CStepAnimation);
    m_apSlots[2] = AllocAgendaItem(Agenda_CSetAttitude);

    short nSlot  = 0;
    m_apSlots[3] = nullptr;

    while (m_apSlots[nSlot] != nullptr)
    {
        m_apSlots[nSlot]->Bind(m_pOwner, this);

        auto* const pNode = m_apSlots[nSlot];
        nSlot++;
        pNode->m_bPersistent = 1;
    }
}
