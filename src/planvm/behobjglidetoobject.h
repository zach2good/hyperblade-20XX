#ifndef HYPERBLADE_BEHOBJGLIDETOOBJECT_H
#define HYPERBLADE_BEHOBJGLIDETOOBJECT_H

#include "common/surfacebehaviorobject.h"

class EngineObject;

// VTABLE: HYPERX 0x004b25dc
class BehObj_GlideToObject : public SurfaceBehaviorObject
{
public:
    ~BehObj_GlideToObject() override;
    void Init(class EngineObject* pOwner, struct PlanNode* pNode, int nFlags) override;
    void SendTimedMessage() override;
    BehObj_GlideToObject();
};

// SYNTHETIC: HYPERX 0x0041d480
// BehObj_GlideToObject::`scalar deleting destructor'

#endif
