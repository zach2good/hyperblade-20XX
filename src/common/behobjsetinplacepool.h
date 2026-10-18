#ifndef HYPERBLADE_BEHOBJSETINPLACEPOOL_H
#define HYPERBLADE_BEHOBJSETINPLACEPOOL_H

#include "common/behaviorowner.h"

// VTABLE: HYPERX 0x004b24c4
class BehObjSetInPlacePool : public BehaviorOwner
{
public:
    BehObjSetInPlacePool()
    {
        m_pHead = nullptr;
    }

    void  Init() override;
    void* Alloc() override;

    ~BehObjSetInPlacePool() override;
};

// SYNTHETIC: HYPERX 0x004211a0
// BehObjSetInPlacePool::`scalar deleting destructor'

#endif
