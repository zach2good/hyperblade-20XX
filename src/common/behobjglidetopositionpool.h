#ifndef HYPERBLADE_BEHOBJGLIDETOPOSITIONPOOL_H
#define HYPERBLADE_BEHOBJGLIDETOPOSITIONPOOL_H

#include "common/behaviorowner.h"

// VTABLE: HYPERX 0x004b25b4
class BehObjGlideToPositionPool : public BehaviorOwner
{
public:
    BehObjGlideToPositionPool()
    {
        m_pHead = nullptr;
    }

    void  Init() override;
    void* Alloc() override;

    ~BehObjGlideToPositionPool() override;
};

// SYNTHETIC: HYPERX 0x00420a20
// BehObjGlideToPositionPool::`scalar deleting destructor'

#endif
