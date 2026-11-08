#ifndef HYPERBLADE_NODEOWNER_H
#define HYPERBLADE_NODEOWNER_H

#include "planvm/interactor.h"

// A root class, as BehaviorOwner is.
// VTABLE: HYPERX 0x004b3840
class NodeOwner
{
public:
    inline NodeOwner()
    {
    }

    // FUNCTION: HYPERX 0x0046be80
    virtual ~NodeOwner()
    {
    }

    // Answers what the holding pointer should hold afterwards; every caller assigns it straight back.
    // The linker folded this onto AgendaItemOwner::Destroy, which carries the marker.
    virtual NodeOwner* Destroy()
    {
        delete this;

        return 0;
    }

    virtual void Init() = 0;

    virtual char Insert(void* pNode);

    // Answers with a node off the free list.
    virtual void* Alloc() = 0;

    class Interactor* m_pHead;
};

// SYNTHETIC: HYPERX 0x0046be90
// NodeOwner::`scalar deleting destructor'

#endif
