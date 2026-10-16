#ifndef HYPERBLADE_BEHOBJGLIDE_H
#define HYPERBLADE_BEHOBJGLIDE_H

#include "common/surfacebehaviorobject.h"

class EngineObject;

// VTABLE: HYPERX 0x004b2654
class BehObj_Glide : public SurfaceBehaviorObject
{
public:
    virtual ~BehObj_Glide();
    virtual void Init(class EngineObject* pOwner, struct PlanNode* pNode, int nFlags);
    BehObj_Glide();
};

// SYNTHETIC: HYPERX 0x0041d980
// BehObj_Glide::`scalar deleting destructor'

#endif
