#ifndef HYPERBLADE_BEHOBJSETMOVINGINPLACE_H
#define HYPERBLADE_BEHOBJSETMOVINGINPLACE_H

#include "common/flatbehaviorobject.h"

class EngineObject;

// VTABLE: HYPERX 0x004b275c
class BehObj_SetMovingInPlace : public FlatBehaviorObject
{
public:
    virtual ~BehObj_SetMovingInPlace();
    virtual void Init(class EngineObject* pOwner, struct PlanNode* pNode, int nFlags);
    BehObj_SetMovingInPlace();
};

// SYNTHETIC: HYPERX 0x0041e480
// BehObj_SetMovingInPlace::`scalar deleting destructor'

static_assert(sizeof(BehObj_SetMovingInPlace) == 0x98);

#endif
