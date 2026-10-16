#ifndef HYPERBLADE_JOYSTICKDEVICEOBJ_H
#define HYPERBLADE_JOYSTICKDEVICEOBJ_H

#include "engine/inputdeviceobj.h"

// What a DefineObject of type Joystick builds, object type 0x16.
// Overrides only the deleting destructor, shared with the other two device classes.
// VTABLE: HYPERX 0x004b35e4
class JoystickDeviceObj : public InputDeviceObj
{
public:
    JoystickDeviceObj(int nSlot, class ObjectDef* pDef);
};

static_assert(sizeof(JoystickDeviceObj) == 0xd28);

#endif
