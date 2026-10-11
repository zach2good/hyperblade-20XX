#ifndef HYPERBLADE_CPALETTEEFFECTPOOL_H
#define HYPERBLADE_CPALETTEEFFECTPOOL_H

#include "common/agendaitemowner.h"

// VTABLE: HYPERX 0x004b183c
class CPaletteEffectPool : public AgendaItemOwner
{
public:
    CPaletteEffectPool()
    {
        m_pHead = 0;
    }

    virtual void  Init();
    virtual void* Alloc();

    ~CPaletteEffectPool();
};

// SYNTHETIC: HYPERX 0x00415f80
// CPaletteEffectPool::`scalar deleting destructor'

#endif
