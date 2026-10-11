#ifndef HYPERBLADE_CAVOIDOBJECTS_H
#define HYPERBLADE_CAVOIDOBJECTS_H

#include "planvm/releasingagendaitem.h"

class EngineObject;
struct PlanNode;

// Slots 0 and 2 are shared bodies defined elsewhere.
// VTABLE: HYPERX 0x004b1d6c
class CAvoidObjects : public ReleasingAgendaItem
{
public:
    virtual void Process(int nArg);
    virtual void BindWithNode(class EngineObject* pObject, class GameBehaviorObject* pOwner, PlanNode* pNode, int nArg);
    virtual void Bind(class EngineObject* pSubject, class GameBehaviorObject* pNode);

    // Five tables share this body at slot seven or eight; the original's link folded the copies.
    // FUNCTION: HYPERX 0x00413ef0
    virtual char IsReplaceable()
    {
        return 1;
    }

    virtual void OnListChanged();
    void         AddAvoidanceTarget(short nIndex);
    static short SignTowardGoal(short nGoalSlot);

    Vec3*         m_pAccum;          // 0x10
    Vec3*         m_pLine;           // 0x14
    float*        m_pLateral;        // 0x18, the body's flAccelDt
    char          m_cReleaseOnRange; // 0x1c
    float         m_flRange;         // 0x20
    float         m_flStrength;      // 0x24
    EngineObject* m_pSwerveTarget;   // 0x28
    EngineObject* m_pDrawTarget;     // 0x2c
};

#endif
