#ifndef HYPERBLADE_CSPEEDBURSTPOOL_H
#define HYPERBLADE_CSPEEDBURSTPOOL_H

#include "common/agendaitemowner.h"

// VTABLE: HYPERX 0x004b1918
class CSpeedBurstPool : public AgendaItemOwner
{
public:
    CSpeedBurstPool()
    {
        m_pHead = 0;
    }

    virtual void  Init();
    virtual void* Alloc();

    ~CSpeedBurstPool();
};

// SYNTHETIC: HYPERX 0x004158a0
// CSpeedBurstPool::`scalar deleting destructor'

#endif
