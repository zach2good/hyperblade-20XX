#include "common/compat.h"
#include "engine/devicector.h"

// FUNCTION: HYPERX100 0x00491200
// FUNCTION: HYPERX 0x0045d6e0
MouseDeviceObj::MouseDeviceObj(int nSlot, class ObjectDef* pDef)
: InputDeviceObj(nSlot, pDef)
{
}
