#include "common/compat.h"
#include "planvm/behaviorids.h"
#include "common/pooledclasses.h"
#include "planvm/conditiontable.h"
#include "planvm/planvmglobals.h"
#include "engine/engineobject.h"
#include "pooledownerroot.h"

// FUNCTION: HYPERX 0x0041e980
BehObj_TrackTrajectory::BehObj_TrackTrajectory() = default;

// FUNCTION: HYPERX 0x0041e9c0
BehObj_TrackTrajectory::~BehObj_TrackTrajectory() = default;

// FUNCTION: HYPERX 0x0041e9d0
void BehObj_TrackTrajectory::Init(EngineObject* pOwner, PlanNode* pBlock, int nFlags)
{
    PurgeAgendaForTrackTrajectory();

    // STRING: HYPERX 0x004bc3a4
    m_pszName = "TrackTrajectory";

    m_wKind = kBehTrackTrajectory;

    m_apSlots[0] = AllocAgendaItem(Agenda_CTrackTraj);

    m_apSlots[1] = AllocAgendaItem(Agenda_CViewIt);

    short nSlot  = 0;
    m_apSlots[2] = nullptr;

    m_pOwner = pOwner;
    m_pBody  = pOwner->GetPhysicsBody();

    while (m_apSlots[nSlot] != nullptr)
    {
        m_apSlots[nSlot]->Bind(m_pOwner, this);

        auto* const pNode = m_apSlots[nSlot];
        nSlot++;
        pNode->m_bPersistent = 1;
    }

    GameBehaviorObject::Init(pOwner, pBlock, nFlags);
}
