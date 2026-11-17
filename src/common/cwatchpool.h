#ifndef HYPERBLADE_CWATCHPOOL_H
#define HYPERBLADE_CWATCHPOOL_H

#include "common/agendaitemowner.h"

// VTABLE: HYPERX1001 0x004b18b4
class CWatchPool : public AgendaItemOwner
{
public:
    CWatchPool()
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

    ~CWatchPool();
};

// SYNTHETIC: HYPERX1001 0x00415bc0
// CWatchPool::`scalar deleting destructor'

#endif
