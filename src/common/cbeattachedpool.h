#ifndef HYPERBLADE_CBEATTACHEDPOOL_H
#define HYPERBLADE_CBEATTACHEDPOOL_H

#include "common/agendaitemowner.h"

// VTABLE: HYPERX 0x004b1a30
class CBeAttachedPool : public AgendaItemOwner
{
public:
    CBeAttachedPool()
    {
        m_pHead = nullptr;
    }

    void  Init() override;
    void* Alloc() override;

    ~CBeAttachedPool() override;
};

// SYNTHETIC: HYPERX 0x00414fe0
// CBeAttachedPool::`scalar deleting destructor'

#endif
