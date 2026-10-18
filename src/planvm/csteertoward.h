#ifndef HYPERBLADE_CSTEERTOWARD_H
#define HYPERBLADE_CSTEERTOWARD_H

#include "planvm/kindagendaitem.h"

class EngineObject;

// Slot 0 is KindAgendaItem's shared deleting destructor.
// VTABLE: HYPERX 0x004b1d04
class CSteerToward : public KindAgendaItem
{
public:
    CSteerToward()
    : m_pPosition(nullptr)
    , m_pStep(nullptr)
    , m_pVelDir(nullptr)
    , m_pflSpeed(nullptr)
    , m_pTarget(nullptr)
    {
    }

    void Process(int nArg) override;
    void Release(int nArg) override;
    void Bind(class EngineObject* pSubject, class GameBehaviorObject* pNode) override;
    void OnListChanged() override;

    void        IntegrateSpringAndGravity();
    static void AddStep(Vec3* pTarget, const Vec3* pStep);

    float GetSpeed() const
    {
        return *m_pflSpeed;
    }

    Vec3* m_pPosition; // 0x10, INTERACTOR_POSITION at six sites
    // The body's prevPos; Release copies the position into it.
    Vec3* m_pTo;     // 0x14
    Vec3* m_pStep;   // 0x18, INTERACTOR_STEP at six against VELOCITY at four
    Vec3* m_pVelDir; // 0x1c, the address of the body's velDir
    // The address of the body's flSpeed; CSlideOnRail's field of that name is not this one.
    float* m_pflSpeed; // 0x20
    // Filled only by src/planvm/planactionlaunch.cpp.
    EngineObject* m_pTarget; // 0x24
    // The body's pPlayer, copied by value; nothing on this class reads it.
    class CPlayer* m_pPlayer; // 0x28
    // The body's netTarget; Release copies it into the target below.
    Vec3* m_pNetTarget; // 0x2c
    // The body's bNetTargetPending; Release tests and clears it, so it is consume-once.
    char* m_pNetTargetPending; // 0x30
    // The body's nPrevTick; nothing on this class reads it.
    unsigned int* m_pPrevTick; // 0x34
    // The target, three words Release seeds together from the net target and moves by the step.
    Vec3 m_vTarget; // 0x38
};

#endif
