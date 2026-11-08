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

    // FUNCTION: HYPERX 0x00414e70
    virtual ~AgendaItemOwner()
    {
    }

    // Answers what the holding pointer should hold afterwards; every caller assigns it straight back.
    // Inline, as each owner's is, so the linker folds all three onto this one.
    // FUNCTION: HYPERX 0x00414b00
    virtual AgendaItemOwner* Destroy()
    {
        delete this;

        return 0;
    }

    virtual void Init() = 0;

    virtual char Insert(void* pNode);

    // Answers with a node off the free list.
    virtual void* Alloc() = 0;

    AgendaItem* m_pHead;
};

// SYNTHETIC: HYPERX 0x00414e80
// AgendaItemOwner::`scalar deleting destructor'

#endif
