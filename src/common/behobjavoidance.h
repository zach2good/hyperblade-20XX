#ifndef HYPERBLADE_BEHOBJAVOIDANCE_H
#define HYPERBLADE_BEHOBJAVOIDANCE_H

#include "common/surfacebehaviorobject.h"

class EngineObject;

// VTABLE: HYPERX 0x004b2624
class BehObj_Avoidance : public SurfaceBehaviorObject
{
public:
    virtual ~BehObj_Avoidance();
    virtual void Init(class EngineObject* pOwner, struct PlanNode* pNode, int nFlags);
    BehObj_Avoidance();

private:
    int m_anUnused[3]; // 0x98
};

// SYNTHETIC: HYPERX 0x0041d7d0
// BehObj_Avoidance::`scalar deleting destructor'

STATIC_ASSERT(sizeof(BehObj_Avoidance) == 0xa4);

#endif
