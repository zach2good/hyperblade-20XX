#ifndef HYPERBLADE_DEVICEOBJECT_H
#define HYPERBLADE_DEVICEOBJECT_H

#include "common/compat.h"
#include "engine/engineobject.h"

class EngineObject;

// The image carries this destructor pair twice; GamepadDeviceObj's implicit pair is the other.
// VTABLE: HYPERX 0x004b3464
class DeviceObject : public EngineObject
{
public:
    DeviceObject(int nSlot, class ObjectDef* pDef);

    // FUNCTION: HYPERX 0x0045d700
    ~DeviceObject()
    {
    }

    // Public: KeyboardDeviceObj's constructor writes four of these, and the bring-up key replay reads them.
public:
    struct ActionBindingGroup* m_pSlots; // 0xd14
    short                      m_nSlots; // 0xd18
    // Only ever stored to 0 and never read.
    short m_wUnused; // 0xd1a
    // Null unless the group count is positive.
    struct ActionBindingGroup* m_pBindingTable; // 0xd1c
    // The device itself at first, then an object from the slot table through PlanAction.
    class EngineObject* m_pOwner; // 0xd20
    // Where DO_ACTION posts go.
    class EngineObject* m_pEntity; // 0xd24
};

// SYNTHETIC: HYPERX 0x0045d710
// DeviceObject::`scalar deleting destructor'

static_assert(sizeof(DeviceObject) == 0xd28);

#endif
