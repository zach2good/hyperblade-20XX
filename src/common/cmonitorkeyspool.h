#ifndef HYPERBLADE_CMONITORKEYSPOOL_H
#define HYPERBLADE_CMONITORKEYSPOOL_H

#include "common/agendaitemowner.h"

// VTABLE: HYPERX 0x004b1940
class CMonitorKeysPool : public AgendaItemOwner
{
public:
    CMonitorKeysPool()
    {
        m_pHead = nullptr;
    }

    void  Init() override;
    void* Alloc() override;

    ~CMonitorKeysPool() override;
};

// SYNTHETIC: HYPERX 0x00415760
// CMonitorKeysPool::`scalar deleting destructor'

#endif
