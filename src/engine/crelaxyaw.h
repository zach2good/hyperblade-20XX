#ifndef HYPERBLADE_CRELAXYAW_H
#define HYPERBLADE_CRELAXYAW_H

#include "planvm/kindagendaitem.h"

class EngineObject;
struct PlanNode;

// Slot 0 is CGlideOnSurface's shared deleting destructor.
// VTABLE: HYPERX 0x004b1e08
class CRelaxYaw : public KindAgendaItem
{
public:
    virtual void Process(int nArg);
    virtual void Release(int nArg);
    virtual void BindWithNode(class EngineObject* pObject, class GameBehaviorObject* pOwner, PlanNode* pNode, int nArg);
    virtual void Bind(class EngineObject* pSubject, class GameBehaviorObject* pNode);
    virtual void OnListChanged();

    void SetAngle(float flAngle)
    {
        m_flAngle = flAngle;
    }

    float m_flDecay; // 0x10, INTERACTOR_DECAY
    float m_flAngle; // 0x14, INTERACTOR_ANGLE at fourteen sites
    // planaction.cpp writes 0x18 and 0x1c on these, but the original allocates only 0x18 bytes.
};

#endif
