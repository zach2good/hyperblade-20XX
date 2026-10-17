#ifndef HYPERBLADE_BEHOBJUPDATEWHILEATTACHEDPOOL_H
#define HYPERBLADE_BEHOBJUPDATEWHILEATTACHEDPOOL_H

#include "common/behaviorowner.h"

// VTABLE: HYPERX 0x004b24d8
class BehObjUpdateWhileAttachedPool : public BehaviorOwner
{
public:
    BehObjUpdateWhileAttachedPool()
    {
        m_pHead = 0;
    }

    void  Init() override;
    void* Alloc() override;

    ~BehObjUpdateWhileAttachedPool() override;
};

// SYNTHETIC: HYPERX 0x00421100
// BehObjUpdateWhileAttachedPool::`scalar deleting destructor'

#endif
