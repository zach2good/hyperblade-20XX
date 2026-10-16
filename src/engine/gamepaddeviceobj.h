#ifndef HYPERBLADE_GAMEPADDEVICEOBJ_H
#define HYPERBLADE_GAMEPADDEVICEOBJ_H

#include "engine/inputdeviceobj.h"

// What a DefineObject of type Gamepad builds, object type 0x15.
// Overrides only the deleting destructor, shared with the other two device classes.
// VTABLE: HYPERX 0x004b3564
class GamepadDeviceObj : public InputDeviceObj
{
public:
    GamepadDeviceObj(int nSlot, class ObjectDef* pDef);
};

static_assert(sizeof(GamepadDeviceObj) == 0xd28);

#endif
