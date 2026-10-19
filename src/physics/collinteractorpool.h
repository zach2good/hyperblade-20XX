#ifndef HYPERBLADE_COLLINTERACTORPOOL_H
#define HYPERBLADE_COLLINTERACTORPOOL_H

#include "common/nodeowner.h"
#include <memory>
#include <vector>

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

private:
    // Every node the pool has made, handed out or not; the free list links through them.
    std::vector<std::unique_ptr<Interactor> > m_nodes;
};

// FUNCTION: HYPERX 0x0046be00
// CollInteractorPool::~CollInteractorPool

// SYNTHETIC: HYPERX 0x0046bde0
// CollInteractorPool::`scalar deleting destructor'

#endif
