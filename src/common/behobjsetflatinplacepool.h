#ifndef HYPERBLADE_BEHOBJSETFLATINPLACEPOOL_H
#define HYPERBLADE_BEHOBJSETFLATINPLACEPOOL_H

#include "common/behaviorowner.h"

// VTABLE: HYPERX 0x004b24b0
class BehObjSetFlatInPlacePool : public BehaviorOwner
{
public:
    BehObjSetFlatInPlacePool()
    {
        m_pHead = nullptr;
    }

    void  Init() override;
    void* Alloc() override;

    ~BehObjSetFlatInPlacePool() override;
};

// SYNTHETIC: HYPERX 0x00421240
// BehObjSetFlatInPlacePool::`scalar deleting destructor'

#endif
