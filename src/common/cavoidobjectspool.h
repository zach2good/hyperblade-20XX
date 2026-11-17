#ifndef HYPERBLADE_CAVOIDOBJECTSPOOL_H
#define HYPERBLADE_CAVOIDOBJECTSPOOL_H

#include "common/agendaitemowner.h"

// VTABLE: HYPERX1001 0x004b19a4
class CAvoidObjectsPool : public AgendaItemOwner
{
public:
    CAvoidObjectsPool()
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

    ~CAvoidObjectsPool();
};

// SYNTHETIC: HYPERX1001 0x00415440
// CAvoidObjectsPool::`scalar deleting destructor'

#endif
