#ifndef HYPERBLADE_CPANSOUNDPOOL_H
#define HYPERBLADE_CPANSOUNDPOOL_H

#include "common/agendaitemowner.h"

// VTABLE: HYPERX1001 0x004b1878
class CPanSoundPool : public AgendaItemOwner
{
public:
    CPanSoundPool()
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

    ~CPanSoundPool();
};

// SYNTHETIC: HYPERX1001 0x00415da0
// CPanSoundPool::`scalar deleting destructor'

#endif
