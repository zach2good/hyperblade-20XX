#ifndef HYPERBLADE_BEHOBJWATCHINGPOOL_H
#define HYPERBLADE_BEHOBJWATCHINGPOOL_H

#include "common/behaviorowner.h"

// VTABLE: HYPERX 0x004b2438
class BehObjWatchingPool : public BehaviorOwner
{
public:
    BehObjWatchingPool()
    {
        m_pHead = 0;
    }

    virtual void  Init();
    virtual void* Alloc();

    ~BehObjWatchingPool();
};

// SYNTHETIC: HYPERX 0x00421600
// BehObjWatchingPool::`scalar deleting destructor'

#endif
