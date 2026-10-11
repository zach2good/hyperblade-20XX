#ifndef HYPERBLADE_CHEADTRACKPOOL_H
#define HYPERBLADE_CHEADTRACKPOOL_H

#include "common/agendaitemowner.h"

// VTABLE: HYPERX 0x004b1850
class CHeadTrackPool : public AgendaItemOwner
{
public:
    CHeadTrackPool()
    {
        m_pHead = 0;
    }

    virtual void  Init();
    virtual void* Alloc();

    ~CHeadTrackPool();
};

// SYNTHETIC: HYPERX 0x00415ee0
// CHeadTrackPool::`scalar deleting destructor'

#endif
