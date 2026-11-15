#ifndef HYPERBLADE_INPUTDEVICEOBJ_H
#define HYPERBLADE_INPUTDEVICEOBJ_H

#include "input/deviceobject.h"

// Stores no table of its own.
class InputDeviceObj : public DeviceObject
{
public:
    InputDeviceObj(int nSlot, class ObjectDef* pDef)
    : DeviceObject(nSlot, pDef)
    {
    }
};

// Each leaf's deleter calls its out-of-line destructor, so the three fold onto Gamepad's.
// SYNTHETIC: HYPERX 0x0045d690
// GamepadDeviceObj::`scalar deleting destructor'

#endif
