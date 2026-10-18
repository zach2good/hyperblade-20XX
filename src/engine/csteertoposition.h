#ifndef HYPERBLADE_CSTEERTOPOSITION_H
#define HYPERBLADE_CSTEERTOPOSITION_H

#include "planvm/releasingagendaitem.h"

class EngineObject;

// The widest pool kind: six of its thirteen slots are its own.
// VTABLE: HYPERX 0x004b1c9c
class CSteerToPosition : public ReleasingAgendaItem
{
public:
    CSteerToPosition()
    : m_pOrigin(nullptr)
    , m_pAxis(nullptr)
    , m_pChase(nullptr)
    , m_flUnused(0)
    , m_cArrived(0)
    {
    }

    void         Process(int nArg) override;
    void         Bind(class EngineObject* pSubject, class GameBehaviorObject* pNode) override;
    void         OnListChanged() override;
    void         Tick(const Vec3* pTargetPos) override;
    struct Vec3* GetVector() override;
    void         OnKind(short nSlot) override;

    Vec3* m_pOrigin; // 0x10
    Vec3* m_pAxis;   // 0x14
    // The body's own surface normal; Bind stores its address here.
    Vec3*         m_pNormal; // 0x18
    Vec3          m_vTarget; // 0x1c
    EngineObject* m_pChase;  // 0x28
    // Bind sets it to a hundred; nothing reads it.
    float m_flUnused; // 0x2c
    char  m_cArrived; // 0x30
};

#endif
