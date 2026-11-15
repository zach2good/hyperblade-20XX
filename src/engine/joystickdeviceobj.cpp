#include "common/compat.h"
#include "engine/devicector.h"

// FUNCTION: HYPERX100 0x004910c0
// FUNCTION: HYPERX 0x0045d6c0
JoystickDeviceObj::JoystickDeviceObj(int nSlot, class ObjectDef* pDef)
: InputDeviceObj(nSlot, pDef)
{
}

// The linker folds this body onto GamepadDeviceObj::~GamepadDeviceObj, which carries the marker.
// FUNCTION: HYPERX100 0x00491180
JoystickDeviceObj::~JoystickDeviceObj()
{
}
