#ifndef HYPERBLADE_BEHOBJGLIDELCLPOOL_H
#define HYPERBLADE_BEHOBJGLIDELCLPOOL_H

#include "common/behaviorowner.h"

// VTABLE: HYPERX 0x004b2550
class BehObjGlideLCLPool : public BehaviorOwner
{
public:
    BehObjGlideLCLPool()
    {
        m_pHead = nullptr;
    }

    void  Init() override;
    void* Alloc() override;

    ~BehObjGlideLCLPool() override;
};

// SYNTHETIC: HYPERX 0x00420d40
// BehObjGlideLCLPool::`scalar deleting destructor'

#endif
