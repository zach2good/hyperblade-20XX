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

    virtual void  Init();
    virtual void* Alloc();

    ~CExplosionAnimationPool();
};

// SYNTHETIC: HYPERX 0x00416200
// CExplosionAnimationPool::`scalar deleting destructor'

#endif
