#ifndef HYPERBLADE_CSTEERTOTARGETPOOL_H
#define HYPERBLADE_CSTEERTOTARGETPOOL_H

#include "common/agendaitemowner.h"

// VTABLE: HYPERX 0x004b1a1c
class CSteerToTargetPool : public AgendaItemOwner
{
public:
    CSteerToTargetPool()
    {
        m_pHead = 0;
    }

    virtual void  Init();
    virtual void* Alloc();

    ~CSteerToTargetPool();
};

// SYNTHETIC: HYPERX 0x00415080
// CSteerToTargetPool::`scalar deleting destructor'

#endif
