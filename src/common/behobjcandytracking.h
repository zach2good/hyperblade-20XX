#ifndef HYPERBLADE_BEHOBJCANDYTRACKING_H
#define HYPERBLADE_BEHOBJCANDYTRACKING_H

#include "common/flightbehaviorobject.h"

class EngineObject;

// VTABLE: HYPERX 0x004b281c
class BehObj_CandyTracking : public FlightBehaviorObject
{
public:
    virtual ~BehObj_CandyTracking();
    virtual void Init(class EngineObject* pOwner, struct PlanNode* pNode, int nFlags);
    BehObj_CandyTracking();
};

// SYNTHETIC: HYPERX 0x0041eba0
// BehObj_CandyTracking::`scalar deleting destructor'

#endif
