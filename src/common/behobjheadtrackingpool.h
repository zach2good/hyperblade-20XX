#ifndef HYPERBLADE_BEHOBJHEADTRACKINGPOOL_H
#define HYPERBLADE_BEHOBJHEADTRACKINGPOOL_H

#include "common/behaviorowner.h"

// VTABLE: HYPERX1001 0x004b244c
class BehObjHeadTrackingPool : public BehaviorOwner
{
public:
    BehObjHeadTrackingPool()
    {
        m_pHead = 0;
    }

    virtual BehaviorOwner* Destroy()
    {
        delete this;

        return 0;
    }

    virtual void  Init();
    virtual char  Insert(void* pNode);
    virtual void* Alloc();

    ~BehObjHeadTrackingPool();
};

// SYNTHETIC: HYPERX1001 0x00421560
// BehObjHeadTrackingPool::`scalar deleting destructor'

#endif
