#ifndef HYPERBLADE_CSTEERTOWARDPOOL_H
#define HYPERBLADE_CSTEERTOWARDPOOL_H

#include "common/agendaitemowner.h"

// VTABLE: HYPERX 0x004b19cc
class CSteerTowardPool : public AgendaItemOwner
{
public:
    CSteerTowardPool()
    {
        m_pHead = 0;
    }

    virtual void  Init();
    virtual void* Alloc();

    ~CSteerTowardPool();
};

// SYNTHETIC: HYPERX 0x00415300
// CSteerTowardPool::`scalar deleting destructor'

#endif
