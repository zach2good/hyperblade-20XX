#ifndef HYPERBLADE_CCIRCLETRACKPOOL_H
#define HYPERBLADE_CCIRCLETRACKPOOL_H

#include "common/agendaitemowner.h"

// VTABLE: HYPERX 0x004b17c4
class CCircleTrackPool : public AgendaItemOwner
{
public:
    CCircleTrackPool()
    {
        m_pHead = 0;
    }

    virtual void  Init();
    virtual void* Alloc();

    ~CCircleTrackPool();
};

// SYNTHETIC: HYPERX 0x00416340
// CCircleTrackPool::`scalar deleting destructor'

#endif
