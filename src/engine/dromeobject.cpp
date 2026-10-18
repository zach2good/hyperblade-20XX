#include "common/compat.h"
#include "engine/objectclasses.h"
#include "engine/dromeobject.h"

// FUNCTION: HYPERX 0x00474880
DromeObject::DromeObject(int nSlot, class ObjectDef* pDef)
: EngineObject(nSlot, pDef)
{
    m_nFlags |= kEngineObjectNoCull;
}

// FUNCTION: HYPERX 0x004748d0
DromeObject::~DromeObject() = default;
