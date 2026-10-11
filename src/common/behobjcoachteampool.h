#ifndef HYPERBLADE_BEHOBJCOACHTEAMPOOL_H
#define HYPERBLADE_BEHOBJCOACHTEAMPOOL_H

#include "common/behaviorowner.h"

// VTABLE: HYPERX 0x004b23fc
class BehObjCoachTeamPool : public BehaviorOwner
{
public:
    BehObjCoachTeamPool()
    {
        m_pHead = 0;
    }

    virtual void  Init();
    virtual void* Alloc();

    ~BehObjCoachTeamPool();
};

// SYNTHETIC: HYPERX 0x004217e0
// BehObjCoachTeamPool::`scalar deleting destructor'

#endif
