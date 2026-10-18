#ifndef HYPERBLADE_BEHOBJHEADTRACKINGPOOL_H
#define HYPERBLADE_BEHOBJHEADTRACKINGPOOL_H

#include "common/behaviorowner.h"

// VTABLE: HYPERX 0x004b244c
class BehObjHeadTrackingPool : public BehaviorOwner
{
public:
    BehObjHeadTrackingPool()
    {
        m_pHead = nullptr;
    }

    void  Init() override;
    void* Alloc() override;

    ~BehObjHeadTrackingPool() override;
};

// SYNTHETIC: HYPERX 0x00421560
// BehObjHeadTrackingPool::`scalar deleting destructor'

#endif
