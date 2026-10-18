#ifndef HYPERBLADE_BEHOBJCANDYTRACKINGPOOL_H
#define HYPERBLADE_BEHOBJCANDYTRACKINGPOOL_H

#include "common/behaviorowner.h"

// VTABLE: HYPERX 0x004b23e8
class BehObjCandyTrackingPool : public BehaviorOwner
{
public:
    BehObjCandyTrackingPool()
    {
        m_pHead = nullptr;
    }

    void  Init() override;
    void* Alloc() override;

    ~BehObjCandyTrackingPool() override;
};

// SYNTHETIC: HYPERX 0x00421880
// BehObjCandyTrackingPool::`scalar deleting destructor'

#endif
