#ifndef HYPERBLADE_BEHOBJGLIDETOOBJECT_H
#define HYPERBLADE_BEHOBJGLIDETOOBJECT_H

#include "common/surfacebehaviorobject.h"

class EngineObject;

// VTABLE: HYPERX 0x004b25dc
class BehObj_GlideToObject : public SurfaceBehaviorObject
{
public:
    virtual ~BehObj_GlideToObject();
    virtual void Init(class EngineObject* pOwner, struct PlanNode* pNode, int nFlags);
    virtual void SendTimedMessage();
    BehObj_GlideToObject();
};

// SYNTHETIC: HYPERX 0x0041d480
// BehObj_GlideToObject::`scalar deleting destructor'

STATIC_ASSERT(sizeof(BehObj_GlideToObject) == 0x98);

#endif
