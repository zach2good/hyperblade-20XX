#ifndef HYPERBLADE_BEHOBJDONOTHING_H
#define HYPERBLADE_BEHOBJDONOTHING_H

#include "engine/inplacebehaviorobject.h"

class EngineObject;

// VTABLE: HYPERX 0x004b2774
class BehObj_DoNothing : public InPlaceBehaviorObject
{
public:
    ~BehObj_DoNothing() override;
    void Init(class EngineObject* pOwner, struct PlanNode* pNode, int nFlags) override;
    BehObj_DoNothing();
};

// SYNTHETIC: HYPERX 0x0041e580
// BehObj_DoNothing::`scalar deleting destructor'

#endif
