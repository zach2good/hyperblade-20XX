#ifndef HYPERBLADE_MSGINTERACTORPOOL_H
#define HYPERBLADE_MSGINTERACTORPOOL_H

#include "common/nodeowner.h"
#include <memory>
#include <vector>

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

private:
    // Every node the pool has made, handed out or not; the free list links through them.
    std::vector<std::unique_ptr<Interactor> > m_nodes;
};

// FUNCTION: HYPERX 0x0046bed0
// MsgInteractorPool::~MsgInteractorPool

// SYNTHETIC: HYPERX 0x0046beb0
// MsgInteractorPool::`scalar deleting destructor'

#endif
