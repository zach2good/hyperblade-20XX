#ifndef HYPERBLADE_CMONITORKEYSPOOL_H
#define HYPERBLADE_CMONITORKEYSPOOL_H

#include "common/agendaitemowner.h"

// VTABLE: HYPERX1001 0x004b1940
class CMonitorKeysPool : public AgendaItemOwner
{
public:
    CMonitorKeysPool()
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

    ~CMonitorKeysPool();
};

// SYNTHETIC: HYPERX1001 0x00415760
// CMonitorKeysPool::`scalar deleting destructor'

#endif
