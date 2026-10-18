#ifndef HYPERBLADE_CUPDATESHOTPOWERPOOL_H
#define HYPERBLADE_CUPDATESHOTPOWERPOOL_H

#include "common/agendaitemowner.h"

// VTABLE: HYPERX 0x004b1814
class CUpdateShotPowerPool : public AgendaItemOwner
{
public:
    CUpdateShotPowerPool()
    {
        m_pHead = nullptr;
    }

    void  Init() override;
    void* Alloc() override;

    ~CUpdateShotPowerPool() override;
};

// SYNTHETIC: HYPERX 0x004160c0
// CUpdateShotPowerPool::`scalar deleting destructor'

#endif
