#include "common/compat.h"
#include "engine/objectclasses.h"
#include "engine/dromeobject.h"

// FUNCTION: HYPERX1000 0x00472170
// FUNCTION: HYPERX1001 0x00474880
DromeObject::DromeObject(int nSlot, class ObjectDef* pDef)
: EngineObject(nSlot, pDef)
{
    m_nFlags |= ENGINEOBJECT_NO_CULL;
}

// FUNCTION: HYPERX1001 0x004748d0
DromeObject::~DromeObject()
{
}
