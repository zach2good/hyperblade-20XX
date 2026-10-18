#ifndef HYPERBLADE_BEHOBJAVOIDANCEPOOL_H
#define HYPERBLADE_BEHOBJAVOIDANCEPOOL_H

#include "common/behaviorowner.h"

// VTABLE: HYPERX 0x004b258c
class BehObjAvoidancePool : public BehaviorOwner
{
public:
    BehObjAvoidancePool()
    {
        m_pHead = nullptr;
    }

    void  Init() override;
    void* Alloc() override;

    ~BehObjAvoidancePool() override;
};

// SYNTHETIC: HYPERX 0x00420b60
// BehObjAvoidancePool::`scalar deleting destructor'

#endif
