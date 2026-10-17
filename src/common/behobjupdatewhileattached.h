#ifndef HYPERBLADE_BEHOBJUPDATEWHILEATTACHED_H
#define HYPERBLADE_BEHOBJUPDATEWHILEATTACHED_H

#include "common/flightbehaviorobject.h"

class EngineObject;

// VTABLE: HYPERX 0x004b26fc
class BehObj_UpdateWhileAttached : public FlightBehaviorObject
{
public:
    ~BehObj_UpdateWhileAttached() override;
    void Init(class EngineObject* pOwner, struct PlanNode* pNode, int nFlags) override;
    void SetInteractorsActive(int nArg) override;
    BehObj_UpdateWhileAttached();
};

// SYNTHETIC: HYPERX 0x0041e0c0
// BehObj_UpdateWhileAttached::`scalar deleting destructor'

#endif
