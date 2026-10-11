#ifndef HYPERBLADE_BEHOBJSETFLATINPLACE_H
#define HYPERBLADE_BEHOBJSETFLATINPLACE_H

#include "common/flatbehaviorobject.h"

class EngineObject;

// VTABLE: HYPERX 0x004b2744
class BehObj_SetFlatInPlace : public FlatBehaviorObject
{
public:
    virtual ~BehObj_SetFlatInPlace();
    virtual void Init(class EngineObject* pOwner, struct PlanNode* pNode, int nFlags);
    BehObj_SetFlatInPlace();
};

// SYNTHETIC: HYPERX 0x0041e3a0
// BehObj_SetFlatInPlace::`scalar deleting destructor'

STATIC_ASSERT(sizeof(BehObj_SetFlatInPlace) == 0x98);

#endif
