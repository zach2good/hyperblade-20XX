#ifndef HYPERBLADE_BEHOBJRAILSLIDE_H
#define HYPERBLADE_BEHOBJRAILSLIDE_H

#include "common/surfacebehaviorobject.h"

class EngineObject;

// VTABLE: HYPERX 0x004b2684
class BehObj_RailSlide : public SurfaceBehaviorObject
{
public:
    ~BehObj_RailSlide() override;
    void Init(class EngineObject* pOwner, struct PlanNode* pNode, int nFlags) override;
    BehObj_RailSlide();
};

// SYNTHETIC: HYPERX 0x0041db50
// BehObj_RailSlide::`scalar deleting destructor'

#endif
