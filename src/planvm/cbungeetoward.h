#ifndef HYPERBLADE_CBUNGEETOWARD_H
#define HYPERBLADE_CBUNGEETOWARD_H

#include "planvm/releasingagendaitem.h"

class EngineObject;
struct PlanNode;

// Slots 0 and 2 are shared bodies declared where they are defined; slot 7 shares CAvoidObjects::IsReplaceable's body.
// VTABLE: HYPERX 0x004b1da0
class CBungeeToward : public ReleasingAgendaItem
{
    // m_bReady is this kind's released flag.

public:
    virtual void Process(int nArg);
    virtual void BindWithNode(class EngineObject* pObject, class GameBehaviorObject* pOwner, PlanNode* pNode, int nArg);
    virtual void Bind(class EngineObject* pSubject, class GameBehaviorObject* pNode);
    virtual void OnListChanged();

    // Slot seven answers one where the base answers nought; the original folds it onto one shared body.
    virtual unsigned char GetReadyFlag()
    {
        return 1;
    }

    // Float parameters, so a stored constant passes through the FPU as in the client.
    void SetNearRange(float flNearRange)
    {
        m_flNearRange = flNearRange;
    }

    void SetDotGate(float flDotGate)
    {
        m_flDotGate = flDotGate;
    }

    Vec3*         m_pAccum;       // 0x10
    EngineObject* m_pTarget;      // 0x14
    Vec3*         m_pAnchor;      // 0x18
    float         m_flGain;       // 0x1c
    float         m_flRestLength; // 0x20
    float         m_flFarRange;   // 0x24
    float         m_flNearRange;  // 0x28
    float         m_flDotGate;    // 0x2c
    short         m_nCountdown;   // 0x30
    char          m_cAttached;    // 0x32
    char          m_cLeadTarget;  // 0x33
};

#endif
