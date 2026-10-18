#ifndef HYPERBLADE_BEHOBJGLIDEPOOL_H
#define HYPERBLADE_BEHOBJGLIDEPOOL_H

#include "common/behaviorowner.h"

// VTABLE: HYPERX 0x004b2564
class BehObjGlidePool : public BehaviorOwner
{
public:
    BehObjGlidePool()
    {
        m_pHead = nullptr;
    }

    void  Init() override;
    void* Alloc() override;

    ~BehObjGlidePool() override;
};

// SYNTHETIC: HYPERX 0x00420ca0
// BehObjGlidePool::`scalar deleting destructor'

#endif
