#ifndef HYPERBLADE_CBUNGEETOWARDPOOL_H
#define HYPERBLADE_CBUNGEETOWARDPOOL_H

#include "common/agendaitemowner.h"

// VTABLE: HYPERX 0x004b1990
class CBungeeTowardPool : public AgendaItemOwner
{
public:
    CBungeeTowardPool()
    {
        m_pHead = 0;
    }

    void  Init() override;
    void* Alloc() override;

    ~CBungeeTowardPool() override;
};

// SYNTHETIC: HYPERX 0x004154e0
// CBungeeTowardPool::`scalar deleting destructor'

#endif
