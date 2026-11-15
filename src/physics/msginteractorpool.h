#ifndef HYPERBLADE_MSGINTERACTORPOOL_H
#define HYPERBLADE_MSGINTERACTORPOOL_H

#include "common/nodeowner.h"

// VTABLE: HYPERX 0x004b3794
class MsgInteractorPool : public NodeOwner
{
public:
    MsgInteractorPool()
    {
        m_pHead = 0;
    }

    virtual NodeOwner* Destroy()
    {
        delete this;

        return 0;
    }

    virtual void  Init();
    virtual char  Insert(void* pNode);
    virtual void* Alloc();

    ~MsgInteractorPool();
};

// SYNTHETIC: HYPERX 0x0046beb0
// MsgInteractorPool::`scalar deleting destructor'

#endif
