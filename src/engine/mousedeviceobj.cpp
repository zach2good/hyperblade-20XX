#include "common/compat.h"
#include "engine/devicector.h"

// FUNCTION: HYPERX1000 0x00491200
// FUNCTION: HYPERX1001 0x0045d6e0
MouseDeviceObj::MouseDeviceObj(int nSlot, class ObjectDef* pDef)
: InputDeviceObj(nSlot, pDef)
{
}

// The linker folds this body onto GamepadDeviceObj::~GamepadDeviceObj, which carries the marker.
// FUNCTION: HYPERX1000 0x004912c0
MouseDeviceObj::~MouseDeviceObj()
{
}
