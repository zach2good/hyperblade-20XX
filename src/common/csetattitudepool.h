#ifndef HYPERBLADE_CSETATTITUDEPOOL_H
#define HYPERBLADE_CSETATTITUDEPOOL_H

#include "common/agendaitemowner.h"

// VTABLE: HYPERX 0x004b197c
class CSetAttitudePool : public AgendaItemOwner
{
public:
    CSetAttitudePool()
    {
        m_pHead = nullptr;
    }

    void  Init() override;
    void* Alloc() override;

    ~CSetAttitudePool() override;
};

// SYNTHETIC: HYPERX 0x00415580
// CSetAttitudePool::`scalar deleting destructor'

#endif
