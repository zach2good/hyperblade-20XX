#ifndef HYPERBLADE_BEHOBJFLYPOOL_H
#define HYPERBLADE_BEHOBJFLYPOOL_H

#include "common/behaviorowner.h"

// VTABLE: HYPERX 0x004b2528
class BehObjFlyPool : public BehaviorOwner
{
public:
    BehObjFlyPool()
    {
        m_pHead = 0;
    }

    virtual void  Init();
    virtual void* Alloc();

    ~BehObjFlyPool();
};

// SYNTHETIC: HYPERX 0x00420e80
// BehObjFlyPool::`scalar deleting destructor'

#endif
