#ifndef HYPERBLADE_CRELAXYAWPOOL_H
#define HYPERBLADE_CRELAXYAWPOOL_H

#include "common/agendaitemowner.h"

// VTABLE: HYPERX 0x004b1968
class CRelaxYawPool : public AgendaItemOwner
{
public:
    CRelaxYawPool()
    {
        m_pHead = nullptr;
    }

    void  Init() override;
    void* Alloc() override;

    ~CRelaxYawPool() override;
};

// SYNTHETIC: HYPERX 0x00415620
// CRelaxYawPool::`scalar deleting destructor'

#endif
