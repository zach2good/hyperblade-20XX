#ifndef HYPERBLADE_CSLIDEONRAILPOOL_H
#define HYPERBLADE_CSLIDEONRAILPOOL_H

#include "common/agendaitemowner.h"

// VTABLE: HYPERX1001 0x004b1a58
class CSlideOnRailPool : public AgendaItemOwner
{
public:
    CSlideOnRailPool()
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

    ~CSlideOnRailPool();
};

// SYNTHETIC: HYPERX1001 0x00414ea0
// CSlideOnRailPool::`scalar deleting destructor'

#endif
