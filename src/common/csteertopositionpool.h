#ifndef HYPERBLADE_CSTEERTOPOSITIONPOOL_H
#define HYPERBLADE_CSTEERTOPOSITIONPOOL_H

#include "common/agendaitemowner.h"

// VTABLE: HYPERX 0x004b19f4
class CSteerToPositionPool : public AgendaItemOwner
{
public:
    CSteerToPositionPool()
    {
        m_pHead = 0;
    }

    virtual void  Init();
    virtual void* Alloc();

    ~CSteerToPositionPool();
};

// SYNTHETIC: HYPERX 0x004151c0
// CSteerToPositionPool::`scalar deleting destructor'

#endif
