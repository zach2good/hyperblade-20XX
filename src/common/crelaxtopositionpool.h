#ifndef HYPERBLADE_CRELAXTOPOSITIONPOOL_H
#define HYPERBLADE_CRELAXTOPOSITIONPOOL_H

#include "common/agendaitemowner.h"

// VTABLE: HYPERX1001 0x004b1904
class CRelaxToPositionPool : public AgendaItemOwner
{
public:
    CRelaxToPositionPool()
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

    ~CRelaxToPositionPool();
};

// SYNTHETIC: HYPERX1001 0x00415940
// CRelaxToPositionPool::`scalar deleting destructor'

#endif
