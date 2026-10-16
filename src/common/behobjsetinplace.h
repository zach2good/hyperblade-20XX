#ifndef HYPERBLADE_BEHOBJSETINPLACE_H
#define HYPERBLADE_BEHOBJSETINPLACE_H

#include "engine/inplacebehaviorobject.h"

class EngineObject;

// VTABLE: HYPERX 0x004b2714
class BehObj_SetInPlace : public InPlaceBehaviorObject
{
public:
    virtual ~BehObj_SetInPlace();
    virtual void Init(class EngineObject* pOwner, struct PlanNode* pNode, int nFlags);
    BehObj_SetInPlace();
};

// SYNTHETIC: HYPERX 0x0041e200
// BehObj_SetInPlace::`scalar deleting destructor'

#endif
