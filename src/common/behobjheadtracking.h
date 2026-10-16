#ifndef HYPERBLADE_BEHOBJHEADTRACKING_H
#define HYPERBLADE_BEHOBJHEADTRACKING_H

#include "common/flightbehaviorobject.h"

class EngineObject;

// VTABLE: HYPERX 0x004b27bc
class BehObj_HeadTracking : public FlightBehaviorObject
{
public:
    virtual ~BehObj_HeadTracking();
    virtual void Init(class EngineObject* pOwner, struct PlanNode* pNode, int nFlags);
    BehObj_HeadTracking();
};

// SYNTHETIC: HYPERX 0x0041e800
// BehObj_HeadTracking::`scalar deleting destructor'

static_assert(sizeof(BehObj_HeadTracking) == 0x98);

#endif
