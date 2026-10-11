#ifndef HYPERBLADE_CSTEERBETWEENFACINGPOOL_H
#define HYPERBLADE_CSTEERBETWEENFACINGPOOL_H

#include "common/agendaitemowner.h"

// VTABLE: HYPERX 0x004b19e0
class CSteerBetweenFacingPool : public AgendaItemOwner
{
public:
    CSteerBetweenFacingPool()
    {
        m_pHead = 0;
    }

    virtual void  Init();
    virtual void* Alloc();

    ~CSteerBetweenFacingPool();
};

// SYNTHETIC: HYPERX 0x00415260
// CSteerBetweenFacingPool::`scalar deleting destructor'

#endif
