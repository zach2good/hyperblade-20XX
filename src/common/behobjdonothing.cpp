#include "common/compat.h"
#include "planvm/behaviorids.h"
#include "common/pooledclasses.h"

// FUNCTION: HYPERX 0x0041e560
BehObj_DoNothing::BehObj_DoNothing() = default;

// FUNCTION: HYPERX 0x0041e5a0
BehObj_DoNothing::~BehObj_DoNothing() = default;

// FUNCTION: HYPERX 0x0041e5b0
void BehObj_DoNothing::Init(EngineObject* pOwner, PlanNode* pBlock, int nFlags)
{
    FillAgendaPurged();

    // STRING: HYPERX 0x004bc374
    m_pszName = "DoNothing";

    GameBehaviorObject::Init(pOwner, pBlock, nFlags);

    m_wKind      = BEH_DO_NOTHING;
    m_apSlots[0] = nullptr;
}
