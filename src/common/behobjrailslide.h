#ifndef HYPERBLADE_BEHOBJRAILSLIDE_H
#define HYPERBLADE_BEHOBJRAILSLIDE_H

#include "common/surfacebehaviorobject.h"

class EngineObject;

// VTABLE: HYPERX 0x004b2684
class BehObj_RailSlide : public SurfaceBehaviorObject
{
public:
    virtual ~BehObj_RailSlide();
    virtual void Init(class EngineObject* pOwner, struct PlanNode* pNode, int nFlags);
    BehObj_RailSlide();
};

// SYNTHETIC: HYPERX 0x0041db50
// BehObj_RailSlide::`scalar deleting destructor'

STATIC_ASSERT(sizeof(BehObj_RailSlide) == 0x98);

#endif
