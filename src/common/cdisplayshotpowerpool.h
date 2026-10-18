#ifndef HYPERBLADE_CDISPLAYSHOTPOWERPOOL_H
#define HYPERBLADE_CDISPLAYSHOTPOWERPOOL_H

#include "common/agendaitemowner.h"

// VTABLE: HYPERX 0x004b1828
class CDisplayShotPowerPool : public AgendaItemOwner
{
public:
    CDisplayShotPowerPool()
    {
        m_pHead = nullptr;
    }

    void  Init() override;
    void* Alloc() override;

    ~CDisplayShotPowerPool() override;
};

// SYNTHETIC: HYPERX 0x00416020
// CDisplayShotPowerPool::`scalar deleting destructor'

#endif
