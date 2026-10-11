#ifndef HYPERBLADE_COLLINTERACTORPOOL_H
#define HYPERBLADE_COLLINTERACTORPOOL_H

#include "common/nodeowner.h"

// VTABLE: HYPERX 0x004b37a8
class CollInteractorPool : public NodeOwner
{
public:
    CollInteractorPool()
    {
        m_pHead = 0;
    }

    virtual void  Init();
    virtual void* Alloc();

    ~CollInteractorPool();
};

// SYNTHETIC: HYPERX 0x0046bde0
// CollInteractorPool::`scalar deleting destructor'

#endif
