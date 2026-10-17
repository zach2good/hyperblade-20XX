#ifndef HYPERBLADE_CPANSOUNDPOOL_H
#define HYPERBLADE_CPANSOUNDPOOL_H

#include "common/agendaitemowner.h"

// VTABLE: HYPERX 0x004b1878
class CPanSoundPool : public AgendaItemOwner
{
public:
    CPanSoundPool()
    {
        m_pHead = 0;
    }

    void  Init() override;
    void* Alloc() override;

    ~CPanSoundPool() override;
};

// SYNTHETIC: HYPERX 0x00415da0
// CPanSoundPool::`scalar deleting destructor'

#endif
