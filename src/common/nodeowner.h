#ifndef HYPERBLADE_NODEOWNER_H
#define HYPERBLADE_NODEOWNER_H

#include "planvm/interactor.h"

// A root class, as BehaviorOwner is.
// VTABLE: HYPERX1001 0x004b3840
class NodeOwner
{
public:
    inline NodeOwner()
    {
    }

    // FUNCTION: HYPERX1000 0x004b8380
    // FUNCTION: HYPERX1001 0x0046be80
    virtual ~NodeOwner()
    {
    }

    // Pure: every pool defines its own, and the linker folds the copies.
    virtual NodeOwner* Destroy() = 0;

    virtual void Init() = 0;

    virtual char Insert(void* pNode) = 0;

    // Answers with a node off the free list.
    virtual void* Alloc() = 0;

    class Interactor* m_pHead;
};

// SYNTHETIC: HYPERX1001 0x0046be90
// NodeOwner::`scalar deleting destructor'

#endif
