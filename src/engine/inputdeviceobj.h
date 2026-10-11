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

// No device class declares a destructor, so each deleter calls an inline copy of DeviceObject's.
// SYNTHETIC: HYPERX 0x0045d690
// GamepadDeviceObj::`scalar deleting destructor'

// SYNTHETIC: HYPERX 0x0045d6b0
// GamepadDeviceObj::~GamepadDeviceObj

#endif
