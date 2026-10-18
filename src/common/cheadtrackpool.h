#ifndef HYPERBLADE_CHEADTRACKPOOL_H
#define HYPERBLADE_CHEADTRACKPOOL_H

#include "common/agendaitemowner.h"

// VTABLE: HYPERX 0x004b1850
class CHeadTrackPool : public AgendaItemOwner
{
public:
    CHeadTrackPool()
    {
        m_pHead = nullptr;
    }

    void  Init() override;
    void* Alloc() override;

    ~CHeadTrackPool() override;
};

// SYNTHETIC: HYPERX 0x00415ee0
// CHeadTrackPool::`scalar deleting destructor'

#endif
