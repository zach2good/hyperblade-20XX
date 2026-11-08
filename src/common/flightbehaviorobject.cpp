#include "common/compat.h"
#include "common/pooledclasses.h"
#include "physics/physicsbody.h"

// FUNCTION: HYPERX100 0x004cb5e0
// FUNCTION: HYPERX 0x0041eb90
void FlightBehaviorObject::StartMotion()
{
    m_pBody->LaunchDiagonal();
}
