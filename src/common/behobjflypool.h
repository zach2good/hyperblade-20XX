#ifndef HYPERBLADE_BEHOBJFLYPOOL_H
#define HYPERBLADE_BEHOBJFLYPOOL_H

#include "common/behaviorowner.h"

// VTABLE: HYPERX 0x004b2528
class BehObjFlyPool : public BehaviorOwner
{
public:
    BehObjFlyPool()
    {
        m_pHead = nullptr;
    }

    void  Init() override;
    void* Alloc() override;

    ~BehObjFlyPool() override;
};

// SYNTHETIC: HYPERX 0x00420e80
// BehObjFlyPool::`scalar deleting destructor'

#endif
