#ifndef HYPERBLADE_CSLIDEONRAILPOOL_H
#define HYPERBLADE_CSLIDEONRAILPOOL_H

#include "common/agendaitemowner.h"

// VTABLE: HYPERX 0x004b1a58
class CSlideOnRailPool : public AgendaItemOwner
{
public:
    CSlideOnRailPool()
    {
        m_pHead = 0;
    }

    void  Init() override;
    void* Alloc() override;

    ~CSlideOnRailPool() override;
};

// SYNTHETIC: HYPERX 0x00414ea0
// CSlideOnRailPool::`scalar deleting destructor'

#endif
