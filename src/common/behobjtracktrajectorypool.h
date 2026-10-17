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

    void  Init() override;
    void* Alloc() override;

    ~BehObjTrackTrajectoryPool() override;
};

// SYNTHETIC: HYPERX 0x004216a0
// BehObjTrackTrajectoryPool::`scalar deleting destructor'

#endif
