#ifndef HYPERBLADE_CPROJECTANDBOUNCE_H
#define HYPERBLADE_CPROJECTANDBOUNCE_H

#include "planvm/kindagendaitem.h"

class CollisionShape;
class EngineObject;

// Slot 0 is KindAgendaItem's deleting destructor, which thirty three tables share.
// VTABLE: HYPERX 0x004b1bcc
class CProjectAndBounce : public KindAgendaItem
{
public:
    CProjectAndBounce()
    : m_pVelocity(nullptr)
    , m_pVelDir(nullptr)
    , m_pSource(nullptr)
    , m_pDest(nullptr)
    , m_pSpeed(nullptr)
    , m_pShape(nullptr)
    , m_pNormal(nullptr)
    , m_pPlayer(nullptr)
    , m_pTarget(nullptr)
    , m_pNetTargetPending(nullptr)
    , m_pnPrevTick(nullptr)
    , m_nMode(0)
    {
    }

    void ProjectAndBounce(int nArg);

    void IntegrateSpringAndGravity();
    void AddStep(const Vec3* pStep);

    float GetSpeed() const
    {
        return *m_pSpeed;
    }

    void Process(int nArg) override;
    void Release(int nArg) override;
    void Bind(class EngineObject* pSubject, class GameBehaviorObject* pNode) override;
    void OnListChanged() override;

    Vec3*           m_pVelocity; // 0x10
    Vec3*           m_pVelDir;   // 0x14, the address of the body's velDir
    Vec3*           m_pSource;   // 0x18
    Vec3*           m_pDest;     // 0x1c
    float*          m_pSpeed;    // 0x20, INTERACTOR_SPEED at six sites
    CollisionShape* m_pShape;    // 0x24, INTERACTOR_SHAPE at six
    Vec3*           m_pNormal;   // 0x28, INTERACTOR_NORMAL at two
    // The body's pPlayer, copied by value; nothing on this class reads it.
    class CPlayer* m_pPlayer; // 0x2c
    // The address of the body's netTarget; the reset arm reads the steps below out of it.
    Vec3* m_pTarget;           // 0x30
    char* m_pNetTargetPending; // 0x34
    // Stored and never read.
    unsigned int* m_pnPrevTick; // 0x38
    Vec3          m_vStep;      // 0x3c
    int           m_nMode;      // 0x48
};

#endif
