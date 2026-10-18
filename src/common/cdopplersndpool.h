#ifndef HYPERBLADE_CDOPPLERSNDPOOL_H
#define HYPERBLADE_CDOPPLERSNDPOOL_H

#include "common/agendaitemowner.h"

// VTABLE: HYPERX 0x004b1864
class CDopplerSndPool : public AgendaItemOwner
{
public:
    CDopplerSndPool()
    {
        m_pHead = nullptr;
    }

    void  Init() override;
    void* Alloc() override;

    ~CDopplerSndPool() override;
};

// SYNTHETIC: HYPERX 0x00415e40
// CDopplerSndPool::`scalar deleting destructor'

#endif
