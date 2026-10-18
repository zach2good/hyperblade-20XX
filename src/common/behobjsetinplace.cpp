#include "common/compat.h"
#include "planvm/behaviorids.h"
#include "common/pooledclasses.h"

// FUNCTION: HYPERX 0x0041e1e0
BehObj_SetInPlace::BehObj_SetInPlace() = default;

// FUNCTION: HYPERX 0x0041e220
BehObj_SetInPlace::~BehObj_SetInPlace() = default;

// The first slot is nulled and then read back, so the walk never runs.
// FUNCTION: HYPERX 0x0041e230
void BehObj_SetInPlace::Init(EngineObject* pOwner, PlanNode* pBlock, int nFlags)
{
    FillAgendaPurged();

    // STRING: HYPERX 0x004bc334
    m_pszName = "SetInPlace";

    GameBehaviorObject::Init(pOwner, pBlock, nFlags);

    m_wKind = BEH_SET_IN_PLACE;

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
