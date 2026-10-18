#ifndef HYPERBLADE_CPROJECTANDBOUNCEPOOL_H
#define HYPERBLADE_CPROJECTANDBOUNCEPOOL_H

#include "common/agendaitemowner.h"

// VTABLE: HYPERX 0x004b1a44
class CProjectAndBouncePool : public AgendaItemOwner
{
public:
    CProjectAndBouncePool()
    {
        m_pHead = nullptr;
    }

    void  Init() override;
    void* Alloc() override;

    ~CProjectAndBouncePool() override;
};

// SYNTHETIC: HYPERX 0x00414f40
// CProjectAndBouncePool::`scalar deleting destructor'

#endif
