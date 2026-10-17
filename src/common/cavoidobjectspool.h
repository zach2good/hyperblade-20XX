#ifndef HYPERBLADE_CAVOIDOBJECTSPOOL_H
#define HYPERBLADE_CAVOIDOBJECTSPOOL_H

#include "common/agendaitemowner.h"

// VTABLE: HYPERX 0x004b19a4
class CAvoidObjectsPool : public AgendaItemOwner
{
public:
    CAvoidObjectsPool()
    {
        m_pHead = 0;
    }

    void  Init() override;
    void* Alloc() override;

    ~CAvoidObjectsPool() override;
};

// SYNTHETIC: HYPERX 0x00415440
// CAvoidObjectsPool::`scalar deleting destructor'

#endif
