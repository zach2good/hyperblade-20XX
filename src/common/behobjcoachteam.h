#ifndef HYPERBLADE_BEHOBJCOACHTEAM_H
#define HYPERBLADE_BEHOBJCOACHTEAM_H

#include "engine/inplacebehaviorobject.h"

class EngineObject;

// VTABLE: HYPERX 0x004b272c
class BehObj_CoachTeam : public InPlaceBehaviorObject
{
public:
    ~BehObj_CoachTeam() override;
    void Init(class EngineObject* pOwner, struct PlanNode* pNode, int nFlags) override;
    BehObj_CoachTeam();
};

// SYNTHETIC: HYPERX 0x0041e2d0
// BehObj_CoachTeam::`scalar deleting destructor'

#endif
