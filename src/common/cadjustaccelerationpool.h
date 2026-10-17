#ifndef HYPERBLADE_CADJUSTACCELERATIONPOOL_H
#define HYPERBLADE_CADJUSTACCELERATIONPOOL_H

#include "common/agendaitemowner.h"

// VTABLE: HYPERX 0x004b19b8
class CAdjustAccelerationPool : public AgendaItemOwner
{
public:
    CAdjustAccelerationPool()
    {
        m_pHead = 0;
    }

    void  Init() override;
    void* Alloc() override;

    ~CAdjustAccelerationPool() override;
};

// SYNTHETIC: HYPERX 0x004153a0
// CAdjustAccelerationPool::`scalar deleting destructor'

#endif
