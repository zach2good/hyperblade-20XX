#ifndef HYPERBLADE_CSTEERTOWARDPOOL_H
#define HYPERBLADE_CSTEERTOWARDPOOL_H

#include "common/agendaitemowner.h"

// VTABLE: HYPERX 0x004b19cc
class CSteerTowardPool : public AgendaItemOwner
{
public:
    CSteerTowardPool()
    {
        m_pHead = nullptr;
    }

    void  Init() override;
    void* Alloc() override;

    ~CSteerTowardPool() override;
};

// SYNTHETIC: HYPERX 0x00415300
// CSteerTowardPool::`scalar deleting destructor'

#endif
