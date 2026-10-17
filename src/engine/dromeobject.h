#ifndef HYPERBLADE_DROMEOBJECT_H
#define HYPERBLADE_DROMEOBJECT_H

#include "common/compat.h"
#include "engine/engineobject.h"
#include "engine/weaponobject.h"

// Lists only the slots this class overrides; symbols/vtable_gaps.csv lists the rest.
// VTABLE: HYPERX 0x004b3bf4
class DromeObject : public EngineObject
{
public:
    DromeObject(int nSlot, class ObjectDef* pDef);
    ~DromeObject() override;
};

// SYNTHETIC: HYPERX 0x004748b0
// DromeObject::`scalar deleting destructor'

#endif
