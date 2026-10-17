#ifndef HYPERBLADE_BEHOBJFLYTOWARDPOOL_H
#define HYPERBLADE_BEHOBJFLYTOWARDPOOL_H

#include "common/behaviorowner.h"

// VTABLE: HYPERX 0x004b2500
class BehObjFlyTowardPool : public BehaviorOwner
{
public:
    BehObjFlyTowardPool()
    {
        m_pHead = 0;
    }

    void  Init() override;
    void* Alloc() override;

    ~BehObjFlyTowardPool() override;
};

// SYNTHETIC: HYPERX 0x00420fc0
// BehObjFlyTowardPool::`scalar deleting destructor'

#endif
