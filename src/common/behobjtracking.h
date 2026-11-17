#ifndef HYPERBLADE_BEHOBJTRACKING_H
#define HYPERBLADE_BEHOBJTRACKING_H

#include "common/flightbehaviorobject.h"

class EngineObject;

// VTABLE: HYPERX1001 0x004b27a4
class BehObj_Tracking : public FlightBehaviorObject
{
public:
    virtual ~BehObj_Tracking();
    virtual void Init(class EngineObject* pOwner, struct PlanNode* pNode, int nFlags);
    BehObj_Tracking();
};

// SYNTHETIC: HYPERX1001 0x0041e730
// BehObj_Tracking::`scalar deleting destructor'

STATIC_ASSERT(sizeof(BehObj_Tracking) == 0x98);

#endif
