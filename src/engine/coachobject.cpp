#include "common/compat.h"
#include "coachobject.h"

// FUNCTION: HYPERX100 0x00473310
// FUNCTION: HYPERX 0x00475450
CoachObject::CoachObject(int nSlot, class ObjectDef* pDef)
: EngineObject(nSlot, pDef)
{
}

// FUNCTION: HYPERX 0x00475490
CoachObject::~CoachObject()
{
}
