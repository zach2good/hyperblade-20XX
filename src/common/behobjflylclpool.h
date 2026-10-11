#ifndef HYPERBLADE_BEHOBJFLYLCLPOOL_H
#define HYPERBLADE_BEHOBJFLYLCLPOOL_H

#include "common/behaviorowner.h"

// VTABLE: HYPERX 0x004b2514
class BehObjFlyLCLPool : public BehaviorOwner
{
public:
    BehObjFlyLCLPool()
    {
        m_pHead = 0;
    }

    virtual void  Init();
    virtual void* Alloc();

    ~BehObjFlyLCLPool();
};

// SYNTHETIC: HYPERX 0x00420f20
// BehObjFlyLCLPool::`scalar deleting destructor'

#endif
