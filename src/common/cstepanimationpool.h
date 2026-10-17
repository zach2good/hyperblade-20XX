#ifndef HYPERBLADE_CSTEPANIMATIONPOOL_H
#define HYPERBLADE_CSTEPANIMATIONPOOL_H

#include "common/agendaitemowner.h"

// VTABLE: HYPERX 0x004b1954
class CStepAnimationPool : public AgendaItemOwner
{
public:
    CStepAnimationPool()
    {
        m_pHead = 0;
    }

    void  Init() override;
    void* Alloc() override;

    ~CStepAnimationPool() override;
};

// SYNTHETIC: HYPERX 0x004156c0
// CStepAnimationPool::`scalar deleting destructor'

#endif
