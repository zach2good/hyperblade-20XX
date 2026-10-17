#ifndef HYPERBLADE_COACHOBJECT_H
#define HYPERBLADE_COACHOBJECT_H

#include "common/compat.h"
#include "engine/engineobject.h"

// Lists only the slots this class overrides; symbols/vtable_gaps.csv lists the rest.
// VTABLE: HYPERX 0x004b3d0c
class CoachObject : public EngineObject
{
public:
    CoachObject(int nSlot, class ObjectDef* pDef);
    ~CoachObject() override;
};

// SYNTHETIC: HYPERX 0x00475470
// CoachObject::`scalar deleting destructor'

#endif
