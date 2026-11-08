// The in-place behaviour's start, which EngineObject's surface setup calls.

#include "common/compat.h"
#include "common/pooledclasses.h"
#include "physics/physicsbody.h"

// FUNCTION: HYPERX100 0x004cc1a0
// FUNCTION: HYPERX 0x0041e620
void InPlaceBehaviorObject::StartMotion()
{
    m_pBody->ResetSpeeds();
}
