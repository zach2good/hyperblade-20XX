#ifndef HYPERBLADE_BEHOBJSETMOVINGINPLACEPOOL_H
#define HYPERBLADE_BEHOBJSETMOVINGINPLACEPOOL_H

#include "common/behaviorowner.h"

// VTABLE: HYPERX 0x004b249c
class BehObjSetMovingInPlacePool : public BehaviorOwner
{
public:
    BehObjSetMovingInPlacePool()
    {
        m_pHead = nullptr;
    }

    void  Init() override;
    void* Alloc() override;

    ~BehObjSetMovingInPlacePool() override;
};

// SYNTHETIC: HYPERX 0x004212e0
// BehObjSetMovingInPlacePool::`scalar deleting destructor'

#endif
