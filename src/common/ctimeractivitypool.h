#ifndef HYPERBLADE_CTIMERACTIVITYPOOL_H
#define HYPERBLADE_CTIMERACTIVITYPOOL_H

#include "common/agendaitemowner.h"

// VTABLE: HYPERX 0x004b18f0
class CTimerActivityPool : public AgendaItemOwner
{
public:
    CTimerActivityPool()
    {
        m_pHead = 0;
    }

    virtual void  Init();
    virtual void* Alloc();

    ~CTimerActivityPool();
};

// SYNTHETIC: HYPERX 0x004159e0
// CTimerActivityPool::`scalar deleting destructor'

#endif
