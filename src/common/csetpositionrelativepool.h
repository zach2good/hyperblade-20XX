#ifndef HYPERBLADE_CSETPOSITIONRELATIVEPOOL_H
#define HYPERBLADE_CSETPOSITIONRELATIVEPOOL_H

#include "common/agendaitemowner.h"

// VTABLE: HYPERX 0x004b1a08
class CSetPositionRelativePool : public AgendaItemOwner
{
public:
    CSetPositionRelativePool()
    {
        m_pHead = nullptr;
    }

    void  Init() override;
    void* Alloc() override;

    ~CSetPositionRelativePool() override;
};

// SYNTHETIC: HYPERX 0x00415120
// CSetPositionRelativePool::`scalar deleting destructor'

#endif
