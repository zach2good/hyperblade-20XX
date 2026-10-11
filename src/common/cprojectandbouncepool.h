#ifndef HYPERBLADE_CPROJECTANDBOUNCEPOOL_H
#define HYPERBLADE_CPROJECTANDBOUNCEPOOL_H

#include "common/agendaitemowner.h"

// VTABLE: HYPERX 0x004b1a44
class CProjectAndBouncePool : public AgendaItemOwner
{
public:
    CProjectAndBouncePool()
    {
        m_pHead = 0;
    }

    virtual void  Init();
    virtual void* Alloc();

    ~CProjectAndBouncePool();
};

// SYNTHETIC: HYPERX 0x00414f40
// CProjectAndBouncePool::`scalar deleting destructor'

#endif
