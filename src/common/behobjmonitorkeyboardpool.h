#ifndef HYPERBLADE_BEHOBJMONITORKEYBOARDPOOL_H
#define HYPERBLADE_BEHOBJMONITORKEYBOARDPOOL_H

#include "common/behaviorowner.h"

// VTABLE: HYPERX 0x004b24ec
class BehObjMonitorKeyboardPool : public BehaviorOwner
{
public:
    BehObjMonitorKeyboardPool()
    {
        m_pHead = 0;
    }

    virtual void  Init();
    virtual void* Alloc();

    ~BehObjMonitorKeyboardPool();
};

// SYNTHETIC: HYPERX 0x00421060
// BehObjMonitorKeyboardPool::`scalar deleting destructor'

#endif
