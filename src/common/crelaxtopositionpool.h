#ifndef HYPERBLADE_CRELAXTOPOSITIONPOOL_H
#define HYPERBLADE_CRELAXTOPOSITIONPOOL_H

#include "common/agendaitemowner.h"

// VTABLE: HYPERX 0x004b1904
class CRelaxToPositionPool : public AgendaItemOwner
{
public:
    CRelaxToPositionPool()
    {
        m_pHead = nullptr;
    }

    void  Init() override;
    void* Alloc() override;

    ~CRelaxToPositionPool() override;
};

// SYNTHETIC: HYPERX 0x00415940
// CRelaxToPositionPool::`scalar deleting destructor'

#endif
