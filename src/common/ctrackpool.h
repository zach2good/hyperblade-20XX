#ifndef HYPERBLADE_CTRACKPOOL_H
#define HYPERBLADE_CTRACKPOOL_H

#include "common/agendaitemowner.h"

// VTABLE: HYPERX 0x004b18dc
class CTrackPool : public AgendaItemOwner
{
public:
    CTrackPool()
    {
        m_pHead = 0;
    }

    void  Init() override;
    void* Alloc() override;

    ~CTrackPool() override;
};

// SYNTHETIC: HYPERX 0x00415a80
// CTrackPool::`scalar deleting destructor'

#endif
