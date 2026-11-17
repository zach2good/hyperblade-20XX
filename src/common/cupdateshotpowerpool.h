#ifndef HYPERBLADE_CUPDATESHOTPOWERPOOL_H
#define HYPERBLADE_CUPDATESHOTPOWERPOOL_H

#include "common/agendaitemowner.h"

// VTABLE: HYPERX1001 0x004b1814
class CUpdateShotPowerPool : public AgendaItemOwner
{
public:
    CUpdateShotPowerPool()
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

    ~CUpdateShotPowerPool();
};

// SYNTHETIC: HYPERX1001 0x004160c0
// CUpdateShotPowerPool::`scalar deleting destructor'

#endif
