#ifndef HYPERBLADE_GOALOBJECT_H
#define HYPERBLADE_GOALOBJECT_H

#include "common/compat.h"
#include "engine/engineobject.h"

// Lists only the slots this class overrides; symbols/vtable_gaps.csv lists the rest.
// VTABLE: HYPERX 0x004b3b74
class GoalObject : public EngineObject
{
public:
    GoalObject(int nSlot, class ObjectDef* pDef);
    ~GoalObject();

private:
    int m_nUnusedE; // 0xd14
};

// SYNTHETIC: HYPERX 0x00474540
// GoalObject::`scalar deleting destructor'

#endif
