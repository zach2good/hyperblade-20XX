#ifndef HYPERBLADE_CCIRCLETRACKPOOL_H
#define HYPERBLADE_CCIRCLETRACKPOOL_H

#include "common/agendaitemowner.h"

// VTABLE: HYPERX 0x004b17c4
class CCircleTrackPool : public AgendaItemOwner
{
public:
    CCircleTrackPool()
    {
        m_pHead = nullptr;
    }

    void  Init() override;
    void* Alloc() override;

    ~CCircleTrackPool() override;
};

// SYNTHETIC: HYPERX 0x00416340
// CCircleTrackPool::`scalar deleting destructor'

#endif
