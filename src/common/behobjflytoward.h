#ifndef HYPERBLADE_BEHOBJFLYTOWARD_H
#define HYPERBLADE_BEHOBJFLYTOWARD_H

#include "common/flightbehaviorobject.h"

class EngineObject;

// VTABLE: HYPERX 0x004b26cc
class BehObj_FlyToward : public FlightBehaviorObject
{
public:
    virtual ~BehObj_FlyToward();
    virtual void Init(class EngineObject* pOwner, struct PlanNode* pNode, int nFlags);
    BehObj_FlyToward();
};

// SYNTHETIC: HYPERX 0x0041de40
// BehObj_FlyToward::`scalar deleting destructor'

#endif
