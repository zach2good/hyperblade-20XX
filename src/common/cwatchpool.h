#ifndef HYPERBLADE_CWATCHPOOL_H
#define HYPERBLADE_CWATCHPOOL_H

#include "common/agendaitemowner.h"

// VTABLE: HYPERX 0x004b18b4
class CWatchPool : public AgendaItemOwner
{
public:
    CWatchPool()
    {
        m_pHead = 0;
    }

    virtual void  Init();
    virtual void* Alloc();

    ~CWatchPool();
};

// SYNTHETIC: HYPERX 0x00415bc0
// CWatchPool::`scalar deleting destructor'

#endif
