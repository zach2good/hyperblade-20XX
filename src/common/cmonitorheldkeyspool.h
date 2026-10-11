#ifndef HYPERBLADE_CMONITORHELDKEYSPOOL_H
#define HYPERBLADE_CMONITORHELDKEYSPOOL_H

#include "common/agendaitemowner.h"

// VTABLE: HYPERX 0x004b192c
class CMonitorHeldKeysPool : public AgendaItemOwner
{
public:
    CMonitorHeldKeysPool()
    {
        m_pHead = 0;
    }

    virtual void  Init();
    virtual void* Alloc();

    ~CMonitorHeldKeysPool();
};

// SYNTHETIC: HYPERX 0x00415800
// CMonitorHeldKeysPool::`scalar deleting destructor'

#endif
