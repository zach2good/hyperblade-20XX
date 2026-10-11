#ifndef HYPERBLADE_BEHOBJTRACKTRAJECTORYPOOL_H
#define HYPERBLADE_BEHOBJTRACKTRAJECTORYPOOL_H

#include "common/behaviorowner.h"

// VTABLE: HYPERX 0x004b2424
class BehObjTrackTrajectoryPool : public BehaviorOwner
{
public:
    BehObjTrackTrajectoryPool()
    {
        m_pHead = 0;
    }

    virtual void  Init();
    virtual void* Alloc();

    ~BehObjTrackTrajectoryPool();
};

// SYNTHETIC: HYPERX 0x004216a0
// BehObjTrackTrajectoryPool::`scalar deleting destructor'

#endif
