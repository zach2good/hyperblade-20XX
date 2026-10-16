#ifndef HYPERBLADE_BEHOBJTRACKPASSPREP_H
#define HYPERBLADE_BEHOBJTRACKPASSPREP_H

#include "common/flightbehaviorobject.h"

class EngineObject;

// VTABLE: HYPERX 0x004b2804
class BehObj_TrackPassPrep : public FlightBehaviorObject
{
public:
    virtual ~BehObj_TrackPassPrep();
    virtual void Init(class EngineObject* pOwner, struct PlanNode* pNode, int nFlags);
    BehObj_TrackPassPrep();
};

// SYNTHETIC: HYPERX 0x0041ea80
// BehObj_TrackPassPrep::`scalar deleting destructor'

#endif
