#ifndef HYPERBLADE_NODEOWNER_H
#define HYPERBLADE_NODEOWNER_H

#include "common/pooledownerroot.h"
#include "planvm/interactor.h"

// VTABLE: HYPERX 0x004b3840
class NodeOwner : public PooledOwnerRoot
{
public:
    inline NodeOwner()
    {
    }

    char Insert(void* pNode) override;

    // FUNCTION: HYPERX 0x0046be80
    ~NodeOwner() override
    {
    }

    class Interactor* m_pHead;
};

// SYNTHETIC: HYPERX 0x0046be90
// NodeOwner::`scalar deleting destructor'

#endif
