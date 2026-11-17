#include "common/compat.h"
#include "common/pooledclasses.h"
#include "physics/physicsbody.h"

// FUNCTION: HYPERX1001 0x0041eb90
void FlightBehaviorObject::StartMotion()
{
    m_pBody->LaunchDiagonal();
}
