#ifndef HYPERBLADE_BEHOBJGLIDELCL_H
#define HYPERBLADE_BEHOBJGLIDELCL_H

#include "common/surfacebehaviorobject.h"

class EngineObject;

// VTABLE: HYPERX 0x004b266c
class BehObj_GlideLCL : public SurfaceBehaviorObject
{
public:
    ~BehObj_GlideLCL() override;
    void Init(class EngineObject* pOwner, struct PlanNode* pNode, int nFlags) override;
    BehObj_GlideLCL();
};

// SYNTHETIC: HYPERX 0x0041da60
// BehObj_GlideLCL::`scalar deleting destructor'

#endif
