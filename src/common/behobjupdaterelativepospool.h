#ifndef HYPERBLADE_BEHOBJUPDATERELATIVEPOSPOOL_H
#define HYPERBLADE_BEHOBJUPDATERELATIVEPOSPOOL_H

#include "common/behaviorowner.h"

// VTABLE: HYPERX 0x004b2578
class BehObjUpdateRelativePosPool : public BehaviorOwner
{
public:
    BehObjUpdateRelativePosPool()
    {
        m_pHead = 0;
    }

    void  Init() override;
    void* Alloc() override;

    ~BehObjUpdateRelativePosPool() override;
};

// SYNTHETIC: HYPERX 0x00420c00
// BehObjUpdateRelativePosPool::`scalar deleting destructor'

#endif
