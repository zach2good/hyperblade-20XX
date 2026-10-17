#ifndef HYPERBLADE_BEHOBJUPDATERELATIVEPOS_H
#define HYPERBLADE_BEHOBJUPDATERELATIVEPOS_H

#include "common/surfacebehaviorobject.h"

class EngineObject;

// VTABLE: HYPERX 0x004b263c
class BehObj_UpdateRelativePos : public SurfaceBehaviorObject
{
public:
    ~BehObj_UpdateRelativePos() override;
    void Init(class EngineObject* pOwner, struct PlanNode* pNode, int nFlags) override;
    BehObj_UpdateRelativePos();
};

// SYNTHETIC: HYPERX 0x0041d8b0
// BehObj_UpdateRelativePos::`scalar deleting destructor'

#endif
