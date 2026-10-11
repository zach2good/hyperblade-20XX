#ifndef HYPERBLADE_BEHOBJDONOTHING_H
#define HYPERBLADE_BEHOBJDONOTHING_H

#include "engine/inplacebehaviorobject.h"

class EngineObject;

// VTABLE: HYPERX 0x004b2774
class BehObj_DoNothing : public InPlaceBehaviorObject
{
public:
    virtual ~BehObj_DoNothing();
    virtual void Init(class EngineObject* pOwner, struct PlanNode* pNode, int nFlags);
    BehObj_DoNothing();
};

// SYNTHETIC: HYPERX 0x0041e580
// BehObj_DoNothing::`scalar deleting destructor'

STATIC_ASSERT(sizeof(BehObj_DoNothing) == 0x98);

#endif
