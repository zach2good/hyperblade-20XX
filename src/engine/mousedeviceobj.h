#ifndef HYPERBLADE_MOUSEDEVICEOBJ_H
#define HYPERBLADE_MOUSEDEVICEOBJ_H

#include "engine/inputdeviceobj.h"

// What a DefineObject of type Mouse builds, object type 0x17.
// Overrides only the deleting destructor, shared with the other two device classes.
// VTABLE: HYPERX 0x004b3664
class MouseDeviceObj : public InputDeviceObj
{
public:
    MouseDeviceObj(int nSlot, class ObjectDef* pDef);
};

#endif
