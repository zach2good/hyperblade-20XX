#ifndef HYPERBLADE_BEHOBJTRACKINGPOOL_H
#define HYPERBLADE_BEHOBJTRACKINGPOOL_H

#include "common/behaviorowner.h"

// VTABLE: HYPERX 0x004b2460
class BehObjTrackingPool : public BehaviorOwner
{
public:
    BehObjTrackingPool()
    {
        m_pHead = 0;
    }

    virtual void  Init();
    virtual void* Alloc();

    ~BehObjTrackingPool();
};

// SYNTHETIC: HYPERX 0x004214c0
// BehObjTrackingPool::`scalar deleting destructor'

#endif
