#ifndef HYPERBLADE_CRELAXYAWPOOL_H
#define HYPERBLADE_CRELAXYAWPOOL_H

#include "common/agendaitemowner.h"

// VTABLE: HYPERX1001 0x004b1968
class CRelaxYawPool : public AgendaItemOwner
{
public:
    CRelaxYawPool()
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

    ~CRelaxYawPool();
};

// SYNTHETIC: HYPERX1001 0x00415620
// CRelaxYawPool::`scalar deleting destructor'

#endif
