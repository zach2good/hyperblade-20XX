#ifndef HYPERBLADE_BEHOBJOPENANDCLOSEPOOL_H
#define HYPERBLADE_BEHOBJOPENANDCLOSEPOOL_H

#include "common/behaviorowner.h"

// VTABLE: HYPERX 0x004b2474
class BehObjOpenAndClosePool : public BehaviorOwner
{
public:
    BehObjOpenAndClosePool()
    {
        m_pHead = 0;
    }

    virtual void  Init();
    virtual void* Alloc();

    ~BehObjOpenAndClosePool();
};

// SYNTHETIC: HYPERX 0x00421420
// BehObjOpenAndClosePool::`scalar deleting destructor'

#endif
