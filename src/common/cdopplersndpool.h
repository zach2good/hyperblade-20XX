#ifndef HYPERBLADE_CDOPPLERSNDPOOL_H
#define HYPERBLADE_CDOPPLERSNDPOOL_H

#include "common/agendaitemowner.h"

// VTABLE: HYPERX1001 0x004b1864
class CDopplerSndPool : public AgendaItemOwner
{
public:
    CDopplerSndPool()
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

    ~CDopplerSndPool();
};

// SYNTHETIC: HYPERX1001 0x00415e40
// CDopplerSndPool::`scalar deleting destructor'

#endif
