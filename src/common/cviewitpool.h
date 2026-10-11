#ifndef HYPERBLADE_CVIEWITPOOL_H
#define HYPERBLADE_CVIEWITPOOL_H

#include "common/agendaitemowner.h"

// VTABLE: HYPERX 0x004b188c
class CViewItPool : public AgendaItemOwner
{
public:
    CViewItPool()
    {
        m_pHead = 0;
    }

    virtual void  Init();
    virtual void* Alloc();

    ~CViewItPool();
};

// SYNTHETIC: HYPERX 0x00415d00
// CViewItPool::`scalar deleting destructor'

#endif
