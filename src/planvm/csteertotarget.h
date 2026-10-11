#ifndef HYPERBLADE_CSTEERTOTARGET_H
#define HYPERBLADE_CSTEERTOTARGET_H

#include "planvm/releasingagendaitem.h"

class EngineObject;

// Slots 0 and 2 are bodies many tables share, declared where they are defined.
// VTABLE: HYPERX 0x004b1c34
class CSteerToTarget : public ReleasingAgendaItem
{
public:
    CSteerToTarget()
    : m_pAxis(0)
    , m_cArrived(0)
    {
    }

    virtual void Process(int nArg);
    virtual void Bind(class EngineObject* pSubject, class GameBehaviorObject* pNode);
    virtual void OnListChanged();
    void         SetTargetObjectByIndex(short nIndex);

    Vec3*         m_pAxis;    // 0x10
    EngineObject* m_pTarget;  // 0x14
    char          m_cArrived; // 0x18
};

#endif
