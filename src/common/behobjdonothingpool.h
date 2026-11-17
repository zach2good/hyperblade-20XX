#ifndef HYPERBLADE_BEHOBJDONOTHINGPOOL_H
#define HYPERBLADE_BEHOBJDONOTHINGPOOL_H

#include "common/behaviorowner.h"

// VTABLE: HYPERX1001 0x004b2488
class BehObjDoNothingPool : public BehaviorOwner
{
public:
    BehObjDoNothingPool()
    {
        m_pHead = 0;
    }

    virtual BehaviorOwner* Destroy()
    {
        delete this;

        return 0;
    }

    virtual void  Init();
    virtual char  Insert(void* pNode);
    virtual void* Alloc();

    ~BehObjDoNothingPool();
};

// SYNTHETIC: HYPERX1001 0x00421380
// BehObjDoNothingPool::`scalar deleting destructor'

#endif
