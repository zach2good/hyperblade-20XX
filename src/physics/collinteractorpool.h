#ifndef HYPERBLADE_COLLINTERACTORPOOL_H
#define HYPERBLADE_COLLINTERACTORPOOL_H

#include "common/nodeowner.h"

// VTABLE: HYPERX 0x004b37a8
class CollInteractorPool : public NodeOwner
{
public:
    CollInteractorPool()
    {
        m_pHead = nullptr;
    }

    void  Init() override;
    void* Alloc() override;

    ~CollInteractorPool() override;
};

// SYNTHETIC: HYPERX 0x0046bde0
// CollInteractorPool::`scalar deleting destructor'

#endif
