#include "common/compat.h"
#include "engine/devicector.h"

// FUNCTION: HYPERX 0x0045d6c0
JoystickDeviceObj::JoystickDeviceObj(int nSlot, class ObjectDef* pDef)
: InputDeviceObj(nSlot, pDef)
{
}
