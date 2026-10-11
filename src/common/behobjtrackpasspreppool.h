#ifndef HYPERBLADE_BEHOBJTRACKPASSPREPPOOL_H
#define HYPERBLADE_BEHOBJTRACKPASSPREPPOOL_H

#include "common/behaviorowner.h"

// VTABLE: HYPERX 0x004b2410
class BehObjTrackPassPrepPool : public BehaviorOwner
{
public:
    BehObjTrackPassPrepPool()
    {
        m_pHead = 0;
    }

    virtual void  Init();
    virtual void* Alloc();

    ~BehObjTrackPassPrepPool();
};

// SYNTHETIC: HYPERX 0x00421740
// BehObjTrackPassPrepPool::`scalar deleting destructor'

#endif
