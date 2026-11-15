#ifndef HYPERBLADE_AGENDAITEMOWNER_H
#define HYPERBLADE_AGENDAITEMOWNER_H

class AgendaItem;

// A root class, as BehaviorOwner is.
// VTABLE: HYPERX 0x004b2218
class AgendaItemOwner
{
public:
    __forceinline AgendaItemOwner()
    {
    }

    // FUNCTION: HYPERX100 0x0041d140
    // FUNCTION: HYPERX 0x00414e70
    virtual ~AgendaItemOwner()
    {
    }

    // Pure: every pool defines its own, and the linker folds the copies.
    virtual AgendaItemOwner* Destroy() = 0;

    virtual void Init() = 0;

    virtual char Insert(void* pNode) = 0;

    // Answers with a node off the free list.
    virtual void* Alloc() = 0;

    AgendaItem* m_pHead;
};

// SYNTHETIC: HYPERX 0x00414e80
// AgendaItemOwner::`scalar deleting destructor'

#endif
