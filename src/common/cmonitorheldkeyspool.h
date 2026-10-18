#ifndef HYPERBLADE_CMONITORHELDKEYSPOOL_H
#define HYPERBLADE_CMONITORHELDKEYSPOOL_H

#include "common/agendaitemowner.h"

// VTABLE: HYPERX 0x004b192c
class CMonitorHeldKeysPool : public AgendaItemOwner
{
public:
    CMonitorHeldKeysPool()
    {
        m_pHead = nullptr;
    }

    void  Init() override;
    void* Alloc() override;

    ~CMonitorHeldKeysPool() override;
};

// SYNTHETIC: HYPERX 0x00415800
// CMonitorHeldKeysPool::`scalar deleting destructor'

#endif
