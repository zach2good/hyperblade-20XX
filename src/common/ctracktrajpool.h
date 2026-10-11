#ifndef HYPERBLADE_CTRACKTRAJPOOL_H
#define HYPERBLADE_CTRACKTRAJPOOL_H

#include "common/agendaitemowner.h"

// VTABLE: HYPERX 0x004b18c8
class CTrackTrajPool : public AgendaItemOwner
{
public:
    CTrackTrajPool()
    {
        m_pHead = 0;
    }

    virtual void  Init();
    virtual void* Alloc();

    ~CTrackTrajPool();
};

// SYNTHETIC: HYPERX 0x00415b20
// CTrackTrajPool::`scalar deleting destructor'

#endif
