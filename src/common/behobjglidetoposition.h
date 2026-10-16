#ifndef HYPERBLADE_BEHOBJGLIDETOPOSITION_H
#define HYPERBLADE_BEHOBJGLIDETOPOSITION_H

#include "common/surfacebehaviorobject.h"
#include "math/vector.h"

class EngineObject;

// VTABLE: HYPERX 0x004b25f4
class BehObj_GlideToPosition : public SurfaceBehaviorObject
{
public:
    virtual ~BehObj_GlideToPosition();
    virtual void Init(class EngineObject* pOwner, struct PlanNode* pNode, int nFlags);
    BehObj_GlideToPosition();

    // The spot BehParam writes when a plan names a point on the rink.
    Vec3 m_vTarget; // 0x98
};

// SYNTHETIC: HYPERX 0x0041d5b0
// BehObj_GlideToPosition::`scalar deleting destructor'

#endif
