#ifndef HYPERBLADE_CEXPLOSIONANIMATIONPOOL_H
#define HYPERBLADE_CEXPLOSIONANIMATIONPOOL_H

#include "common/agendaitemowner.h"

// VTABLE: HYPERX1001 0x004b17ec
class CExplosionAnimationPool : public AgendaItemOwner
{
public:
    CExplosionAnimationPool()
    {
        m_pHead = 0;
    }

    virtual AgendaItemOwner* Destroy()
    {
        delete this;

        return 0;
    }

    virtual void  Init();
    virtual char  Insert(void* pNode);
    virtual void* Alloc();

    ~CExplosionAnimationPool();
};

// SYNTHETIC: HYPERX1001 0x00416200
// CExplosionAnimationPool::`scalar deleting destructor'

#endif
