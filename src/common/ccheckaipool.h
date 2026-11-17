#ifndef HYPERBLADE_CCHECKAIPOOL_H
#define HYPERBLADE_CCHECKAIPOOL_H

#include "common/agendaitemowner.h"

// VTABLE: HYPERX1001 0x004b17d8
class CCheckAIPool : public AgendaItemOwner
{
public:
    CCheckAIPool()
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

    ~CCheckAIPool();
};

// SYNTHETIC: HYPERX1001 0x004162a0
// CCheckAIPool::`scalar deleting destructor'

#endif
