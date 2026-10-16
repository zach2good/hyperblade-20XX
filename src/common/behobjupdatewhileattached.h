#ifndef HYPERBLADE_BEHOBJUPDATEWHILEATTACHED_H
#define HYPERBLADE_BEHOBJUPDATEWHILEATTACHED_H

#include "common/flightbehaviorobject.h"

class EngineObject;

// VTABLE: HYPERX 0x004b26fc
class BehObj_UpdateWhileAttached : public FlightBehaviorObject
{
public:
    virtual ~BehObj_UpdateWhileAttached();
    virtual void Init(class EngineObject* pOwner, struct PlanNode* pNode, int nFlags);
    virtual void SetInteractorsActive(int nArg);
    BehObj_UpdateWhileAttached();
};

// SYNTHETIC: HYPERX 0x0041e0c0
// BehObj_UpdateWhileAttached::`scalar deleting destructor'

static_assert(sizeof(BehObj_UpdateWhileAttached) == 0x98);

#endif
