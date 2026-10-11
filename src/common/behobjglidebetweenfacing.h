#ifndef HYPERBLADE_BEHOBJGLIDEBETWEENFACING_H
#define HYPERBLADE_BEHOBJGLIDEBETWEENFACING_H

#include "common/surfacebehaviorobject.h"

class EngineObject;

// VTABLE: HYPERX 0x004b260c
class BehObj_GlideBetweenFacing : public SurfaceBehaviorObject
{
public:
    virtual ~BehObj_GlideBetweenFacing();
    virtual void Init(class EngineObject* pOwner, struct PlanNode* pNode, int nFlags);
    BehObj_GlideBetweenFacing();

    float GetBlendStep() const
    {
        return m_flBlendStep;
    }

    void SetBlendStep(float flStep)
    {
        m_flBlendStep = flStep;
    }

    // BehParam::SetModsTweenNFacing writes these four; the third is the one the behaviour heads for.
    class EngineObject* m_pObjectA;   // 0x98
    class EngineObject* m_pObjectB;   // 0x9c
    class EngineObject* m_pAimObject; // 0xa0
    // One over the plan's period.
    float m_flBlendStep; // 0xa4
};

// SYNTHETIC: HYPERX 0x0041d6c0
// BehObj_GlideBetweenFacing::`scalar deleting destructor'

STATIC_ASSERT(sizeof(BehObj_GlideBetweenFacing) == 0xa8);

#endif
