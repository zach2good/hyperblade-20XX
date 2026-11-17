#include "common/compat.h"
#include "coachobject.h"

// FUNCTION: HYPERX1001 0x00475450
CoachObject::CoachObject(int nSlot, class ObjectDef* pDef)
: EngineObject(nSlot, pDef)
{
}

// FUNCTION: HYPERX1001 0x00475490
CoachObject::~CoachObject() = default;
