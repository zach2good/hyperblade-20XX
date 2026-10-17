#ifndef HYPERBLADE_BEHOBJDONOTHINGPOOL_H
#define HYPERBLADE_BEHOBJDONOTHINGPOOL_H

#include "common/behaviorowner.h"

// VTABLE: HYPERX 0x004b2488
class BehObjDoNothingPool : public BehaviorOwner
{
public:
    BehObjDoNothingPool()
    {
        m_pHead = 0;
    }

    void  Init() override;
    void* Alloc() override;

    ~BehObjDoNothingPool() override;
};

// SYNTHETIC: HYPERX 0x00421380
// BehObjDoNothingPool::`scalar deleting destructor'

#endif
