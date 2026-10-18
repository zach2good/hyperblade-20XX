#ifndef HYPERBLADE_BEHOBJCOACHTEAMPOOL_H
#define HYPERBLADE_BEHOBJCOACHTEAMPOOL_H

#include "common/behaviorowner.h"

// VTABLE: HYPERX 0x004b23fc
class BehObjCoachTeamPool : public BehaviorOwner
{
public:
    BehObjCoachTeamPool()
    {
        m_pHead = nullptr;
    }

    void  Init() override;
    void* Alloc() override;

    ~BehObjCoachTeamPool() override;
};

// SYNTHETIC: HYPERX 0x004217e0
// BehObjCoachTeamPool::`scalar deleting destructor'

#endif
