#ifndef HYPERBLADE_CDISPLAYPASSBARPOOL_H
#define HYPERBLADE_CDISPLAYPASSBARPOOL_H

#include "common/agendaitemowner.h"

// VTABLE: HYPERX 0x004b1800
class CDisplayPassBarPool : public AgendaItemOwner
{
public:
    CDisplayPassBarPool()
    {
        m_pHead = 0;
    }

    void  Init() override;
    void* Alloc() override;

    ~CDisplayPassBarPool() override;
};

// SYNTHETIC: HYPERX 0x00416160
// CDisplayPassBarPool::`scalar deleting destructor'

#endif
