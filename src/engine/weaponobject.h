#ifndef HYPERBLADE_WEAPONOBJECT_H
#define HYPERBLADE_WEAPONOBJECT_H

#include "common/compat.h"
#include "engine/engineobject.h"

// Lists only the slots this class overrides; symbols/vtable_gaps.csv lists the rest.
// VTABLE: HYPERX 0x004b3a74
class WeaponObject : public EngineObject
{
public:
    WeaponObject(int nSlot, class ObjectDef* pDef);
    ~WeaponObject();

    // Slot 54 is "Boom" and 55 is "Swarf"; the Rok keeps the same pair at the same offsets.
    int m_nAttachedSlot;     // 0xd14
    int m_nPrevAttachedSlot; // 0xd18
};

// SYNTHETIC: HYPERX 0x00473f10
// WeaponObject::`scalar deleting destructor'

#endif
