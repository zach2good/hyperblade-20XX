#ifndef HYPERBLADE_BEHOBJTRACKTRAJECTORY_H
#define HYPERBLADE_BEHOBJTRACKTRAJECTORY_H

#include "common/flightbehaviorobject.h"

class EngineObject;

// VTABLE: HYPERX 0x004b27ec
class BehObj_TrackTrajectory : public FlightBehaviorObject
{
public:
    virtual ~BehObj_TrackTrajectory();
    virtual void Init(class EngineObject* pOwner, struct PlanNode* pNode, int nFlags);
    BehObj_TrackTrajectory();
};

// SYNTHETIC: HYPERX 0x0041e9a0
// BehObj_TrackTrajectory::`scalar deleting destructor'

#endif
