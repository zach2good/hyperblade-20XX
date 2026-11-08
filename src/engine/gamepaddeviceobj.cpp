#include "common/compat.h"
#include "engine/devicector.h"

// FUNCTION: HYPERX100 0x00490f80
// FUNCTION: HYPERX 0x0045d670
GamepadDeviceObj::GamepadDeviceObj(int nSlot, class ObjectDef* pDef)
: InputDeviceObj(nSlot, pDef)
{
}
