#include "common/compat.h"
#include "common/pooledclasses.h"
#include "physics/physicsbody.h"

// FUNCTION: HYPERX 0x0041e470
void FlatBehaviorObject::StartMotion()
{
    auto* const pBody = m_pBody;

    pBody->SetFlatOnTerrain(&pBody->pos);
}
