#ifndef HYPERBLADE_BEHOBJWATCHING_H
#define HYPERBLADE_BEHOBJWATCHING_H

#include "common/flightbehaviorobject.h"

class EngineObject;

// VTABLE: HYPERX 0x004b27d4
class BehObj_Watching : public FlightBehaviorObject
{
public:
    virtual ~BehObj_Watching();
    virtual void Init(class EngineObject* pOwner, struct PlanNode* pNode, int nFlags);
    BehObj_Watching();
};

// SYNTHETIC: HYPERX 0x0041e8d0
// BehObj_Watching::`scalar deleting destructor'

static_assert(sizeof(BehObj_Watching) == 0x98);

#endif
