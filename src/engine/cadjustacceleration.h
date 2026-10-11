#ifndef HYPERBLADE_CADJUSTACCELERATION_H
#define HYPERBLADE_CADJUSTACCELERATION_H

#include "planvm/releasingagendaitem.h"

class EngineObject;

// Slot 10 holds Interactor::SetMember, a body the linker shared with this class's Tick.
// VTABLE: HYPERX 0x004b1d38
class CAdjustAcceleration : public ReleasingAgendaItem
{
public:
    CAdjustAcceleration()
    : m_pSmoothVel(0)
    , m_pVelDir(0)
    , m_pflStep(0)
    , m_pTargetPos(0)
    {
    }

    virtual void Process(int nArg);
    virtual void Bind(class EngineObject* pSubject, class GameBehaviorObject* pNode);
    virtual void OnListChanged();

    virtual void Tick(const Vec3* pTargetPos)
    {
        m_pTargetPos = pTargetPos;
    }

    float GetStep() const
    {
        return *m_pflStep;
    }

    Vec3*  m_pSmoothVel; // 0x10
    Vec3*  m_pVelDir;    // 0x14
    float* m_pflStep;    // 0x18
    // Cleared by Bind and by OnListChanged, set by Tick, and read by nothing.
    const Vec3* m_pTargetPos; // 0x1c
};

#endif
