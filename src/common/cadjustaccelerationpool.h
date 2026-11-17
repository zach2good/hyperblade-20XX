#ifndef HYPERBLADE_CADJUSTACCELERATIONPOOL_H
#define HYPERBLADE_CADJUSTACCELERATIONPOOL_H

#include "common/agendaitemowner.h"

// VTABLE: HYPERX1001 0x004b19b8
class CAdjustAccelerationPool : public AgendaItemOwner
{
public:
    CAdjustAccelerationPool()
    {
        m_pHead = 0;
    }

    virtual AgendaItemOwner* Destroy()
    {
        delete this;

        return 0;
    }

    virtual void  Init();
    virtual char  Insert(void* pNode);
    virtual void* Alloc();

    ~CAdjustAccelerationPool();
};

// SYNTHETIC: HYPERX1001 0x004153a0
// CAdjustAccelerationPool::`scalar deleting destructor'

#endif
