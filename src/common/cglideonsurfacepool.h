#ifndef HYPERBLADE_CGLIDEONSURFACEPOOL_H
#define HYPERBLADE_CGLIDEONSURFACEPOOL_H

#include "common/agendaitemowner.h"

// VTABLE: HYPERX 0x004b1a6c
class CGlideOnSurfacePool : public AgendaItemOwner
{
public:
    CGlideOnSurfacePool()
    {
        m_pHead = 0;
    }

    virtual void  Init();
    virtual void* Alloc();

    ~CGlideOnSurfacePool();
};

// SYNTHETIC: HYPERX 0x00414dd0
// CGlideOnSurfacePool::`scalar deleting destructor'

#endif
