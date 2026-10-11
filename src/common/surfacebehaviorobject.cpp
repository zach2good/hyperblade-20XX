#include "common/compat.h"
#include "common/pooledclasses.h"
#include "physics/physicsbody.h"

// FUNCTION: HYPERX 0x0041df10
void SurfaceBehaviorObject::StartMotion()
{
    m_pBody->ResetOnSurface();
}
