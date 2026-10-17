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

    void  Init() override;
    void* Alloc() override;

    ~CSteerBetweenFacingPool() override;
};

// SYNTHETIC: HYPERX 0x00415260
// CSteerBetweenFacingPool::`scalar deleting destructor'

#endif
