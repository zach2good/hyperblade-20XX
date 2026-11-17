#ifndef HYPERBLADE_CDISPLAYPASSBARPOOL_H
#define HYPERBLADE_CDISPLAYPASSBARPOOL_H

#include "common/agendaitemowner.h"

// VTABLE: HYPERX1001 0x004b1800
class CDisplayPassBarPool : public AgendaItemOwner
{
public:
    CDisplayPassBarPool()
    {
        m_pHead = 0;
    }

    // Every pool's copy folds onto this one; v1.0.0.0 keeps them apart.
    // FUNCTION: HYPERX1000 0x0044b970
    // FUNCTION: HYPERX1001 0x00414b00
    virtual AgendaItemOwner* Destroy()
    {
        delete this;

        return 0;
    }

    virtual void  Init();
    virtual char  Insert(void* pNode);
    virtual void* Alloc();

    ~CDisplayPassBarPool();
};

// SYNTHETIC: HYPERX1001 0x00416160
// CDisplayPassBarPool::`scalar deleting destructor'

#endif
