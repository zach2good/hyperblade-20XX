#ifndef HYPERBLADE_CTRACKPASSERRECEIVERPOOL_H
#define HYPERBLADE_CTRACKPASSERRECEIVERPOOL_H

#include "common/agendaitemowner.h"

// VTABLE: HYPERX 0x004b18a0
class CTrackPasser_ReceiverPool : public AgendaItemOwner
{
public:
    CTrackPasser_ReceiverPool()
    {
        m_pHead = 0;
    }

    virtual void  Init();
    virtual void* Alloc();

    ~CTrackPasser_ReceiverPool();
};

// SYNTHETIC: HYPERX 0x00415c60
// CTrackPasser_ReceiverPool::`scalar deleting destructor'

#endif
