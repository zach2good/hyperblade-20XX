#ifndef HYPERBLADE_BEHOBJSETFLATINPLACE_H
#define HYPERBLADE_BEHOBJSETFLATINPLACE_H

#include "common/flatbehaviorobject.h"

class EngineObject;

// VTABLE: HYPERX 0x004b2744
class BehObj_SetFlatInPlace : public FlatBehaviorObject
{
public:
    ~BehObj_SetFlatInPlace() override;
    void Init(class EngineObject* pOwner, struct PlanNode* pNode, int nFlags) override;
    BehObj_SetFlatInPlace();
};

// SYNTHETIC: HYPERX 0x0041e3a0
// BehObj_SetFlatInPlace::`scalar deleting destructor'

#endif
