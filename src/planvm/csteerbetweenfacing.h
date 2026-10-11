#ifndef HYPERBLADE_CSTEERBETWEENFACING_H
#define HYPERBLADE_CSTEERBETWEENFACING_H

#include "planvm/releasingagendaitem.h"

class EngineObject;

// Slots 0 and 2 are bodies many tables share, declared where they are defined.
// VTABLE: HYPERX 0x004b1cd0
class CSteerBetweenFacing : public ReleasingAgendaItem
{
public:
    CSteerBetweenFacing()
    : m_pOrigin(0)
    , m_pAxis(0)
    , m_flUnused(0)
    , m_cArrived(0)
    {
    }

    virtual void Process(int nArg);
    virtual void Bind(class EngineObject* pSubject, class GameBehaviorObject* pNode);
    virtual void OnListChanged();

    class BehObj_GlideBetweenFacing* m_pPath;    // 0x10
    Vec3*                            m_pOrigin;  // 0x14
    Vec3*                            m_pAxis;    // 0x18
    Vec3*                            m_pNormal;  // 0x1c
    float                            m_flUnused; // 0x20
    char                             m_cArrived; // 0x24
};

#endif
