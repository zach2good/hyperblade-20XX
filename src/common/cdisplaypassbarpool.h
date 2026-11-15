#ifndef HYPERBLADE_CDISPLAYPASSBARPOOL_H
#define HYPERBLADE_CDISPLAYPASSBARPOOL_H

#include "common/agendaitemowner.h"

// VTABLE: HYPERX 0x004b1800
class CDisplayPassBarPool : public AgendaItemOwner
{
public:
    CDisplayPassBarPool()
    {
        m_pHead = 0;
    }

    // The linker folds every pool's copy onto this one, as the original's did.
    // FUNCTION: HYPERX100 0x0044b970
    // FUNCTION: HYPERX 0x00414b00
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

// SYNTHETIC: HYPERX 0x00416160
// CDisplayPassBarPool::`scalar deleting destructor'

#endif
