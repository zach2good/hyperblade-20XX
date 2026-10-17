#ifndef HYPERBLADE_AGENDAITEMOWNER_H
#define HYPERBLADE_AGENDAITEMOWNER_H

#include "common/pooledownerroot.h"

class AgendaItem;

// VTABLE: HYPERX 0x004b2218
class AgendaItemOwner : public PooledOwnerRoot
{
public:
    AgendaItemOwner()
    {
    }

    char Insert(void* pNode) override;

    // FUNCTION: HYPERX 0x00414e70
    ~AgendaItemOwner() override
    {
    }

    AgendaItem* m_pHead;
};

// SYNTHETIC: HYPERX 0x00414e80
// AgendaItemOwner::`scalar deleting destructor'

#endif
