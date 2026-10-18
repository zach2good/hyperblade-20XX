#ifndef HYPERBLADE_MSGINTERACTORPOOL_H
#define HYPERBLADE_MSGINTERACTORPOOL_H

#include "common/nodeowner.h"

// VTABLE: HYPERX 0x004b3794
class MsgInteractorPool : public NodeOwner
{
public:
    MsgInteractorPool()
    {
        m_pHead = nullptr;
    }

    void  Init() override;
    void* Alloc() override;

    ~MsgInteractorPool() override;
};

// SYNTHETIC: HYPERX 0x0046beb0
// MsgInteractorPool::`scalar deleting destructor'

#endif
