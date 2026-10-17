#ifndef HYPERBLADE_CEXPLOSIONANIMATIONPOOL_H
#define HYPERBLADE_CEXPLOSIONANIMATIONPOOL_H

#include "common/agendaitemowner.h"

// VTABLE: HYPERX 0x004b17ec
class CExplosionAnimationPool : public AgendaItemOwner
{
public:
    CExplosionAnimationPool()
    {
        m_pHead = 0;
    }

    void  Init() override;
    void* Alloc() override;

    ~CExplosionAnimationPool() override;
};

// SYNTHETIC: HYPERX 0x00416200
// CExplosionAnimationPool::`scalar deleting destructor'

#endif
