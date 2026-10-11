#ifndef HYPERBLADE_BEHOBJGLIDELCL_H
#define HYPERBLADE_BEHOBJGLIDELCL_H

#include "common/surfacebehaviorobject.h"

class EngineObject;

// VTABLE: HYPERX 0x004b266c
class BehObj_GlideLCL : public SurfaceBehaviorObject
{
public:
    virtual ~BehObj_GlideLCL();
    virtual void Init(class EngineObject* pOwner, struct PlanNode* pNode, int nFlags);
    BehObj_GlideLCL();
};

// SYNTHETIC: HYPERX 0x0041da60
// BehObj_GlideLCL::`scalar deleting destructor'

STATIC_ASSERT(sizeof(BehObj_GlideLCL) == 0x98);

#endif
