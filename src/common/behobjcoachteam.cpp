#include "common/compat.h"
#include "planvm/behaviorids.h"
#include "common/pooledclasses.h"

// FUNCTION: HYPERX 0x0041e2b0
BehObj_CoachTeam::BehObj_CoachTeam()
{
}

// FUNCTION: HYPERX 0x0041e2f0
BehObj_CoachTeam::~BehObj_CoachTeam()
{
}

// FUNCTION: HYPERX 0x0041e300
void BehObj_CoachTeam::Init(EngineObject* pOwner, PlanNode* pBlock, int nFlags)
{
    FillAgendaPurged();

    // STRING: HYPERX 0x004bc340
    m_pszName = "CoachTheTeam";

    GameBehaviorObject::Init(pOwner, pBlock, nFlags);

    m_wKind = BEH_COACH_TEAM;

    m_apSlots[0] = nullptr;
    short nSlot  = 0;

    while (m_apSlots[nSlot] != nullptr)
    {
        m_apSlots[nSlot]->Bind(m_pOwner, this);

        AgendaItem* const pNode = m_apSlots[nSlot];
        nSlot++;
        pNode->m_bPersistent = 1;
    }
}
