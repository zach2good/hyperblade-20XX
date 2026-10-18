#ifndef HYPERBLADE_BEHOBJGLIDEBETWEENFACINGPOOL_H
#define HYPERBLADE_BEHOBJGLIDEBETWEENFACINGPOOL_H

#include "common/behaviorowner.h"

// VTABLE: HYPERX 0x004b25a0
class BehObjGlideBetweenFacingPool : public BehaviorOwner
{
public:
    BehObjGlideBetweenFacingPool()
    {
        m_pHead = nullptr;
    }

    void  Init() override;
    void* Alloc() override;

    ~BehObjGlideBetweenFacingPool() override;
};

// SYNTHETIC: HYPERX 0x00420ac0
// BehObjGlideBetweenFacingPool::`scalar deleting destructor'

#endif
