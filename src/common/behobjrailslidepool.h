#ifndef HYPERBLADE_BEHOBJRAILSLIDEPOOL_H
#define HYPERBLADE_BEHOBJRAILSLIDEPOOL_H

#include "common/behaviorowner.h"

// VTABLE: HYPERX 0x004b253c
class BehObjRailSlidePool : public BehaviorOwner
{
public:
    BehObjRailSlidePool()
    {
        m_pHead = 0;
    }

    void  Init() override;
    void* Alloc() override;

    ~BehObjRailSlidePool() override;
};

// SYNTHETIC: HYPERX 0x00420de0
// BehObjRailSlidePool::`scalar deleting destructor'

#endif
