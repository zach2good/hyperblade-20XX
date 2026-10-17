#ifndef HYPERBLADE_BEHOBJSETINPLACE_H
#define HYPERBLADE_BEHOBJSETINPLACE_H

#include "engine/inplacebehaviorobject.h"

class EngineObject;

// VTABLE: HYPERX 0x004b2714
class BehObj_SetInPlace : public InPlaceBehaviorObject
{
public:
    ~BehObj_SetInPlace() override;
    void Init(class EngineObject* pOwner, struct PlanNode* pNode, int nFlags) override;
    BehObj_SetInPlace();
};

// SYNTHETIC: HYPERX 0x0041e200
// BehObj_SetInPlace::`scalar deleting destructor'

#endif
