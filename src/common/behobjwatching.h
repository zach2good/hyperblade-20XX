#ifndef HYPERBLADE_BEHOBJWATCHING_H
#define HYPERBLADE_BEHOBJWATCHING_H

#include "common/flightbehaviorobject.h"

class EngineObject;

// VTABLE: HYPERX 0x004b27d4
class BehObj_Watching : public FlightBehaviorObject
{
public:
    ~BehObj_Watching() override;
    void Init(class EngineObject* pOwner, struct PlanNode* pNode, int nFlags) override;
    BehObj_Watching();
};

// SYNTHETIC: HYPERX 0x0041e8d0
// BehObj_Watching::`scalar deleting destructor'

#endif
