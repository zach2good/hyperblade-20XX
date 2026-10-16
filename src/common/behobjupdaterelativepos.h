#ifndef HYPERBLADE_BEHOBJUPDATERELATIVEPOS_H
#define HYPERBLADE_BEHOBJUPDATERELATIVEPOS_H

#include "common/surfacebehaviorobject.h"

class EngineObject;

// VTABLE: HYPERX 0x004b263c
class BehObj_UpdateRelativePos : public SurfaceBehaviorObject
{
public:
    virtual ~BehObj_UpdateRelativePos();
    virtual void Init(class EngineObject* pOwner, struct PlanNode* pNode, int nFlags);
    BehObj_UpdateRelativePos();
};

// SYNTHETIC: HYPERX 0x0041d8b0
// BehObj_UpdateRelativePos::`scalar deleting destructor'

static_assert(sizeof(BehObj_UpdateRelativePos) == 0x98);

#endif
