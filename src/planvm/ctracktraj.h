#ifndef HYPERBLADE_CTRACKTRAJ_H
#define HYPERBLADE_CTRACKTRAJ_H

#include <memory>

#include "common/compat.h"
#include "planvm/baseresetagendaitem.h"

class EngineObject;
class Spline;

// VTABLE: HYPERX 0x004b1710
class CTrackTraj : public BaseResetAgendaItem
{
public:
    CTrackTraj();

    void Process(int nArg) override;
    void Bind(class EngineObject* pSubject, class GameBehaviorObject* pNode) override;

    ~CTrackTraj() override;

    void InitRange();
    void SetDirectedRange(float flLow, float flHigh);
    void InitComplexRange();
    void SetBlendT(int nValue);

    // ~Spline walks SplineCurve::m_pNext from offset 0, so the node list and spline are one field.
    std::unique_ptr<Spline> m_pSpline;  // 0x10
    Vec3                    m_vTrajPos; // 0x14
    int                     m_nMode;    // 0x20
    float                   m_flBlend;  // 0x24
};

// SYNTHETIC: HYPERX 0x0040a5a0
// CTrackTraj::`scalar deleting destructor'

#endif
