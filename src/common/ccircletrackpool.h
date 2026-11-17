#ifndef HYPERBLADE_CCIRCLETRACKPOOL_H
#define HYPERBLADE_CCIRCLETRACKPOOL_H

#include "common/agendaitemowner.h"

// VTABLE: HYPERX1001 0x004b17c4
class CCircleTrackPool : public AgendaItemOwner
{
public:
    CCircleTrackPool()
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

    ~CCircleTrackPool();
};

// SYNTHETIC: HYPERX1001 0x00416340
// CCircleTrackPool::`scalar deleting destructor'

#endif
