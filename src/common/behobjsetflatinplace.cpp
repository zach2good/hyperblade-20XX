#include "common/compat.h"
#include "planvm/behaviorids.h"
#include "common/pooledclasses.h"

// FUNCTION: HYPERX 0x0041e380
BehObj_SetFlatInPlace::BehObj_SetFlatInPlace() = default;

// FUNCTION: HYPERX 0x0041e3c0
BehObj_SetFlatInPlace::~BehObj_SetFlatInPlace() = default;

// FUNCTION: HYPERX 0x0041e3d0
void BehObj_SetFlatInPlace::Init(EngineObject* pOwner, PlanNode* pBlock, int nFlags)
{
    FillAgendaPurged();

    // STRING: HYPERX 0x004bc350
    m_pszName = "SetFlatInPlace";

    GameBehaviorObject::Init(pOwner, pBlock, nFlags);

    m_wKind = kBehSetFlatInPlace;

    m_apSlots[0] = nullptr;
    short nSlot  = 0;

    while (m_apSlots[nSlot] != nullptr)
    {
        m_apSlots[nSlot]->Bind(m_pOwner, this);

        auto* const pNode = m_apSlots[nSlot];
        nSlot++;
        pNode->m_bPersistent = 1;
    }
}
