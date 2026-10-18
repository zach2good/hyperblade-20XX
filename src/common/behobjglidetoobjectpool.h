#ifndef HYPERBLADE_BEHOBJGLIDETOOBJECTPOOL_H
#define HYPERBLADE_BEHOBJGLIDETOOBJECTPOOL_H

#include "common/behaviorowner.h"

// VTABLE: HYPERX 0x004b25c8
class BehObjGlideToObjectPool : public BehaviorOwner
{
public:
    BehObjGlideToObjectPool()
    {
        m_pHead = nullptr;
    }

    void  Init() override;
    void* Alloc() override;

    ~BehObjGlideToObjectPool() override;
};

// SYNTHETIC: HYPERX 0x00420950
// BehObjGlideToObjectPool::`scalar deleting destructor'

#endif
