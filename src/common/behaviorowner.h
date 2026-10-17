#ifndef HYPERBLADE_BEHAVIOROWNER_H
#define HYPERBLADE_BEHAVIOROWNER_H

#include "common/pooledownerroot.h"

class GameBehaviorObject;

// VTABLE: HYPERX 0x004b2834
class BehaviorOwner : public PooledOwnerRoot
{
public:
    BehaviorOwner()
    {
    }

    char Insert(void* pNode) override;

    // FUNCTION: HYPERX 0x004209f0
    ~BehaviorOwner() override
    {
    }

    GameBehaviorObject* m_pHead;
};

// SYNTHETIC: HYPERX 0x00420a00
// BehaviorOwner::`scalar deleting destructor'

#endif
